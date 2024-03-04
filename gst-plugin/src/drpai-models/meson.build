gstdrpai_dep = declare_dependency(link_with : gstplugindrpai, include_directories : '..')

library('gstdrpai-yolo',
    'drpai_yolo.cpp',
    include_directories: incdir,
    dependencies: [gstdrpai_dep, gst_dep],
    install : true,
)