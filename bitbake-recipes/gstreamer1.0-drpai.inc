SECTION = "multimedia"
LICENSE = "MIT"
SRC_URI = "git://github.com/MistySOM/gstreamer1.0-drpai.git;branch=master"
SRCREV = "${AUTOREV}"
LIC_FILES_CHKSUM = "file://LICENSE.md;md5=546bb90dc9b7cbf2b99de6cc06051bf9"
DEPENDS = "gstreamer1.0 drpai"

inherit meson
MESON_BUILDTYPE = "release"
MESON_TARGET ?= ""

do_patch_append() {
    f = f"{d.getVar('S')}/gst-plugin/src/linux/drpai.h"
    if os.path.exists(f):
      os.remove(f)
}

do_compile() {
    meson compile -v ${PARALLEL_MAKE} ${MESON_TARGET}
}

do_install_append() {
    find ${D}${libdir} -type f -not -name "lib${MESON_TARGET}.so" -delete
    find ${D}${libdir} -type d -empty -delete

    find ${D}${bindir} -type f -not -name "${MESON_TARGET}" -delete
    find ${D}${bindir} -type d -empty -delete
}

S = "${WORKDIR}/git"
PV = "1.0"

PACKAGES = "${PN} ${PN}-dbg"
