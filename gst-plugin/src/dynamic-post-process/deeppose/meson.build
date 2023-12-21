deeppose_sources = [
    'deeppose.cpp'
]

opencv = dependency('opencv4')

deeppose = library('postprocess-deeppose',
    deeppose_sources,
    cpp_args: ['-DPOST_PROCESS_LIBRARY'],
    dependencies: [opencv],
    install : true,
    install_dir : '/usr/lib64',
)
