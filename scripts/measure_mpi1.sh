#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BIN="${REPO_ROOT}/src/ant_simu.exe"

print_help() {
  cat <<'EOF'
Usage:
  ./scripts/measure_mpi1.sh [--folder NAME] [--np 1,2,4,8]
                            [--iterations N] [--warmup N] [--reps N]
                            [--skip-build] [-h|--help]

Description:
  Runs MPI1 benchmark sweeps over process counts.
  For each np value, it executes:
    mpirun -np <np> ./src/ant_simu.exe --exec mpi1 --layout soa --benchmark --no-render ...

Outputs:
  results/<folder>/mpi1/soa/<timestamp>/
    run_config.env
    system_info.txt
    summary_np.csv
    np_<np>/
      run_config.env
      rep_<r>.metrics
      rep_<r>.stderr
      raw_measurements.csv
EOF
}

detect_physical_cores() {
  local cores_per_socket sockets
  cores_per_socket="$(lscpu 2>/dev/null | awk -F: '/Core\(s\) per socket/{gsub(/ /,"",$2); print $2; exit}')"
  sockets="$(lscpu 2>/dev/null | awk -F: '/Socket\(s\)/{gsub(/ /,"",$2); print $2; exit}')"
  if [[ "${cores_per_socket:-}" =~ ^[0-9]+$ && "${sockets:-}" =~ ^[0-9]+$ ]] && [[ "${cores_per_socket}" -gt 0 && "${sockets}" -gt 0 ]]; then
    echo $((cores_per_socket * sockets))
    return
  fi
  nproc 2>/dev/null || echo 1
}

default_np_csv() {
  local max_np="$1"
  local p=1
  local list=()
  while [[ "${p}" -le "${max_np}" ]]; do
    list+=("${p}")
    p=$((p * 2))
  done
  if [[ "${list[$((${#list[@]} - 1))]}" -ne "${max_np}" ]]; then
    list+=("${max_np}")
  fi

  local out=""
  for v in "${list[@]}"; do
    if [[ -z "${out}" ]]; then
      out="${v}"
    else
      out="${out},${v}"
    fi
  done
  echo "${out}"
}

validate_np_csv() {
  local csv="$1"
  IFS=',' read -r -a arr <<< "${csv}"
  if [[ "${#arr[@]}" -eq 0 ]]; then
    return 1
  fi
  for p in "${arr[@]}"; do
    if ! [[ "${p}" =~ ^[0-9]+$ ]] || [[ "${p}" -le 0 ]]; then
      return 1
    fi
  done
  return 0
}

metric_from_file() {
  local metric_name="$1"
  local file_path="$2"
  awk -v key="${metric_name}" '$1=="METRIC" && $2==key {print $3}' "${file_path}" | tail -n 1
}

ns_per_iter_to_ms() {
  local ns_value="$1"
  local iters="$2"
  awk -v ns="${ns_value}" -v n="${iters}" 'BEGIN { if (n > 0) printf "%.6f", (ns / 1000000.0) / n; else print "0.000000" }'
}

stats_for_column() {
  local csv_file="$1"
  local column_name="$2"
  awk -F, -v col="${column_name}" '
    NR==1 {
      for (i=1; i<=NF; ++i) if ($i==col) idx=i;
      next;
    }
    idx>0 {
      n++;
      sum += $idx;
      sumsq += ($idx * $idx);
    }
    END {
      if (n == 0) {
        print "0.000000,0.000000";
        exit;
      }
      mean = sum / n;
      var = (sumsq / n) - (mean * mean);
      if (var < 0) var = 0;
      std = sqrt(var);
      printf "%.6f,%.6f\n", mean, std;
    }' "${csv_file}"
}

split_stats() {
  local stats_line="$1"
  local which="$2"
  if [[ "${which}" == "mean" ]]; then
    echo "${stats_line}" | cut -d, -f1
  else
    echo "${stats_line}" | cut -d, -f2
  fi
}

