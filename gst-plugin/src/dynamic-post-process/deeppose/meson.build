deeppose_sources = [
    'deeppose.cpp'
]

deeppose = library('postprocess-deeppose',
    deeppose_sources,
    cpp_args: ['-DPOST_PROCESS_LIBRARY'],
    dependencies: [opencv_dep],
    install : true,
    install_dir : '/usr/lib64',
)
