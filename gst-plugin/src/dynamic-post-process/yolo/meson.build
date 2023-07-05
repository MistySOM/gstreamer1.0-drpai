yolov_sources = [
    'yolo.cpp'
]

yolov = library('postprocess-yolo',
    yolov_sources,
    cpp_args: ['-DPOST_PROCESS_LIBRARY'],
    dependencies: [],
    install : true,
    install_dir : '/usr/lib64',
)