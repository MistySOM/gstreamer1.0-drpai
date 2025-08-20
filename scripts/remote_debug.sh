#!/bin/bash

set -e

source /workspace/.env

sshpass -e ssh -C -L 127.0.0.1:${GDB_PORT}:127.0.0.1:${GDB_PORT} ${MISTYSOM_USER}@${MISTYSOM_HOST} << EOF
/home/root/v4l2-init.sh ${CAMERA_WIDTH}x${CAMERA_HEIGHT}
gdbserver :${GDB_PORT} /home/root/gst-launch "${PIPELINE}"
EOF
