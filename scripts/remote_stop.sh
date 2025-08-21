#!/bin/bash

set -e

source /workspace/.env

SOCK="/tmp/${MISTYSOM_USER}@${MISTYSOM_HOST}.ctl"
SSH_CMD="sshpass -e ssh \
  -o ControlMaster=auto \
  -o ControlPath=${SOCK} \
  -o ControlPersist=180 \
  -o StrictHostKeyChecking=no ${MISTYSOM_USER}@${MISTYSOM_HOST}"

echo -e "\nTerminating Process..."
${SSH_CMD} "pkill -INT -f gst-launch || true"
echo