SPECIFIC_FOLDER="test_mpi1_sweep"
ITERATIONS=1200
WARMUP=200
REPS=5
SKIP_BUILD=0
NP_CSV=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help)
      print_help
      exit 0
      ;;
    --folder)
      shift
      [[ $# -gt 0 ]] || { echo "Error: --folder requires a value." >&2; exit 1; }
      SPECIFIC_FOLDER="$1"
      ;;
    --folder=*)
      SPECIFIC_FOLDER="${1#*=}"
      ;;
    --np)
      shift
      [[ $# -gt 0 ]] || { echo "Error: --np requires a CSV list (e.g. 1,2,4)." >&2; exit 1; }
      NP_CSV="$1"
      ;;
    --np=*)
      NP_CSV="${1#*=}"
      ;;
    --iterations)
      shift
      [[ $# -gt 0 ]] || { echo "Error: --iterations requires a value." >&2; exit 1; }
      ITERATIONS="$1"
      ;;
    --iterations=*)
      ITERATIONS="${1#*=}"
      ;;
    --warmup)
      shift
      [[ $# -gt 0 ]] || { echo "Error: --warmup requires a value." >&2; exit 1; }
      WARMUP="$1"
      ;;
    --warmup=*)
      WARMUP="${1#*=}"
      ;;
    --reps)
      shift
      [[ $# -gt 0 ]] || { echo "Error: --reps requires a value." >&2; exit 1; }
      REPS="$1"
      ;;
    --reps=*)
      REPS="${1#*=}"
      ;;
    --skip-build)
      SKIP_BUILD=1
      ;;
    *)
      echo "Error: unknown argument '$1'." >&2
      print_help >&2
      exit 1
      ;;
  esac
  shift
done

for n in "${ITERATIONS}" "${WARMUP}" "${REPS}"; do
  if ! [[ "${n}" =~ ^[0-9]+$ ]]; then
    echo "Error: iterations/warmup/reps must be non-negative integers." >&2
    exit 1
  fi
done

if [[ "${WARMUP}" -ge "${ITERATIONS}" ]]; then
  echo "Error: warmup must be strictly lower than iterations." >&2
  exit 1
fi

if [[ "${NP_CSV}" == "" ]]; then
  PHYSICAL_CORES="$(detect_physical_cores)"
  NP_CSV="$(default_np_csv "${PHYSICAL_CORES}")"
fi

if ! validate_np_csv "${NP_CSV}"; then
  echo "Error: invalid --np list '${NP_CSV}'." >&2
  exit 1
fi

IFS=',' read -r -a NP_LIST <<< "${NP_CSV}"

RUN_ID="$(date +%Y%m%d_%H%M%S)"
OUT_DIR="${REPO_ROOT}/results/${SPECIFIC_FOLDER}/mpi1/soa/${RUN_ID}"
LATEST_LINK="${REPO_ROOT}/results/${SPECIFIC_FOLDER}/mpi1/soa/latest"

mkdir -p "${OUT_DIR}"

{
  echo "run_id=${RUN_ID}"
  echo "specific_folder=${SPECIFIC_FOLDER}"
  echo "exec=mpi1"
  echo "layout=soa"
  echo "np_csv=${NP_CSV}"
  echo "repetitions=${REPS}"
  echo "iterations=${ITERATIONS}"
  echo "warmup=${WARMUP}"
  echo "benchmark_date_utc=$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
} > "${OUT_DIR}/run_config.env"

{
  echo "=== uname -a ==="
  uname -a
  echo
  echo "=== lscpu ==="
  lscpu
  echo
  echo "=== nproc ==="
  nproc
  echo
  echo "=== mpirun --version ==="
  mpirun --version
} > "${OUT_DIR}/system_info.txt"

if [[ "${SKIP_BUILD}" -eq 0 ]]; then
  echo "[1/4] Build release..."
  make -C "${REPO_ROOT}/src" clean all > "${OUT_DIR}/build.log" 2>&1
else
  echo "[1/4] Skipping build (--skip-build)."
fi

