#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BACKEND_URL="${BACKEND_URL:-http://127.0.0.1:8000}"
BACKEND_HEALTH_URL="${BACKEND_HEALTH_URL:-${BACKEND_URL}/api/v1/health}"
BACKEND_LOG_PATH="${ROOT_DIR}/backend/.backend.log"
QT_BUILD_DIR="${ROOT_DIR}/frontend/qt_client/build"
QT_BINARY_PATH="${QT_BUILD_DIR}/SmartLicensePlateQtClient"

backend_started_by_script=0
backend_pid=""

cleanup() {
  if [[ "${backend_started_by_script}" -eq 1 && -n "${backend_pid}" ]]; then
    echo "Stopping backend (pid=${backend_pid})..."
    kill "${backend_pid}" >/dev/null 2>&1 || true
    wait "${backend_pid}" 2>/dev/null || true
  fi
}

trap cleanup EXIT INT TERM

require_command() {
  local command_name="$1"
  if ! command -v "${command_name}" >/dev/null 2>&1; then
    echo "Missing required command: ${command_name}" >&2
    exit 1
  fi
}

wait_for_backend() {
  local attempts="${BACKEND_WAIT_ATTEMPTS:-300}"
  local i

  for ((i = 1; i <= attempts; i++)); do
    if curl --silent --fail "${BACKEND_HEALTH_URL}" >/dev/null 2>&1; then
      return 0
    fi
    sleep 1
  done

  return 1
}

ensure_backend() {
  echo "Checking backend at ${BACKEND_HEALTH_URL}..."

  if curl --silent --fail "${BACKEND_HEALTH_URL}" >/dev/null 2>&1; then
    echo "Backend already running."
    return 0
  fi

  echo "Starting backend (logs: ${BACKEND_LOG_PATH})..."
  (
    cd "${ROOT_DIR}"
    bash "${ROOT_DIR}/deploy/scripts/start_backend.sh"
  ) >"${BACKEND_LOG_PATH}" 2>&1 &
  backend_pid="$!"
  backend_started_by_script=1

  # Forward backend log to terminal so users can watch pip install progress.
  tail --pid="${backend_pid}" -n +1 -f "${BACKEND_LOG_PATH}" 2>/dev/null &
  local tail_pid=$!

  if wait_for_backend; then
    kill "${tail_pid}" >/dev/null 2>&1 || true
    echo "Backend is ready."
    return 0
  fi

  kill "${tail_pid}" >/dev/null 2>&1 || true
  echo "Backend failed to become ready. Check log: ${BACKEND_LOG_PATH}" >&2
  exit 1
}

build_qt_client() {
  echo "Configuring Qt client..."
  cmake -S "${ROOT_DIR}/frontend/qt_client" -B "${QT_BUILD_DIR}"

  echo "Building Qt client..."
  cmake --build "${QT_BUILD_DIR}"
}

launch_qt_client() {
  if [[ ! -x "${QT_BINARY_PATH}" ]]; then
    echo "Qt client binary not found: ${QT_BINARY_PATH}" >&2
    exit 1
  fi

  echo "Launching Qt client..."
  "${QT_BINARY_PATH}"
}

main() {
  require_command python3
  require_command cmake
  require_command curl

  ensure_backend
  build_qt_client

  echo "Backend docs: ${BACKEND_URL}/docs"
  launch_qt_client
}

main "$@"
