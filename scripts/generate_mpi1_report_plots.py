#!/usr/bin/env python3
"""Generate MPI1 report figures from benchmark CSV/metrics outputs."""

from __future__ import annotations

import argparse
import csv
import re
import statistics
from pathlib import Path

import matplotlib.pyplot as plt


REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_MPI_DIR = REPO_ROOT / "results" / "test_mpi1_sync_every_rerun" / "20260307_155513"
DEFAULT_OUT_DIR = REPO_ROOT / "docs" / "report" / "imgs" / "mpi1"


def read_summary(path: Path):
    rows = []
    with path.open(newline="") as f:
        for row in csv.DictReader(f):
            if "k" in row:
                if int(row["k"]) != 1:
                    continue
                rows.append(
                    {
                        "np": int(row["np"]),
                        "k0_mean": float(row["k0_mean_ms_iter"]),
                        "k0_std": float(row["k0_std"]),
                        "speedup": float(row["speedup"]),
                        "efficiency": float(row["efficiency"]),
                    }
                )
            else:
                rows.append(
                    {
                        "np": int(row["np"]),
                        "k0_mean": float(row["mean_k0_ms_iter"]),
                        "k0_std": float(row["std_k0_ms_iter"]),
                    }
                )
    rows.sort(key=lambda r: r["np"])
    if not rows:
        raise ValueError(f"No rows found in summary file: {path}")
    return rows


def metric_from_file(metric_path: Path, key: str):
    with metric_path.open() as f:
        for line in f:
            if line.startswith(f"METRIC {key} "):
                return line.split()[2]
    return None


def read_k_mpi_sync_means(mpi_dir: Path):
    per_np = {}
    np_dirs = sorted(mpi_dir.glob("np_*"))
    if np_dirs:
        for np_dir in np_dirs:
            try:
                np_value = int(np_dir.name.split("_", 1)[1])
            except (IndexError, ValueError):
                continue

            values = []
            for metric_file in sorted(np_dir.glob("rep_*.metrics")):
                measured_iterations = metric_from_file(metric_file, "measured_iterations")
                k_sync_ns = metric_from_file(metric_file, "k_mpi_sync_ns")
                if measured_iterations is None or k_sync_ns is None:
                    continue
                iters = int(measured_iterations)
                if iters <= 0:
                    continue
                values.append((float(k_sync_ns) / 1_000_000.0) / iters)

            per_np[np_value] = sum(values) / len(values) if values else 0.0
        return per_np

    flat_pattern = re.compile(r"k1_np(\d+)_rep\d+\.metrics$")
    grouped = {}
    for metric_file in sorted(mpi_dir.glob("k1_np*_rep*.metrics")):
        match = flat_pattern.match(metric_file.name)
        if not match:
            continue
        np_value = int(match.group(1))
        measured_iterations = metric_from_file(metric_file, "measured_iterations")
        k_sync_ns = metric_from_file(metric_file, "k_mpi_sync_ns")
        if measured_iterations is None or k_sync_ns is None:
            continue
        iters = int(measured_iterations)
        if iters <= 0:
            continue
        grouped.setdefault(np_value, []).append((float(k_sync_ns) / 1_000_000.0) / iters)

    for np_value, values in grouped.items():
        per_np[np_value] = sum(values) / len(values) if values else 0.0
    return per_np


def read_phase_means(mpi_dir: Path):
    phase_means = {}
    flat_pattern = re.compile(r"k1_np(\d+)_rep\d+\.metrics$")
    grouped = {}
    for metric_file in sorted(mpi_dir.glob("k1_np*_rep*.metrics")):
        match = flat_pattern.match(metric_file.name)
        if not match:
            continue
        np_value = int(match.group(1))
        measured_iterations = metric_from_file(metric_file, "measured_iterations")
        if measured_iterations is None:
            continue
        iters = int(measured_iterations)
        if iters <= 0:
            continue
        grouped.setdefault(np_value, {"k1": [], "k4": [], "k5": []})
        for key in ("k1_ns", "k4_ns", "k5_ns"):
            raw = metric_from_file(metric_file, key)
            if raw is None:
                continue
            grouped[np_value][key[:2]].append((float(raw) / 1_000_000.0) / iters)

    for np_value, values in grouped.items():
        phase_means[np_value] = {
            "k1": statistics.mean(values["k1"]) if values["k1"] else 0.0,
            "k4": statistics.mean(values["k4"]) if values["k4"] else 0.0,
            "k5": statistics.mean(values["k5"]) if values["k5"] else 0.0,
        }
    return phase_means


def save_speedup_plot(rows, out_dir: Path):
    np_values = [r["np"] for r in rows]
    speedup = [r.get("speedup", rows[0]["k0_mean"] / r["k0_mean"]) for r in rows]

    plt.figure(figsize=(7.0, 4.2))
    plt.plot(np_values, speedup, marker="o", linewidth=2.0, label="Mesure MPI1")
    plt.plot(np_values, np_values, linestyle="--", linewidth=1.8, label="Speedup ideal")
    plt.xlabel("Nombre de processus")
    plt.ylabel("Speedup S(p)")
    plt.title("Speedup MPI1 (SoA)")
    plt.grid(True, alpha=0.25)
    plt.xticks(np_values)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_dir / "mpi1_speedup.png", dpi=180)
    plt.close()


