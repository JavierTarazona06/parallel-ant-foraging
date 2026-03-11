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
      raw_measurements_full.csv
      table_a.csv
      table_b.csv
      init_times.csv
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

ns_to_ms() {
  local ns_value="$1"
  awk -v ns="${ns_value}" 'BEGIN { printf "%.6f", ns / 1000000.0 }'
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

  RAW_FULL_CSV="${THREAD_DIR}/raw_measurements_full.csv"
  cat > "${RAW_FULL_CSV}" <<'CSV'
rep,measured_iters,nb_ants,render_enabled,p0_ns,p1_ns,p2_ns,k0_ns,k1_ns,k2_ns,k3_ns,k4_ns,k5_ns,r0_ns,e0_ns,p0_ms,p1_ms,p2_ms,k0_ms_iter,k1_ms_iter,k2_ms_iter,k3_ms_iter,k4_ms_iter,k5_ms_iter,r0_ms_iter,e0_ms_iter,t_iter_total_ms_iter,ants_per_s
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
    nb_ants="$(metric_from_file nb_ants "${METRICS_FILE}")"
    render_enabled="$(metric_from_file render_enabled "${METRICS_FILE}")"
    p0_ns="$(metric_from_file p0_ns "${METRICS_FILE}")"
    p1_ns="$(metric_from_file p1_ns "${METRICS_FILE}")"
    p2_ns="$(metric_from_file p2_ns "${METRICS_FILE}")"
    k1_ns="$(metric_from_file k1_ns "${METRICS_FILE}")"
    k2_ns="$(metric_from_file k2_ns "${METRICS_FILE}")"
    k3_ns="$(metric_from_file k3_ns "${METRICS_FILE}")"
    k5_ns="$(metric_from_file k5_ns "${METRICS_FILE}")"
    r0_ns="$(metric_from_file r0_ns "${METRICS_FILE}")"
    e0_ns="$(metric_from_file e0_ns "${METRICS_FILE}")"

    if [[ -z "${measured_iters}" || "${measured_iters}" -eq 0 ]]; then
      echo "Invalid measured_iterations for thread=${t}, rep=${rep}" >&2
      exit 1
    fi

    if [[ -z "${nb_ants}" ]]; then
      nb_ants="$(metric_from_file ants "${METRICS_FILE}")"
    fi

    for metric in nb_ants render_enabled p0_ns p1_ns p2_ns k1_ns k2_ns k3_ns k5_ns r0_ns e0_ns; do
      if [[ -z "${!metric}" ]]; then
        printf -v "${metric}" "0"
      fi
    done

    p0_ms="$(ns_to_ms "${p0_ns}")"
    p1_ms="$(ns_to_ms "${p1_ns}")"
    p2_ms="$(ns_to_ms "${p2_ns}")"
    k0_ms_iter="$(ns_per_iter_to_ms "${k0_ns}" "${measured_iters}")"
    k1_ms_iter="$(ns_per_iter_to_ms "${k1_ns}" "${measured_iters}")"
    k2_ms_iter="$(ns_per_iter_to_ms "${k2_ns}" "${measured_iters}")"
    k3_ms_iter="$(ns_per_iter_to_ms "${k3_ns}" "${measured_iters}")"
    k4_ms_iter="$(ns_per_iter_to_ms "${k4_ns}" "${measured_iters}")"
    k5_ms_iter="$(ns_per_iter_to_ms "${k5_ns}" "${measured_iters}")"
    r0_ms_iter="$(ns_per_iter_to_ms "${r0_ns}" "${measured_iters}")"
    e0_ms_iter="$(ns_per_iter_to_ms "${e0_ns}" "${measured_iters}")"
    t_iter_total_ms_iter="$(awk -v a="${k0_ms_iter}" -v b="${r0_ms_iter}" -v c="${e0_ms_iter}" 'BEGIN { printf "%.6f", a+b+c }')"
    ants_per_s="$(awk -v ants="${nb_ants}" -v n="${measured_iters}" -v ns="${k0_ns}" 'BEGIN { if (ns > 0) printf "%.6f", (ants*n*1e9)/ns; else print "0.000000" }')"

    echo "${rep},${measured_iters},${k0_ns},${k4_ns},${k0_ms_iter},${k4_ms_iter}" >> "${RAW_CSV}"
    echo "${rep},${measured_iters},${nb_ants},${render_enabled},${p0_ns},${p1_ns},${p2_ns},${k0_ns},${k1_ns},${k2_ns},${k3_ns},${k4_ns},${k5_ns},${r0_ns},${e0_ns},${p0_ms},${p1_ms},${p2_ms},${k0_ms_iter},${k1_ms_iter},${k2_ms_iter},${k3_ms_iter},${k4_ms_iter},${k5_ms_iter},${r0_ms_iter},${e0_ms_iter},${t_iter_total_ms_iter},${ants_per_s}" \
      >> "${RAW_FULL_CSV}"
  done

  k0_stats="$(stats_for_column "${RAW_CSV}" "k0_ms_iter")"
  k4_stats="$(stats_for_column "${RAW_CSV}" "k4_ms_iter")"
  THREAD_K0_MEAN["${t}"]="$(split_stats "${k0_stats}" mean)"
  THREAD_K4_MEAN["${t}"]="$(split_stats "${k4_stats}" mean)"

  mean_std_p0="$(stats_for_column "${RAW_FULL_CSV}" "p0_ms")"
  mean_std_p1="$(stats_for_column "${RAW_FULL_CSV}" "p1_ms")"
  mean_std_p2="$(stats_for_column "${RAW_FULL_CSV}" "p2_ms")"
  mean_std_k0="$(stats_for_column "${RAW_FULL_CSV}" "k0_ms_iter")"
  mean_std_k1="$(stats_for_column "${RAW_FULL_CSV}" "k1_ms_iter")"
  mean_std_k2="$(stats_for_column "${RAW_FULL_CSV}" "k2_ms_iter")"
  mean_std_k3="$(stats_for_column "${RAW_FULL_CSV}" "k3_ms_iter")"
  mean_std_k4="$(stats_for_column "${RAW_FULL_CSV}" "k4_ms_iter")"
  mean_std_k5="$(stats_for_column "${RAW_FULL_CSV}" "k5_ms_iter")"
  mean_std_r0="$(stats_for_column "${RAW_FULL_CSV}" "r0_ms_iter")"
  mean_std_e0="$(stats_for_column "${RAW_FULL_CSV}" "e0_ms_iter")"
  mean_std_total="$(stats_for_column "${RAW_FULL_CSV}" "t_iter_total_ms_iter")"
  mean_std_ants="$(stats_for_column "${RAW_FULL_CSV}" "ants_per_s")"

  mean_k0="$(split_stats "${mean_std_k0}" mean)"
  mean_k1="$(split_stats "${mean_std_k1}" mean)"
  mean_k4="$(split_stats "${mean_std_k4}" mean)"

  TABLE_A_CSV="${THREAD_DIR}/table_a.csv"
  cat > "${TABLE_A_CSV}" <<'CSV'
Parte,ms_iter_promedio,desviacion_std,porcentaje_sobre_kernel
CSV

  for part in K1 K2 K3 K4 K5; do
    mean_std_var="mean_std_${part,,}"
    mean_val="$(split_stats "${!mean_std_var}" mean)"
    std_val="$(split_stats "${!mean_std_var}" std)"
    pct_val="$(awk -v m="${mean_val}" -v k0="${mean_k0}" 'BEGIN { if (k0>0) printf "%.4f", (100.0*m)/k0; else print "0.0000" }')"
    echo "${part},${mean_val},${std_val},${pct_val}" >> "${TABLE_A_CSV}"
  done

  echo "R0 (fuera de kernel),$(split_stats "${mean_std_r0}" mean),$(split_stats "${mean_std_r0}" std),n/a" >> "${TABLE_A_CSV}"
  echo "E0 (fuera de kernel),$(split_stats "${mean_std_e0}" mean),$(split_stats "${mean_std_e0}" std),n/a" >> "${TABLE_A_CSV}"

  TABLE_B_CSV="${THREAD_DIR}/table_b.csv"
  {
    echo "repeticion,T_iter_total_ms,T_kernel_ms,T_render_ms,hormigas_por_s"
    awk -F, '
      NR==1 {
        for (i=1; i<=NF; ++i) idx[$i]=i;
        next;
      }
      {
        print $idx["rep"] "," $idx["t_iter_total_ms_iter"] "," $idx["k0_ms_iter"] "," $idx["r0_ms_iter"] "," $idx["ants_per_s"];
      }' "${RAW_FULL_CSV}"
    echo "Media,$(split_stats "${mean_std_total}" mean),${mean_k0},$(split_stats "${mean_std_r0}" mean),$(split_stats "${mean_std_ants}" mean)"
    echo "Desviacion estandar,$(split_stats "${mean_std_total}" std),$(split_stats "${mean_std_k0}" std),$(split_stats "${mean_std_r0}" std),$(split_stats "${mean_std_ants}" std)"
  } > "${TABLE_B_CSV}"

  INIT_TIMES_CSV="${THREAD_DIR}/init_times.csv"
  {
    echo "parte,ms_promedio,desviacion_std"
    echo "P0,$(split_stats "${mean_std_p0}" mean),$(split_stats "${mean_std_p0}" std)"
    echo "P1,$(split_stats "${mean_std_p1}" mean),$(split_stats "${mean_std_p1}" std)"
    echo "P2,$(split_stats "${mean_std_p2}" mean),$(split_stats "${mean_std_p2}" std)"
  } > "${INIT_TIMES_CSV}"

  {
    echo "thread=${t}"
    echo "mean_k0_ms_iter=${mean_k0}"
    echo "mean_k1_ms_iter=${mean_k1}"
    echo "mean_k4_ms_iter=${mean_k4}"
    echo "note_k2_k3=In OpenMP runs, K2/K3 are thread-accumulated and not strict wall-time."
  } > "${THREAD_DIR}/summary_thread.env"

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
