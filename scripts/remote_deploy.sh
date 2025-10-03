#!/bin/bash

set -e

source /workspace/.env

DESTDIR=/opt/poky/3.1.31/sysroots/aarch64-poky-linux
sudo meson install --quiet --destdir ${DESTDIR} -C build

SOCK="/tmp/${MISTYSOM_USER}@${MISTYSOM_HOST}.ctl"

# 1) Open a single persistent SSH control connection
echo "Connecting to ${MISTYSOM_USER}@${MISTYSOM_HOST}..."
sshpass -e ssh -MNf \
  -o StrictHostKeyChecking=no \
  -o ControlMaster=auto \
  -o ControlPath="${SOCK}" \
  -o ControlPersist=180 \
  ${MISTYSOM_USER}@${MISTYSOM_HOST}

# 2) Define a base rsync command that reuses the same connection
RSYNC_SSH="ssh -o StrictHostKeyChecking=no -o ControlMaster=auto -o ControlPath=${SOCK} -o ControlPersist=180"
RSYNC="sshpass -e rsync -a --checksum --human-readable --info=name2,skip,copy,progress1 -e \"$RSYNC_SSH\""

# 3) Push files (rsync will SKIP identical files thanks to --checksum)

# /home/root
eval $RSYNC "/workspace/build/gst-app/gst-launch" \
  "${MISTYSOM_USER}@${MISTYSOM_HOST}:/home/root/"

# /usr/lib64/gstreamer-1.0/
eval $RSYNC "/workspace/build/gst-plugin/libgstdrpai.so" \
  "${MISTYSOM_USER}@${MISTYSOM_HOST}:/usr/lib64/gstreamer-1.0/"

# /usr/lib64/
eval $RSYNC "/workspace/build/gst-plugin/src/drpai-models/drpai-yolo/libgstdrpai-yolo.so" \
  "${MISTYSOM_USER}@${MISTYSOM_HOST}:/usr/lib64/"
eval $RSYNC "/workspace/build/gst-plugin/src/drpai-models/drpai-dummy/libgstdrpai-dummy.so" \
  "${MISTYSOM_USER}@${MISTYSOM_HOST}:/usr/lib64/"
eval $RSYNC "/workspace/build/gst-plugin/src/drpai-models/drpai-mobilenet/libgstdrpai-mobilenet.so" \
  "${MISTYSOM_USER}@${MISTYSOM_HOST}:/usr/lib64/"
eval $RSYNC "/workspace/build/gst-plugin/src/drpai-models/drpai-ssdv3/libgstdrpai-ssdv3.so" \
  "${MISTYSOM_USER}@${MISTYSOM_HOST}:/usr/lib64/"
eval $RSYNC "/workspace/gst-plugin/src/drivers/drpai-tvm/rzv_drp-ai_tvm/obj/build_runtime/V2L/libtvm_runtime.so" \
  "${MISTYSOM_USER}@${MISTYSOM_HOST}:/usr/lib64/"

echo