def save_efficiency_plot(rows, out_dir: Path):
    np_values = [r["np"] for r in rows]
    efficiency = [r.get("efficiency", r.get("speedup", rows[0]["k0_mean"] / r["k0_mean"]) / r["np"]) for r in rows]

    plt.figure(figsize=(7.0, 4.2))
    plt.plot(np_values, efficiency, marker="o", linewidth=2.0, label="Efficacite MPI1")
    plt.axhline(1.0, linestyle="--", linewidth=1.6, label="Efficacite ideale")
    plt.xlabel("Nombre de processus")
    plt.ylabel("Efficacite E(p)")
    plt.title("Efficacite MPI1 (SoA)")
    plt.grid(True, alpha=0.25)
    plt.xticks(np_values)
    plt.ylim(bottom=0.0)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_dir / "mpi1_efficiency.png", dpi=180)
    plt.close()


def save_k0_plot(rows, out_dir: Path):
    np_values = [r["np"] for r in rows]
    k0_mean = [r["k0_mean"] for r in rows]
    k0_std = [r["k0_std"] for r in rows]

    plt.figure(figsize=(7.0, 4.2))
    plt.errorbar(np_values, k0_mean, yerr=k0_std, marker="o", capsize=4, linewidth=2.0)
    plt.xlabel("Nombre de processus")
    plt.ylabel("K0 moyen (ms / itération)")
    plt.title("Temps noyau K0 vs nombre de processus (MPI1 SoA)")
    plt.grid(True, alpha=0.25)
    plt.xticks(np_values)
    plt.tight_layout()
    plt.savefig(out_dir / "mpi1_k0_ms_iter.png", dpi=180)
    plt.close()


def save_breakdown_plot(rows, phase_means, k_sync_by_np, out_dir: Path):
    np_values = [r["np"] for r in rows]
    k1 = [phase_means.get(np_value, {}).get("k1", r.get("k1_mean", 0.0)) for np_value, r in zip(np_values, rows)]
    k4 = [phase_means.get(np_value, {}).get("k4", r.get("k4_mean", 0.0)) for np_value, r in zip(np_values, rows)]
    k5 = [phase_means.get(np_value, {}).get("k5", r.get("k5_mean", 0.0)) for np_value, r in zip(np_values, rows)]
    k_sync = [k_sync_by_np.get(np_value, 0.0) for np_value in np_values]

    plt.figure(figsize=(7.6, 4.6))
    plt.bar(np_values, k1, label="K1 (ants update)")
    plt.bar(np_values, k4, bottom=k1, label="K4 (evaporation)")
    bottom_k1k4 = [a + b for a, b in zip(k1, k4)]
    plt.bar(np_values, k5, bottom=bottom_k1k4, label="K5 (update)")
    bottom_total = [a + b + c for a, b, c in zip(k1, k4, k5)]
    plt.bar(np_values, k_sync, bottom=bottom_total, label="K_MPI_sync")
    plt.xlabel("Nombre de processus")
    plt.ylabel("ms / itération")
    plt.title("Décomposition du noyau MPI1 (SoA)")
    plt.xticks(np_values)
    plt.grid(True, axis="y", alpha=0.25)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_dir / "mpi1_kernel_breakdown.png", dpi=180)
    plt.close()


def parse_args():
    parser = argparse.ArgumentParser(description="Generate MPI1 report figures from CSV/metrics.")
    parser.add_argument(
        "--mpi-dir",
        type=Path,
        default=DEFAULT_MPI_DIR,
        help=f"Path to MPI1 sweep folder (default: {DEFAULT_MPI_DIR})",
    )
    parser.add_argument(
        "--out-dir",
        type=Path,
        default=DEFAULT_OUT_DIR,
        help=f"Output folder for generated figures (default: {DEFAULT_OUT_DIR})",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    mpi_dir = args.mpi_dir
    out_dir = args.out_dir
    out_dir.mkdir(parents=True, exist_ok=True)

    summary_path = mpi_dir / "summary_speedup.csv"
    if not summary_path.exists():
        summary_path = mpi_dir / "summary_np.csv"
    rows = read_summary(summary_path)
    phase_means = read_phase_means(mpi_dir)
    k_sync_by_np = read_k_mpi_sync_means(mpi_dir)

    save_speedup_plot(rows, out_dir)
    save_efficiency_plot(rows, out_dir)
    save_k0_plot(rows, out_dir)
    save_breakdown_plot(rows, phase_means, k_sync_by_np, out_dir)

    print(f"Generated plots in: {out_dir}")


if __name__ == "__main__":
    main()
