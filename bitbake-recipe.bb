SUMMARY = "GStreamer DRP-AI plugin"
SECTION = "multimedia"
LICENSE = "MIT"
SRC_URI = "git://github.com/MistySOM/gstreamer1.0-drpai.git;branch=master"
SRCREV = "${AUTOREV}"
LIC_FILES_CHKSUM = "file://LICENSE.md;md5=546bb90dc9b7cbf2b99de6cc06051bf9"
MESON_BUILDTYPE = "release"

inherit meson

DEPENDS = "gstreamer1.0 gstreamer1.0-plugins-base drpai opencv"

S = "${WORKDIR}/git"
PV = "1.0"

FILES_${PN} = "${libdir}/gstreamer-1.0/libgstdrpai.so"
FILES_${PN}-dev = "${libdir}/gstreamer-1.0/libgstdrpai.la"
FILES_${PN}-staticdev = "${libdir}/gstreamer-1.0/libgstdrpai.a"
FILES_${PN}-dbg = " \
    ${libdir}/gstreamer-1.0/.debug \
    ${prefix}/src"
RDEPENDS_${PN} = "gstreamer1.0 gstreamer1.0-plugins-base kernel-module-udmabuf"

# yolo library
PACKAGES += "${PN}-yolo"
FILES_${PN}-yolo = "${libdir}/libgstdrpai-yolo.so"
RDEPENDS_${PN} += "${PN}-yolo"
RDEPENDS_${PN}_yolo += "${PN}"

# deeppose library
PACKAGES += "${PN}-deeppose"
FILES_${PN}-deeppose = "${libdir}/libgstdrpai-deeppose.so"
RDEPENDS_${PN} += "${PN}-deeppose"
RDEPENDS_${PN}_deeppose += "${PN} ${PN}-yolo"

# install models
SRC_URI += "https://remote.mistywest.com/download/mh11/models.zip"
SRC_URI[sha256sum] = "1129004a8b222e058e34e3d0cc78f4ad93d195450e11d17fdda21282de2a3948"
do_install_append() {
    install -d ${D}${ROOT_HOME}
    cp -r ${WORKDIR}/models ${D}${ROOT_HOME}
}
PACKAGES += "${PN}-models ${PN}-models-yolov3 ${PN}-models-tinyyolov3 ${PN}-models-yolov2 ${PN}-models-tinyyolov2"
FILES_${PN}-models-yolov3 = "${ROOT_HOME}/models/yolov3"
FILES_${PN}-models-tinyyolov3 = "${ROOT_HOME}/models/tinyyolov3"
FILES_${PN}-models-yolov2 = "${ROOT_HOME}/models/yolov2"
FILES_${PN}-models-tinyyolov2 = "${ROOT_HOME}/models/tinyyolov2"
DEPENDS_${PN}-models = "${PN}-models-yolov3 ${PN}-models-tinyyolov3 ${PN}-models-yolov2 ${PN}-models-tinyyolov2"
# deeppose
PACKAGES += "${PN}-models-tinyyolov2-face ${PN}-models-deeppose"
FILES_${PN}-models-deeppose = "${ROOT_HOME}/models/deeppose"
FILES_${PN}-models-tinyyolov2-face = "${ROOT_HOME}/models/tinyyolov2_face"
DEPENDS_${PN}-models += "${PN}-models-deeppose ${PN}-models-tinyyolov2-face"
