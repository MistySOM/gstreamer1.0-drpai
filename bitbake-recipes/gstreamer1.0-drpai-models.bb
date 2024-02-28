SUMMARY = "GStreamer DRP-AI plugin models"
SECTION = "multimedia"
LICENSE = "closed"

PV = "1.0"

RDEPENDS_${PN} = "gstreamer1.0-drpai"

SRC_URI += "https://remote.mistywest.com/download/mh11/models.zip"
SRC_URI[sha256sum] = "1129004a8b222e058e34e3d0cc78f4ad93d195450e11d17fdda21282de2a3948"
do_install_append() {
    install -d ${D}${ROOT_HOME}
    cp -r ${WORKDIR}/models ${D}${ROOT_HOME}
}
PACKAGES = "${PN}-yolov3 ${PN}-tinyyolov3 ${PN}-yolov2 ${PN}-tinyyolov2"
FILES_${PN}-yolov3 = "${ROOT_HOME}/models/yolov3"
FILES_${PN}-tinyyolov3 = "${ROOT_HOME}/models/tinyyolov3"
FILES_${PN}-yolov2 = "${ROOT_HOME}/models/yolov2"
FILES_${PN}-tinyyolov2 = "${ROOT_HOME}/models/tinyyolov2"
# deeppose
PACKAGES += "${PN}-tinyyolov2-face ${PN}-deeppose"
FILES_${PN}-deeppose = "${ROOT_HOME}/models/deeppose"
FILES_${PN}-tinyyolov2-face = "${ROOT_HOME}/models/tinyyolov2_face"
