#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.10"
# dependencies = ["matplotlib"]
# ///
"""Generate the lab throughput chart."""

from pathlib import Path

import matplotlib.pyplot as plt


THREADS = [1, 2, 4, 8, 16]
DATA = {
    "Baseline": [415.335, None, None, None, None],
    "Empty lock": [117.372, 60.733, 49.6068, 35.8053, 31.3645],
    "Global lock": [104.854, 47.3898, 36.5972, 27.697, 20.1725],
    "Striped lock": [94.5216, 27.6585, 24.6077, 21.4375, 15.1389],
    "Thread-local": [242.301, 462.07, 789.379, 937.586, 969.911],
    "Double buffer": [217.614, 416.612, 665.391, 832.971, 792.533],
}


def main() -> None:
    output = Path(__file__).resolve().with_name("results.png")

    fig, ax = plt.subplots(figsize=(11, 6.5))
    for name, values in DATA.items():
        x = [thread_count for thread_count, value in zip(THREADS, values) if value is not None]
        y = [value for value in values if value is not None]
        ax.plot(x, y, marker="o", linewidth=2, label=name)

    ax.set_title("Metrics collector throughput")
    ax.set_xlabel("Writer threads")
    ax.set_ylabel("Million record() calls / sec")
    ax.set_xticks(THREADS)
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.tight_layout()
    fig.savefig(output, dpi=160)
    print(output)


if __name__ == "__main__":
    main()
