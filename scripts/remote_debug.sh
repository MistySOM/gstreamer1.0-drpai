#!/bin/bash

set -e

source /workspace/.env

SOCK="/tmp/${MISTYSOM_USER}@${MISTYSOM_HOST}.ctl"
SSH_CMD="sshpass -e ssh -C \
  -o ControlMaster=auto \
  -o ControlPath=${SOCK} \
  -o ControlPersist=180 \
  -o StrictHostKeyChecking=no \
  -L 127.0.0.1:${GDB_PORT}:127.0.0.1:${GDB_PORT} \
  ${MISTYSOM_USER}@${MISTYSOM_HOST}"

cleanup() {
  ./scripts/remote_stop.sh
}
trap cleanup INT TERM

echo "Connecting to ${MISTYSOM_USER}@${MISTYSOM_HOST}..."

${SSH_CMD} << EOF
set -e
/home/root/v4l2-init.sh ${CAMERA_WIDTH}x${CAMERA_HEIGHT}
gdbserver :${GDB_PORT} /home/root/gst-launch "${PIPELINE}"
EOF
