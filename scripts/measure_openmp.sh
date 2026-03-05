#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BIN="${REPO_ROOT}/src/ant_simu.exe"

print_help() {
  cat <<'EOF'
Usage:
  ./scripts/measure_openmp.sh [--folder NAME] [--layout aos|soa] [--exec omp|serial]
                              [--threads 1,2,4,8] [--iterations N] [--warmup N] [--reps N]
                              [--skip-build] [-h|--help]

Description:
  Runs an OpenMP thread sweep for benchmark mode with fixed workload per run.
  For each thread count:
  - Sets OMP_NUM_THREADS=<t>
  - Executes several repetitions
  - Stores per-repetition METRIC output under a dedicated thread folder

Outputs:
  results/<folder>/<exec>/<layout>/<timestamp>/
    run_config.env
    system_info.txt
    summary_threads.csv
    threads_<t>/
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

default_threads_csv() {
  local max_threads="$1"
  local t=1
  local list=()
  while [[ "${t}" -le "${max_threads}" ]]; do
    list+=("${t}")
    t=$((t * 2))
  done
  if [[ "${list[$((${#list[@]} - 1))]}" -ne "${max_threads}" ]]; then
    list+=("${max_threads}")
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

validate_threads_csv() {
  local csv="$1"
  IFS=',' read -r -a arr <<< "${csv}"
  if [[ "${#arr[@]}" -eq 0 ]]; then
    return 1
  fi
  for t in "${arr[@]}"; do
    if ! [[ "${t}" =~ ^[0-9]+$ ]] || [[ "${t}" -le 0 ]]; then
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

SPECIFIC_FOLDER="test_openmp_sweep"
LAYOUT="aos"
EXEC_MODEL="omp"
ITERATIONS=1200
WARMUP=200
REPS=5
SKIP_BUILD=0
THREADS_CSV=""

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
    --layout)
      shift
      [[ $# -gt 0 ]] || { echo "Error: --layout requires aos|soa." >&2; exit 1; }
      LAYOUT="$1"
      ;;
    --layout=*)
      LAYOUT="${1#*=}"
      ;;
    --exec)
      shift
      [[ $# -gt 0 ]] || { echo "Error: --exec requires omp|serial." >&2; exit 1; }
      EXEC_MODEL="$1"
      ;;
    --exec=*)
      EXEC_MODEL="${1#*=}"
      ;;
    --threads)
      shift
      [[ $# -gt 0 ]] || { echo "Error: --threads requires a CSV list (e.g. 1,2,4)." >&2; exit 1; }
      THREADS_CSV="$1"
      ;;
    --threads=*)
      THREADS_CSV="${1#*=}"
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

if [[ "${LAYOUT}" != "aos" && "${LAYOUT}" != "soa" ]]; then
  echo "Error: invalid --layout '${LAYOUT}' (expected aos|soa)." >&2
  exit 1
fi

if [[ "${EXEC_MODEL}" != "omp" && "${EXEC_MODEL}" != "serial" ]]; then
  echo "Error: invalid --exec '${EXEC_MODEL}' (expected omp|serial)." >&2
  exit 1
fi

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

if [[ "${THREADS_CSV}" == "" ]]; then
  PHYSICAL_CORES="$(detect_physical_cores)"
  THREADS_CSV="$(default_threads_csv "${PHYSICAL_CORES}")"
fi

if ! validate_threads_csv "${THREADS_CSV}"; then
  echo "Error: invalid --threads list '${THREADS_CSV}'." >&2
  exit 1
fi

IFS=',' read -r -a THREAD_LIST <<< "${THREADS_CSV}"

RUN_ID="$(date +%Y%m%d_%H%M%S)"
OUT_DIR="${REPO_ROOT}/results/${SPECIFIC_FOLDER}/${EXEC_MODEL}/${LAYOUT}/${RUN_ID}"
LATEST_LINK="${REPO_ROOT}/results/${SPECIFIC_FOLDER}/${EXEC_MODEL}/${LAYOUT}/latest"

mkdir -p "${OUT_DIR}"
mkdir -p "${REPO_ROOT}/results/${SPECIFIC_FOLDER}/${EXEC_MODEL}/${LAYOUT}"

{
  echo "run_id=${RUN_ID}"
  echo "specific_folder=${SPECIFIC_FOLDER}"
  echo "exec=${EXEC_MODEL}"
  echo "layout=${LAYOUT}"
  echo "repetitions=${REPS}"
  echo "iterations=${ITERATIONS}"
  echo "warmup=${WARMUP}"
  echo "threads_csv=${THREADS_CSV}"
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
} > "${OUT_DIR}/system_info.txt"

if [[ "${SKIP_BUILD}" -eq 0 ]]; then
  echo "[1/4] Build release..."
  make -C "${REPO_ROOT}/src" clean all > "${OUT_DIR}/build.log" 2>&1
fi

SUMMARY_CSV="${OUT_DIR}/summary_threads.csv"
cat > "${SUMMARY_CSV}" <<'CSV'
thread,reps,mean_k0_ms_iter,std_k0_ms_iter,mean_k4_ms_iter,std_k4_ms_iter,speedup_vs_thread1
CSV

declare -A THREAD_K0_MEAN
declare -A THREAD_K4_MEAN

echo "[2/4] Running thread sweep: ${THREADS_CSV}"
for t in "${THREAD_LIST[@]}"; do
  THREAD_DIR="${OUT_DIR}/threads_${t}"
  mkdir -p "${THREAD_DIR}"
  RAW_CSV="${THREAD_DIR}/raw_measurements.csv"
  cat > "${RAW_CSV}" <<'CSV'
rep,measured_iterations,k0_ns,k4_ns,k0_ms_iter,k4_ms_iter
CSV

  {
    echo "thread=${t}"
    echo "exec=${EXEC_MODEL}"
    echo "layout=${LAYOUT}"
    echo "repetitions=${REPS}"
    echo "iterations=${ITERATIONS}"
    echo "warmup=${WARMUP}"
  } > "${THREAD_DIR}/run_config.env"

  for rep in $(seq 1 "${REPS}"); do
    METRICS_FILE="${THREAD_DIR}/rep_${rep}.metrics"
    STDERR_FILE="${THREAD_DIR}/rep_${rep}.stderr"

    OMP_NUM_THREADS="${t}" SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
      "${BIN}" --benchmark --no-render --exec "${EXEC_MODEL}" --layout "${LAYOUT}" \
      --iterations "${ITERATIONS}" --warmup "${WARMUP}" \
      > "${METRICS_FILE}" 2> "${STDERR_FILE}"

    measured_iters="$(metric_from_file measured_iterations "${METRICS_FILE}")"
    k0_ns="$(metric_from_file k0_ns "${METRICS_FILE}")"
    k4_ns="$(metric_from_file k4_ns "${METRICS_FILE}")"

    if [[ -z "${measured_iters}" || "${measured_iters}" -eq 0 ]]; then
      echo "Invalid measured_iterations for thread=${t}, rep=${rep}" >&2
      exit 1
    fi

    k0_ms_iter="$(ns_per_iter_to_ms "${k0_ns}" "${measured_iters}")"
    k4_ms_iter="$(ns_per_iter_to_ms "${k4_ns}" "${measured_iters}")"
    echo "${rep},${measured_iters},${k0_ns},${k4_ns},${k0_ms_iter},${k4_ms_iter}" >> "${RAW_CSV}"
  done

  k0_stats="$(stats_for_column "${RAW_CSV}" "k0_ms_iter")"
  k4_stats="$(stats_for_column "${RAW_CSV}" "k4_ms_iter")"
  THREAD_K0_MEAN["${t}"]="$(split_stats "${k0_stats}" mean)"
  THREAD_K4_MEAN["${t}"]="$(split_stats "${k4_stats}" mean)"

  echo "${t},${REPS},${THREAD_K0_MEAN[${t}]},$(split_stats "${k0_stats}" std),${THREAD_K4_MEAN[${t}]},$(split_stats "${k4_stats}" std),pending" >> "${SUMMARY_CSV}"
done

BASE_THREAD="${THREAD_LIST[0]}"
BASE_K0="${THREAD_K0_MEAN[${BASE_THREAD}]}"

TMP_SUMMARY="${OUT_DIR}/summary_threads.tmp.csv"
{
  head -n 1 "${SUMMARY_CSV}"
  tail -n +2 "${SUMMARY_CSV}" | while IFS=, read -r thread reps k0_mean k0_std k4_mean k4_std _; do
    speedup="$(awk -v b="${BASE_K0}" -v t="${k0_mean}" 'BEGIN { if (t>0) printf "%.6f", b/t; else print "0.000000" }')"
    echo "${thread},${reps},${k0_mean},${k0_std},${k4_mean},${k4_std},${speedup}"
  done
} > "${TMP_SUMMARY}"
mv "${TMP_SUMMARY}" "${SUMMARY_CSV}"

ln -sfn "${OUT_DIR}" "${LATEST_LINK}"

echo "[3/4] Done."
echo "Results: ${OUT_DIR}"
echo "Summary: ${SUMMARY_CSV}"
