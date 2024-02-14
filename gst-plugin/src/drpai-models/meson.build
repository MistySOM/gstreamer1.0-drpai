gstdrpai_dep = declare_dependency(link_with : gstplugindrpai, include_directories : '..')

library('gstdrpai-yolo',
    'drpai_yolo.cpp',
    dependencies: [gstdrpai_dep, gst_dep],
    install : true,
)

opencv_dep = dependency('opencv4')
library('gstdrpai-deeppose',
    'drpai_deeppose.cpp',
    dependencies: [gstdrpai_dep, gst_dep, opencv_dep],
    install : true,
)
