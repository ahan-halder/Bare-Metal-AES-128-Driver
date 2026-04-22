#!/usr/bin/env python3
"""Render benchmark and quality charts from sim/metrics.csv."""

import csv
import os
import sys
from collections import defaultdict

import matplotlib.pyplot as plt


METRICS_CSV = os.path.join("sim", "metrics.csv")
OUTPUT_PNG = os.path.join("sim", "benchmark_charts.png")


def read_metrics(path: str):
    rows = []
    with open(path, "r", encoding="utf-8") as fp:
        reader = csv.DictReader(fp)
        for row in reader:
            rows.append(
                {
                    "test_id": int(row["test_id"]),
                    "mode": row["mode"],
                    "description": row["description"],
                    "elapsed_us": float(row["elapsed_us"]),
                    "throughput_mbps": float(row["throughput_mbps"]),
                    "mismatched_bytes": int(row["mismatched_bytes"]),
                    "bit_errors": int(row["bit_errors"]),
                    "passed": int(row["passed"]),
                }
            )
    return rows


def avg(values):
    return sum(values) / len(values) if values else 0.0


def group_by_mode(metrics):
    grouped = defaultdict(list)
    for m in metrics:
        grouped[m["mode"]].append(m)
    return grouped


def build_charts(metrics, out_path: str):
    grouped = group_by_mode(metrics)

    enc = grouped.get("enc", [])
    dec = grouped.get("dec", [])

    enc_avg_latency = avg([m["elapsed_us"] for m in enc])
    dec_avg_latency = avg([m["elapsed_us"] for m in dec])
    enc_avg_tp = avg([m["throughput_mbps"] for m in enc])
    dec_avg_tp = avg([m["throughput_mbps"] for m in dec])

    labels = [f"{m['mode'].upper()}-{m['test_id']}" for m in metrics]
    latencies = [m["elapsed_us"] for m in metrics]
    mismatches = [m["mismatched_bytes"] for m in metrics]

    fig, axes = plt.subplots(2, 2, figsize=(12, 8))
    fig.suptitle("AES Driver Testbench Metrics", fontsize=14, fontweight="bold")

    axes[0, 0].bar(["Encrypt", "Decrypt"], [enc_avg_latency, dec_avg_latency], color=["#0081a7", "#f07167"])
    axes[0, 0].set_title("Average Latency (us)")
    axes[0, 0].set_ylabel("microseconds")

    axes[0, 1].bar(["Encrypt", "Decrypt"], [enc_avg_tp, dec_avg_tp], color=["#00afb9", "#fcbf49"])
    axes[0, 1].set_title("Average Throughput (Mbps)")
    axes[0, 1].set_ylabel("Mbps")

    axes[1, 0].bar(labels, latencies, color="#3d5a80")
    axes[1, 0].set_title("Per-Test Latency")
    axes[1, 0].set_ylabel("microseconds")
    axes[1, 0].tick_params(axis="x", rotation=45)

    axes[1, 1].bar(labels, mismatches, color="#ef476f")
    axes[1, 1].set_title("Per-Test Byte Mismatches")
    axes[1, 1].set_ylabel("mismatched bytes")
    axes[1, 1].tick_params(axis="x", rotation=45)

    for ax in axes.flat:
        ax.grid(axis="y", linestyle="--", alpha=0.3)

    fig.tight_layout(rect=[0, 0, 1, 0.96])
    fig.savefig(out_path, dpi=180)
    plt.close(fig)


def main():
    in_path = METRICS_CSV
    if len(sys.argv) > 1:
        in_path = sys.argv[1]

    if not os.path.exists(in_path):
        print(f"[ERROR] Metrics file not found: {in_path}")
        print("Run the C testbench first so sim/metrics.csv is created.")
        return 1

    metrics = read_metrics(in_path)
    if not metrics:
        print("[ERROR] Metrics file is empty.")
        return 1

    os.makedirs(os.path.dirname(OUTPUT_PNG), exist_ok=True)
    build_charts(metrics, OUTPUT_PNG)
    print(f"[OK] Wrote chart image: {OUTPUT_PNG}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
