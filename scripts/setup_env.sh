#!/bin/bash

set -e

POKY_INSTALL_ZIP_URL=https://remote.mistywest.com/Download/mh11/rzv2l/poky-mistysom-${POKY_VERSION}.zip

if [[ ! -f /opt/poky/${POKY_VERSION}/environment-setup-aarch64-poky-linux ]]; then
    wget -c ${POKY_INSTALL_ZIP_URL} -O /tmp/poky-mistysom-${POKY_VERSION}.zip || :
    unzip -o /tmp/poky-mistysom-${POKY_VERSION}.zip -d /tmp
    chmod a+x /tmp/*.sh
    sudo /tmp/*.sh -y -d /opt/poky/${POKY_VERSION}
    rm -rf /tmp/poky-*
fi

source /opt/poky/${POKY_VERSION}/environment-setup-aarch64-poky-linux
POKY_PYTHON="${OECORE_NATIVE_SYSROOT}/usr/bin/python3"
POKY_PIP="${POKY_PYTHON} -m pip"

sudo ${POKY_PYTHON} -m ensurepip
sudo ${POKY_PIP} install pip --upgrade --root-user-action=ignore
sudo ${POKY_PIP} install meson --upgrade --root-user-action=ignore

sudo ln -s ${OECORE_NATIVE_SYSROOT}/usr/bin/meson /usr/bin/meson

git config --global --add safe.directory /workspace/gst-plugin/src/drivers/drpai-tvm/rzv_drp-ai_tvm
git submodule update --init --recursive --depth=1
