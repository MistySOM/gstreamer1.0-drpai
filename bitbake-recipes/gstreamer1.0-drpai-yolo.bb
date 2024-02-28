SUMMARY = "GStreamer DRP-AI plugin Yolo Library"
SECTION = "multimedia"
LICENSE = "MIT"
SRC_URI = "git://github.com/MistySOM/gstreamer1.0-drpai.git;branch=master"
SRCREV = "${AUTOREV}"
LIC_FILES_CHKSUM = "file://LICENSE.md;md5=546bb90dc9b7cbf2b99de6cc06051bf9"
MESON_BUILDTYPE = "release"
MESON_TARGET = "gstdrpai-yolo"

inherit meson

DEPENDS = "gstreamer1.0 gstreamer1.0-drpai"

S = "${WORKDIR}/git"
PV = "1.0"

PACKAGES = "${PN} ${PN}-dbg"
FILES_${PN} = "${libdir}"
FILES_${PN}-dbg = "${libdir}/.debug/"
RDEPENDS_${PN} = "gstreamer1.0 gstreamer1.0-drpai"
