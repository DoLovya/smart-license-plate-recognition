#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
VENV_DIR="${ROOT_DIR}/backend/.venv"
DEPS_MARKER="${VENV_DIR}/.deps_installed"
BACKEND_REQUIREMENTS_PATH="${ROOT_DIR}/backend/requirements.txt"
ALGORITHM_REQUIREMENTS_PATH="${ROOT_DIR}/algorithms/requirements.txt"

cd "${ROOT_DIR}/backend"

if [[ ! -d "${VENV_DIR}" ]]; then
  python3 -m venv "${VENV_DIR}"
fi

# shellcheck source=/dev/null
source "${VENV_DIR}/bin/activate"

if [[ ! -f "${DEPS_MARKER}" ]] \
  || [[ "${BACKEND_REQUIREMENTS_PATH}" -nt "${DEPS_MARKER}" ]] \
  || [[ "${ALGORITHM_REQUIREMENTS_PATH}" -nt "${DEPS_MARKER}" ]]; then
  echo "Installing backend dependencies (this may take several minutes on first run)..."
  pip install \
    -r "${BACKEND_REQUIREMENTS_PATH}" \
    -r "${ALGORITHM_REQUIREMENTS_PATH}" \
    -i https://pypi.tuna.tsinghua.edu.cn/simple
  touch "${DEPS_MARKER}"
  echo "Dependencies installed."
else
  echo "Dependencies already installed, skipping pip install."
fi

uvicorn app.main:app --host 0.0.0.0 --port 8000 --reload
