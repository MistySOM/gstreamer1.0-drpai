yolov_sources = [
    'drpai_yolo.cpp'
]

gstdrpai_dep = declare_dependency(link_with : gstplugindrpai,
                            include_directories : '..')

yolov = library('gstdrpai-yolo',
    yolov_sources,
    dependencies: [gstdrpai_dep],
    install : true,
)