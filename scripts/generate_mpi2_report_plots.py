#!/usr/bin/env python3
"""Generate MPI2 report figures from benchmark CSV outputs."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path

import matplotlib.pyplot as plt


REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_MPI2_DIR = REPO_ROOT / "results" / "test_mpi2_report_v1" / "mpi2" / "soa" / "latest"
DEFAULT_OUT_DIR = REPO_ROOT / "docs" / "report" / "imgs" / "mpi2"


def read_summary(path: Path):
    rows = []
    with path.open(newline="") as f:
        for row in csv.DictReader(f):
            rows.append(
                {
                    "np": int(row["np"]),
                    "k0_mean": float(row["mean_k0_ms_iter"]),
                    "k0_std": float(row["std_k0_ms_iter"]),
                    "k1_mean": float(row["mean_k1_ms_iter"]),
                    "k4_mean": float(row["mean_k4_ms_iter"]),
                    "k5_mean": float(row["mean_k5_ms_iter"]),
                    "k_sync_mean": float(row["mean_k_mpi_sync_ms_iter"]),
                    "k_halo_mean": float(row["mean_k_mpi_halo_ms_iter"]),
                    "k_migrate_mean": float(row["mean_k_mpi_migrate_ms_iter"]),
                }
            )
    rows.sort(key=lambda r: r["np"])
    if not rows:
        raise ValueError(f"No data rows found in: {path}")
    return rows


def compute_speedup_efficiency(rows):
    base_k0 = rows[0]["k0_mean"]
    for row in rows:
        row["speedup"] = base_k0 / row["k0_mean"] if row["k0_mean"] > 0.0 else 0.0
        row["efficiency"] = row["speedup"] / row["np"] if row["np"] > 0 else 0.0
    return rows


def save_speedup_plot(rows, out_dir: Path):
    np_values = [r["np"] for r in rows]
    speedup = [r["speedup"] for r in rows]

    plt.figure(figsize=(7.0, 4.2))
    plt.plot(np_values, speedup, marker="o", linewidth=2.0, label="Mesure MPI2")
    plt.plot(np_values, np_values, linestyle="--", linewidth=1.8, label="Speedup ideal")
    plt.xlabel("Nombre de processus")
    plt.ylabel("Speedup S(p)")
    plt.title("Speedup MPI2 (SoA)")
    plt.grid(True, alpha=0.25)
    plt.xticks(np_values)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_dir / "mpi2_speedup.png", dpi=180)
    plt.close()


def save_efficiency_plot(rows, out_dir: Path):
    np_values = [r["np"] for r in rows]
    efficiency = [r["efficiency"] for r in rows]

    plt.figure(figsize=(7.0, 4.2))
    plt.plot(np_values, efficiency, marker="o", linewidth=2.0, label="Efficacite MPI2")
    plt.axhline(1.0, linestyle="--", linewidth=1.6, label="Efficacite ideale")
    plt.xlabel("Nombre de processus")
    plt.ylabel("Efficacite E(p)")
    plt.title("Efficacite MPI2 (SoA)")
    plt.grid(True, alpha=0.25)
    plt.xticks(np_values)
    plt.ylim(bottom=0.0)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_dir / "mpi2_efficiency.png", dpi=180)
    plt.close()


def save_k0_plot(rows, out_dir: Path):
    np_values = [r["np"] for r in rows]
    k0_mean = [r["k0_mean"] for r in rows]
    k0_std = [r["k0_std"] for r in rows]

    plt.figure(figsize=(7.0, 4.2))
    plt.errorbar(np_values, k0_mean, yerr=k0_std, marker="o", capsize=4, linewidth=2.0)
    plt.xlabel("Nombre de processus")
    plt.ylabel("K0 moyen (ms / itération)")
    plt.title("Temps noyau K0 vs nombre de processus (MPI2 SoA)")
    plt.grid(True, alpha=0.25)
    plt.xticks(np_values)
    plt.tight_layout()
    plt.savefig(out_dir / "mpi2_k0_ms_iter.png", dpi=180)
    plt.close()


def save_breakdown_plot(rows, out_dir: Path):
    np_values = [r["np"] for r in rows]
    k1 = [r["k1_mean"] for r in rows]
    k4 = [r["k4_mean"] for r in rows]
    k5 = [r["k5_mean"] for r in rows]
    k_sync = [r["k_sync_mean"] for r in rows]
    k_halo = [r["k_halo_mean"] for r in rows]
    k_migrate = [r["k_migrate_mean"] for r in rows]

    plt.figure(figsize=(8.2, 4.8))
    plt.bar(np_values, k1, label="K1 (ants update)")
    plt.bar(np_values, k4, bottom=k1, label="K4 (evaporation)")
    bottom1 = [a + b for a, b in zip(k1, k4)]
    plt.bar(np_values, k5, bottom=bottom1, label="K5 (update)")
    bottom2 = [a + b for a, b in zip(bottom1, k5)]
    plt.bar(np_values, k_sync, bottom=bottom2, label="K_MPI_sync")
    bottom3 = [a + b for a, b in zip(bottom2, k_sync)]
    plt.bar(np_values, k_halo, bottom=bottom3, label="K_MPI_halo")
    bottom4 = [a + b for a, b in zip(bottom3, k_halo)]
    plt.bar(np_values, k_migrate, bottom=bottom4, label="K_MPI_migrate")
    plt.xlabel("Nombre de processus")
    plt.ylabel("ms / itération")
    plt.title("Décomposition du noyau MPI2 (SoA)")
    plt.xticks(np_values)
    plt.grid(True, axis="y", alpha=0.25)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_dir / "mpi2_kernel_breakdown.png", dpi=180)
    plt.close()


def parse_args():
    parser = argparse.ArgumentParser(description="Generate MPI2 report figures from CSV metrics.")
    parser.add_argument(
        "--mpi2-dir",
        type=Path,
        default=DEFAULT_MPI2_DIR,
        help=f"Path to MPI2 sweep folder (default: {DEFAULT_MPI2_DIR})",
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
    mpi2_dir = args.mpi2_dir
    out_dir = args.out_dir
    out_dir.mkdir(parents=True, exist_ok=True)

    summary_path = mpi2_dir / "summary_np.csv"
    rows = read_summary(summary_path)
    rows = compute_speedup_efficiency(rows)

    save_speedup_plot(rows, out_dir)
    save_efficiency_plot(rows, out_dir)
    save_k0_plot(rows, out_dir)
    save_breakdown_plot(rows, out_dir)

    print(f"Generated plots in: {out_dir}")


if __name__ == "__main__":
    main()
