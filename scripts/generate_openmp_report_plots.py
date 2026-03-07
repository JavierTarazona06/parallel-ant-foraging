#!/usr/bin/env python3
"""Generate OpenMP figures for the report from benchmark CSV outputs."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path

import matplotlib.pyplot as plt


REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_OMP_DIR = REPO_ROOT / "results" / "test_openmp_report_v2" / "omp" / "soa" / "latest"
DEFAULT_SERIAL_DIR = REPO_ROOT / "results" / "test_openmp_report_v2" / "serial" / "soa" / "latest"
DEFAULT_OUT_DIR = REPO_ROOT / "docs" / "report" / "imgs" / "openmp"


def read_summary(path: Path):
    rows = []
    with path.open(newline="") as f:
        for row in csv.DictReader(f):
            rows.append(
                {
                    "thread": int(row["thread"]),
                    "k0_mean": float(row["mean_k0_ms_iter"]),
                    "k0_std": float(row["std_k0_ms_iter"]),
                    "k4_mean": float(row["mean_k4_ms_iter"]),
                    "k4_std": float(row["std_k4_ms_iter"]),
                    "speedup": float(row["speedup_vs_thread1"]),
                }
            )
    return rows


def read_table_a(path: Path):
    values = {}
    with path.open(newline="") as f:
        for row in csv.DictReader(f):
            values[row["Parte"]] = float(row["ms_iter_promedio"])
    return values


def save_speedup_plot(rows, out_dir: Path):
    threads = [r["thread"] for r in rows]
    speedup = [r["speedup"] for r in rows]

    plt.figure(figsize=(7.0, 4.2))
    plt.plot(threads, speedup, marker="o", linewidth=2.0, label="Mesure OpenMP")
    plt.plot(threads, threads, linestyle="--", linewidth=1.8, label="Speedup ideal")
    plt.xlabel("Nombre de threads")
    plt.ylabel("Speedup S(p)")
    plt.title("Speedup OpenMP (SoA)")
    plt.grid(True, alpha=0.25)
    plt.xticks(threads)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_dir / "openmp_speedup.png", dpi=180)
    plt.close()


def save_k0_plot(rows, out_dir: Path):
    threads = [r["thread"] for r in rows]
    k0_mean = [r["k0_mean"] for r in rows]
    k0_std = [r["k0_std"] for r in rows]

    plt.figure(figsize=(7.0, 4.2))
    plt.errorbar(threads, k0_mean, yerr=k0_std, marker="o", capsize=4, linewidth=2.0)
    plt.xlabel("Nombre de threads")
    plt.ylabel("K0 moyen (ms / itération)")
    plt.title("Temps noyau K0 vs nombre de threads (OpenMP SoA)")
    plt.grid(True, alpha=0.25)
    plt.xticks(threads)
    plt.tight_layout()
    plt.savefig(out_dir / "openmp_k0_ms_iter.png", dpi=180)
    plt.close()


def save_phase_breakdown_plot(rows, omp_dir: Path, out_dir: Path):
    threads = [r["thread"] for r in rows]
    k1 = []
    k4 = []
    k5 = []
    for t in threads:
        table = read_table_a(omp_dir / f"threads_{t}" / "table_a.csv")
        k1.append(table.get("K1", 0.0))
        k4.append(table.get("K4", 0.0))
        k5.append(table.get("K5", 0.0))

    plt.figure(figsize=(7.6, 4.6))
    plt.bar(threads, k1, label="K1 (advance ants)")
    plt.bar(threads, k4, bottom=k1, label="K4 (evaporation)")
    bottom_k1k4 = [a + b for a, b in zip(k1, k4)]
    plt.bar(threads, k5, bottom=bottom_k1k4, label="K5 (update)")
    plt.xlabel("Nombre de threads")
    plt.ylabel("ms / itération")
    plt.title("Décomposition du noyau OpenMP (SoA)")
    plt.xticks(threads)
    plt.grid(True, axis="y", alpha=0.25)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_dir / "openmp_kernel_breakdown.png", dpi=180)
    plt.close()


def save_thread1_compare_plot(omp_rows, serial_rows, out_dir: Path):
    omp_k0 = next(r["k0_mean"] for r in omp_rows if r["thread"] == 1)
    serial_k0 = next(r["k0_mean"] for r in serial_rows if r["thread"] == 1)
    omp_std = next(r["k0_std"] for r in omp_rows if r["thread"] == 1)
    serial_std = next(r["k0_std"] for r in serial_rows if r["thread"] == 1)

    labels = ["serial+soa (1 th)", "omp+soa (1 th)"]
    means = [serial_k0, omp_k0]
    stds = [serial_std, omp_std]

    plt.figure(figsize=(6.8, 4.0))
    plt.bar(labels, means, yerr=stds, capsize=5)
    plt.ylabel("K0 moyen (ms / itération)")
    plt.title("Comparaison 1 thread: backend serial vs backend omp")
    plt.grid(True, axis="y", alpha=0.25)
    plt.tight_layout()
    plt.savefig(out_dir / "openmp_thread1_backend_compare.png", dpi=180)
    plt.close()


def parse_args():
    parser = argparse.ArgumentParser(description="Generate OpenMP report figures from CSV metrics.")
    parser.add_argument("--omp-dir", type=Path, default=DEFAULT_OMP_DIR,
                        help=f"Path to OpenMP sweep folder (default: {DEFAULT_OMP_DIR})")
    parser.add_argument("--serial-dir", type=Path, default=DEFAULT_SERIAL_DIR,
                        help=f"Path to serial reference folder (default: {DEFAULT_SERIAL_DIR})")
    parser.add_argument("--out-dir", type=Path, default=DEFAULT_OUT_DIR,
                        help=f"Output folder for generated figures (default: {DEFAULT_OUT_DIR})")
    return parser.parse_args()


def main():
    args = parse_args()
    omp_dir = args.omp_dir
    serial_dir = args.serial_dir
    out_dir = args.out_dir

    out_dir.mkdir(parents=True, exist_ok=True)
    omp_rows = read_summary(omp_dir / "summary_threads.csv")
    serial_rows = read_summary(serial_dir / "summary_threads.csv")

    save_speedup_plot(omp_rows, out_dir)
    save_k0_plot(omp_rows, out_dir)
    save_phase_breakdown_plot(omp_rows, omp_dir, out_dir)
    save_thread1_compare_plot(omp_rows, serial_rows, out_dir)

    print(f"Generated plots in: {out_dir}")


if __name__ == "__main__":
    main()
