gstdrpai_dep = declare_dependency(link_with : gstplugindrpai, include_directories : incdir)
gstdrpai_yolo_sources = [
    'drpai_yolo.cpp',
    'tracker.cpp',
    'detection_filterer.cpp'
]

gstdrpai_yolo = library('gstdrpai-yolo',
    gstdrpai_yolo_sources,
    include_directories: incdir,
    dependencies: [gstdrpai_dep, gst_dep],
    install : true,
)

opencv_dep = dependency('opencv4', required: false)
library('gstdrpai-deeppose',
    'drpai_deeppose.cpp',
    dependencies: [gstdrpai_dep, gst_dep, opencv_dep],
    install : true,
)
