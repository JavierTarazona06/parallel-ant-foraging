#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

print_help() {
  cat <<'EOF'
Usage:
  ./scripts/measure_temps.sh [specific_folder]
  ./scripts/measure_temps.sh --folder <specific_folder>
  ./scripts/measure_temps.sh -h|--help

Description:
  Ejecuta la medicion de tiempos del Ejercicio 4:
  - Build release
  - 5 repeticiones
  - 1200 iteraciones por repeticion
  - 200 iteraciones warm-up descartadas
  - OMP_NUM_THREADS=1

Outputs:
  results/<specific_folder>/<timestamp>/
  results/<specific_folder>/latest -> enlace a la ultima corrida
EOF
}

SPECIFIC_FOLDER="test_time_measurements"
SPECIFIC_FOLDER_SET=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help)
      print_help
      exit 0
      ;;
    --folder)
      shift
      if [[ $# -eq 0 || -z "${1}" ]]; then
        echo "Error: --folder requiere un valor." >&2
        exit 1
      fi
      SPECIFIC_FOLDER="$1"
      SPECIFIC_FOLDER_SET=1
      ;;
    --folder=*)
      SPECIFIC_FOLDER="${1#*=}"
      SPECIFIC_FOLDER_SET=1
      if [[ -z "${SPECIFIC_FOLDER}" ]]; then
        echo "Error: --folder requiere un valor." >&2
        exit 1
      fi
      ;;
    -*)
      echo "Error: opcion desconocida '$1'." >&2
      print_help >&2
      exit 1
      ;;
    *)
      if [[ "${SPECIFIC_FOLDER_SET}" -eq 0 ]]; then
        SPECIFIC_FOLDER="$1"
        SPECIFIC_FOLDER_SET=1
      else
        echo "Error: argumento inesperado '$1'." >&2
        print_help >&2
        exit 1
      fi
      ;;
  esac
  shift
done

RUN_ID="$(date +%Y%m%d_%H%M%S)"
OUT_DIR="${REPO_ROOT}/results/${SPECIFIC_FOLDER}/${RUN_ID}"
LATEST_LINK="${REPO_ROOT}/results/${SPECIFIC_FOLDER}/latest"

REPS=5
ITERATIONS=1200
WARMUP=200
THREADS=1
NB_ANTS=5000

mkdir -p "${OUT_DIR}"
mkdir -p "${REPO_ROOT}/results/${SPECIFIC_FOLDER}"

