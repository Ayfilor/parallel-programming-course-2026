#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.10"
# dependencies = ["numpy"]
# ///
import numpy as np
from collections import Counter 

A = 1.15
N = 2**20
K = 1024
SEED = 22

def main() -> None:
    rng = np.random.default_rng(SEED)
    values = rng.zipf(A, size=N)

    # Normalize generated numbers to [0; 1] range
    values = (values - 1) % K

    # Get list of frequencies: keys are numbers, values are frequencies 
    frequencies = Counter(values)
    frequencies = [item[1] for item in sorted(frequencies.items())]

    assert sum(frequencies) == N

    head = sum(frequencies[:3]) / N
    tail = sum(frequencies[25:]) / N

    print(f"First 3 buckets (0-12 ms): {head:.2f}")
    print(f"Buckets after 25th (100+ ms): {tail:.2f}")
    print(frequencies[-10:])

if __name__ == "__main__":
    main()
