#!/bin/bash

set -e

source /workspace/.env

sshpass -e sftp -Cq ${MISTYSOM_USER}@${MISTYSOM_HOST} << EOF
put /workspace/build/gst-app/gst-launch /home/root
put /workspace/build/gst-plugin/libgstdrpai.so /usr/lib64/gstreamer-1.0/
put /workspace/build/gst-plugin/src/drpai-models/drpai-yolo/libgstdrpai-yolo.so /usr/lib64/
put /workspace/build/gst-plugin/src/drpai-models/drpai-dummy/libgstdrpai-dummy.so /usr/lib64/
put /workspace/build/gst-plugin/src/drpai-models/drpai-mobilenet/libgstdrpai-mobilenet.so /usr/lib64/
put /workspace/gst-plugin/src/drivers/drpai-tvm/rzv_drp-ai_tvm/obj/build_runtime/V2L/libtvm_runtime.so /usr/lib64/
EOF