SUMMARY_CSV="${OUT_DIR}/summary_np.csv"
cat > "${SUMMARY_CSV}" <<'CSV'
np,reps,mean_k0_ms_iter,std_k0_ms_iter,mean_k1_ms_iter,std_k1_ms_iter,mean_k4_ms_iter,std_k4_ms_iter,mean_k5_ms_iter,std_k5_ms_iter
CSV

echo "[2/4] Running MPI sweep: np=${NP_CSV}, reps=${REPS} ..."
for np in "${NP_LIST[@]}"; do
  NP_DIR="${OUT_DIR}/np_${np}"
  mkdir -p "${NP_DIR}"

  {
    echo "np=${np}"
    echo "repetitions=${REPS}"
    echo "iterations=${ITERATIONS}"
    echo "warmup=${WARMUP}"
  } > "${NP_DIR}/run_config.env"

  RAW_CSV="${NP_DIR}/raw_measurements.csv"
  cat > "${RAW_CSV}" <<'CSV'
rep,measured_iters,k0_ns,k1_ns,k4_ns,k5_ns,k0_ms_iter,k1_ms_iter,k4_ms_iter,k5_ms_iter
CSV

  for rep in $(seq 1 "${REPS}"); do
    METRICS_FILE="${NP_DIR}/rep_${rep}.metrics"
    STDERR_FILE="${NP_DIR}/rep_${rep}.stderr"

    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
      mpirun -np "${np}" "${BIN}" --benchmark --no-render --exec mpi1 --layout soa \
      --iterations "${ITERATIONS}" --warmup "${WARMUP}" \
      > "${METRICS_FILE}" 2> "${STDERR_FILE}"

    measured_iters="$(metric_from_file measured_iterations "${METRICS_FILE}")"
    k0_ns="$(metric_from_file k0_ns "${METRICS_FILE}")"
    k1_ns="$(metric_from_file k1_ns "${METRICS_FILE}")"
    k4_ns="$(metric_from_file k4_ns "${METRICS_FILE}")"
    k5_ns="$(metric_from_file k5_ns "${METRICS_FILE}")"

    if [[ -z "${measured_iters}" || "${measured_iters}" -eq 0 ]]; then
      echo "Error: invalid measured_iterations for np=${np}, rep=${rep}." >&2
      exit 1
    fi

    k0_ms_iter="$(ns_per_iter_to_ms "${k0_ns}" "${measured_iters}")"
    k1_ms_iter="$(ns_per_iter_to_ms "${k1_ns}" "${measured_iters}")"
    k4_ms_iter="$(ns_per_iter_to_ms "${k4_ns}" "${measured_iters}")"
    k5_ms_iter="$(ns_per_iter_to_ms "${k5_ns}" "${measured_iters}")"

    echo "${rep},${measured_iters},${k0_ns},${k1_ns},${k4_ns},${k5_ns},${k0_ms_iter},${k1_ms_iter},${k4_ms_iter},${k5_ms_iter}" \
      >> "${RAW_CSV}"
  done

  k0_stats="$(stats_for_column "${RAW_CSV}" "k0_ms_iter")"
  k1_stats="$(stats_for_column "${RAW_CSV}" "k1_ms_iter")"
  k4_stats="$(stats_for_column "${RAW_CSV}" "k4_ms_iter")"
  k5_stats="$(stats_for_column "${RAW_CSV}" "k5_ms_iter")"

  echo "${np},${REPS},$(split_stats "${k0_stats}" mean),$(split_stats "${k0_stats}" std),$(split_stats "${k1_stats}" mean),$(split_stats "${k1_stats}" std),$(split_stats "${k4_stats}" mean),$(split_stats "${k4_stats}" std),$(split_stats "${k5_stats}" mean),$(split_stats "${k5_stats}" std)" \
    >> "${SUMMARY_CSV}"
done

echo "[3/4] Updating latest symlink..."
mkdir -p "$(dirname "${LATEST_LINK}")"
ln -sfn "${OUT_DIR}" "${LATEST_LINK}"

echo "[4/4] Done."
echo "Results saved under: ${OUT_DIR}"