{
  echo "run_id=${RUN_ID}"
  echo "specific_folder=${SPECIFIC_FOLDER}"
  echo "repetitions=${REPS}"
  echo "iterations=${ITERATIONS}"
  echo "warmup=${WARMUP}"
  echo "threads=${THREADS}"
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

echo "[1/4] Build release..."
make -C "${REPO_ROOT}/src" clean all > "${OUT_DIR}/build.log" 2>&1

RAW_CSV="${OUT_DIR}/raw_measurements.csv"
cat > "${RAW_CSV}" <<'CSV'
rep,measured_iters,nb_ants,render_enabled,p0_ns,p1_ns,p2_ns,k0_ns,k1_ns,k2_ns,k3_ns,k4_ns,k5_ns,r0_ns,e0_ns,p0_ms,p1_ms,p2_ms,k0_ms_iter,k1_ms_iter,k2_ms_iter,k3_ms_iter,k4_ms_iter,k5_ms_iter,r0_ms_iter,e0_ms_iter,t_iter_total_ms_iter,ants_per_s
CSV

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

echo "[2/4] Running ${REPS} benchmark repetitions..."
for rep in $(seq 1 "${REPS}"); do
  metrics_file="${OUT_DIR}/rep_${rep}.metrics"
  stderr_file="${OUT_DIR}/rep_${rep}.stderr"

  OMP_NUM_THREADS="${THREADS}" SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    "${REPO_ROOT}/src/ant_simu.exe" --benchmark --iterations "${ITERATIONS}" --warmup "${WARMUP}" \
    > "${metrics_file}" 2> "${stderr_file}"

  measured_iters="$(metric_from_file measured_iterations "${metrics_file}")"
  render_enabled="$(metric_from_file render_enabled "${metrics_file}")"
  p0_ns="$(metric_from_file p0_ns "${metrics_file}")"
  p1_ns="$(metric_from_file p1_ns "${metrics_file}")"
  p2_ns="$(metric_from_file p2_ns "${metrics_file}")"
  k0_ns="$(metric_from_file k0_ns "${metrics_file}")"
  k1_ns="$(metric_from_file k1_ns "${metrics_file}")"
  k2_ns="$(metric_from_file k2_ns "${metrics_file}")"
  k3_ns="$(metric_from_file k3_ns "${metrics_file}")"
  k4_ns="$(metric_from_file k4_ns "${metrics_file}")"
  k5_ns="$(metric_from_file k5_ns "${metrics_file}")"
  r0_ns="$(metric_from_file r0_ns "${metrics_file}")"
  e0_ns="$(metric_from_file e0_ns "${metrics_file}")"

  if [[ -z "${measured_iters}" || "${measured_iters}" -eq 0 ]]; then
    echo "Benchmark repetition ${rep} returned invalid measured_iterations." >&2
    exit 1
  fi

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
  ants_per_s="$(awk -v ants="${NB_ANTS}" -v n="${measured_iters}" -v ns="${k0_ns}" 'BEGIN { if (ns > 0) printf "%.6f", (ants*n*1e9)/ns; else print "0.000000" }')"

  echo "${rep},${measured_iters},${NB_ANTS},${render_enabled},${p0_ns},${p1_ns},${p2_ns},${k0_ns},${k1_ns},${k2_ns},${k3_ns},${k4_ns},${k5_ns},${r0_ns},${e0_ns},${p0_ms},${p1_ms},${p2_ms},${k0_ms_iter},${k1_ms_iter},${k2_ms_iter},${k3_ms_iter},${k4_ms_iter},${k5_ms_iter},${r0_ms_iter},${e0_ms_iter},${t_iter_total_ms_iter},${ants_per_s}" \
    >> "${RAW_CSV}"
done

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

mean_std_k0="$(stats_for_column "${RAW_CSV}" "k0_ms_iter")"
mean_k0="$(split_stats "${mean_std_k0}" mean)"

mean_std_p0="$(stats_for_column "${RAW_CSV}" "p0_ms")"
mean_std_p1="$(stats_for_column "${RAW_CSV}" "p1_ms")"
mean_std_p2="$(stats_for_column "${RAW_CSV}" "p2_ms")"
mean_std_k1="$(stats_for_column "${RAW_CSV}" "k1_ms_iter")"
mean_std_k2="$(stats_for_column "${RAW_CSV}" "k2_ms_iter")"
mean_std_k3="$(stats_for_column "${RAW_CSV}" "k3_ms_iter")"
mean_std_k4="$(stats_for_column "${RAW_CSV}" "k4_ms_iter")"
mean_std_k5="$(stats_for_column "${RAW_CSV}" "k5_ms_iter")"
mean_std_r0="$(stats_for_column "${RAW_CSV}" "r0_ms_iter")"
mean_std_e0="$(stats_for_column "${RAW_CSV}" "e0_ms_iter")"
mean_std_total="$(stats_for_column "${RAW_CSV}" "t_iter_total_ms_iter")"
mean_std_ants="$(stats_for_column "${RAW_CSV}" "ants_per_s")"

mean_k1="$(split_stats "${mean_std_k1}" mean)"
mean_k4="$(split_stats "${mean_std_k4}" mean)"

TABLE_A_CSV="${OUT_DIR}/table_a.csv"
cat > "${TABLE_A_CSV}" <<'CSV'
Parte,ms_iter_promedio,desviacion_std,porcentaje_sobre_kernel
CSV

append_table_a_row() {
  local name="$1"
  local mean_std="$2"
  local include_pct="$3"
  local mean_val std_val pct_val
  mean_val="$(split_stats "${mean_std}" mean)"
  std_val="$(split_stats "${mean_std}" std)"
  if [[ "${include_pct}" == "yes" ]]; then
    pct_val="$(awk -v m="${mean_val}" -v k0="${mean_k0}" 'BEGIN { if (k0>0) printf "%.4f", (100.0*m)/k0; else print "0.0000" }')"
  else
    pct_val="n/a"
  fi
  echo "${name},${mean_val},${std_val},${pct_val}" >> "${TABLE_A_CSV}"
}

append_table_a_row "K1" "${mean_std_k1}" "yes"
append_table_a_row "K2" "${mean_std_k2}" "yes"
append_table_a_row "K3" "${mean_std_k3}" "yes"
append_table_a_row "K4" "${mean_std_k4}" "yes"
append_table_a_row "K5" "${mean_std_k5}" "yes"
append_table_a_row "R0 (fuera de kernel)" "${mean_std_r0}" "no"
append_table_a_row "E0 (fuera de kernel)" "${mean_std_e0}" "no"

INIT_TIMES_CSV="${OUT_DIR}/init_times.csv"
{
  echo "parte,ms_promedio,desviacion_std"
  echo "P0,$(split_stats "${mean_std_p0}" mean),$(split_stats "${mean_std_p0}" std)"
  echo "P1,$(split_stats "${mean_std_p1}" mean),$(split_stats "${mean_std_p1}" std)"
  echo "P2,$(split_stats "${mean_std_p2}" mean),$(split_stats "${mean_std_p2}" std)"
} > "${INIT_TIMES_CSV}"

TABLE_B_CSV="${OUT_DIR}/table_b.csv"
{
  echo "repeticion,T_iter_total_ms,T_kernel_ms,T_render_ms,hormigas_por_s"
  awk -F, '
    NR==1 {
      for (i=1; i<=NF; ++i) idx[$i]=i;
      next;
    }
    {
      print $idx["rep"] "," $idx["t_iter_total_ms_iter"] "," $idx["k0_ms_iter"] "," $idx["r0_ms_iter"] "," $idx["ants_per_s"];
    }' "${RAW_CSV}"
  echo "Media,$(split_stats "${mean_std_total}" mean),$(split_stats "${mean_std_k0}" mean),$(split_stats "${mean_std_r0}" mean),$(split_stats "${mean_std_ants}" mean)"
  echo "Desviacion estandar,$(split_stats "${mean_std_total}" std),$(split_stats "${mean_std_k0}" std),$(split_stats "${mean_std_r0}" std),$(split_stats "${mean_std_ants}" std)"
} > "${TABLE_B_CSV}"

F_VALUE="$(awk -v k1="${mean_k1}" -v k4="${mean_k4}" -v k0="${mean_k0}" 'BEGIN { if (k0>0) printf "%.6f", (k1+k4)/k0; else print "0.000000" }')"

AMDAHL_CSV="${OUT_DIR}/amdahl_prediction.csv"
{
  echo "threads,speedup_teorico"
  for p in 2 4 8 16; do
    s_val="$(awk -v f="${F_VALUE}" -v p="${p}" 'BEGIN { d=(1-f)+(f/p); if (d>0) printf "%.6f", 1.0/d; else print "0.000000" }')"
    echo "${p},${s_val}"
  done
} > "${AMDAHL_CSV}"

echo "[3/4] Generating markdown summary..."
SUMMARY_MD="${OUT_DIR}/summary.md"
{
  echo "# Medicion de tiempos - Ejercicio 4"
  echo
  echo "- Carpeta de resultados: \`${OUT_DIR}\`"
  echo "- Repeticiones: ${REPS}"
  echo "- Iteraciones por repeticion: ${ITERATIONS}"
  echo "- Warm-up descartado: ${WARMUP}"
  echo "- Hilos OpenMP: ${THREADS}"
  echo
  echo "## Tiempos one-shot de inicializacion"
  echo
  echo "| Parte | ms promedio | desviacion std |"
  echo "|---|---:|---:|"
  awk -F, 'NR>1{printf "| %s | %s | %s |\n",$1,$2,$3}' "${INIT_TIMES_CSV}"
  echo
  echo "## Tabla A (base)"
  echo
  echo "| Parte | ms/iter promedio | desviacion std | % sobre kernel |"
  echo "|---|---:|---:|---:|"
  awk -F, 'NR>1{printf "| %s | %s | %s | %s |\n",$1,$2,$3,$4}' "${TABLE_A_CSV}"
  echo
  echo "## Tabla B (vision global)"
  echo
  echo "| Repeticion | T_iter_total (ms) | T_kernel (ms) | T_render (ms) | hormigas/s |"
  echo "|---|---:|---:|---:|---:|"
  awk -F, 'NR>1{printf "| %s | %s | %s | %s | %s |\n",$1,$2,$3,$4,$5}' "${TABLE_B_CSV}"
  echo
  echo "## Fraccion paralelizable candidata"
  echo
  echo "- F = (K1 + K4) / K0 = **${F_VALUE}**"
  echo
  echo "## Speedup teorico (Amdahl)"
  echo
  echo "| Threads | S(p) |"
  echo "|---:|---:|"
  awk -F, 'NR>1{printf "| %s | %s |\n",$1,$2}' "${AMDAHL_CSV}"
} > "${SUMMARY_MD}"

ln -sfn "${OUT_DIR}" "${LATEST_LINK}"

echo "[4/4] Done."
echo "Results saved under: ${OUT_DIR}"
