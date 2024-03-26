opencv_dep = dependency('opencv4', required: true)

library('gstdrpai-deeppose',
    'drpai_deeppose.cpp',
    include_directories: incdir,
    dependencies: [gstdrpai_dep, gst_dep, opencv_dep],
    install : true,
)
