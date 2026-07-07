#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

cd "${ROOT_DIR}"
python algorithms/inference/run_inference.py --image "${1:-data/raw/demo.jpg}"
