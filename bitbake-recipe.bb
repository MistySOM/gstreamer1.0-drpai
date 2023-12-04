SUMMARY = "GStreamer DRP-AI plugin"
SECTION = "multimedia"
LICENSE = "MIT"
SRC_URI = "git://github.com/MistySOM/gstreamer1.0-drpai.git;branch=master"
SRCREV = "${AUTOREV}"
LIC_FILES_CHKSUM = "file://LICENSE.md;md5=546bb90dc9b7cbf2b99de6cc06051bf9"
MESON_BUILDTYPE = "release"

inherit meson

DEPENDS = "gstreamer1.0 gstreamer1.0-plugins-base drpai"

S = "${WORKDIR}/git"
PV = "1.0"

FILES_${PN} = "${libdir}/gstreamer-1.0/libgstdrpai.so"
FILES_${PN}-dev = "${libdir}/gstreamer-1.0/libgstdrpai.la"
FILES_${PN}-staticdev = "${libdir}/gstreamer-1.0/libgstdrpai.a"
FILES_${PN}-dbg = " \
    ${libdir}/gstreamer-1.0/.debug \
    ${prefix}/src"
RDEPENDS_${PN} = "gstreamer1.0 gstreamer1.0-plugins-base kernel-module-udmabuf"



PACKAGES += "${PN}-postprocess-yolo"
FILES_${PN}-postprocess-yolo = "${libdir}/libpostprocess-yolo.so"
RDEPENDS_${PN} += "${PN}-postprocess-yolo"



SRC_URI += "https://remote.mistywest.com/download/mh11/models.zip"
SRC_URI[sha256sum] = "80215345f43e0e565d3a95f86933e96773ad9bf3cc03a2d8c2ecfe0803995a93"
do_install_append() {
    install -d ${D}${ROOT_HOME}
    cp -r ${WORKDIR}/models ${D}${ROOT_HOME}
}
PACKAGES += "${PN}-models ${PN}-models-yolov3 ${PN}-models-tinyyolov3 ${PN}-models-yolov2 ${PN}-models-tinyyolov2"
FILES_${PN}-models-yolov3 = "${ROOT_HOME}/models/yolov3"
FILES_${PN}-models-tinyyolov3 = "${ROOT_HOME}/models/tinyyolov3"
FILES_${PN}-models-yolov2 = "${ROOT_HOME}/models/yolov2"
FILES_${PN}-models-tinyyolov2 = "${ROOT_HOME}/models/tinyyolov2"
RDEPENDS_${PN}-models = "${PN}-models-yolov3 ${PN}-models-tinyyolov3 ${PN}-models-yolov2 ${PN}-models-tinyyolov2"
