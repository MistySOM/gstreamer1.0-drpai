gstdrpai_yolo_sources = [
    'drpai_tvm.cpp',
    'rzv_drp-ai_tvm/apps/PreRuntime.cpp',
    'rzv_drp-ai_tvm/apps/MeraDrpRuntimeWrapper.cpp',
]

message('Merging TVM include files with DRPAI extensions...')
run_command('bash', '-c', 'cp -r ' +
            meson.project_source_root() / meson.current_source_dir() / 'rzv_drp-ai_tvm/tvm/include/ ' +
            meson.current_build_dir() / 'extended_tvm && cp ' +
            meson.project_source_root() / meson.current_source_dir() / 'rzv_drp-ai_tvm/setup/include/* ' +
            meson.current_build_dir() / 'extended_tvm/tvm/runtime/',
            check:true)

libtvm_runtime_path = meson.project_source_root() / meson.current_source_dir() / 'rzv_drp-ai_tvm/obj/build_runtime/V2L/libtvm_runtime.so'
libtvm_runtime_dep = declare_dependency(
    link_args: libtvm_runtime_path,
    include_directories: [
        include_directories('rzv_drp-ai_tvm/tvm/3rdparty/dlpack/include', is_system: true),
        include_directories('rzv_drp-ai_tvm/tvm/3rdparty/dmlc-core/include', is_system: true),
        include_directories('rzv_drp-ai_tvm/tvm/3rdparty/compiler-rt', is_system: true),
        include_directories('extended_tvm', is_system: true),
        include_directories('rzv_drp-ai_tvm/apps', is_system: true),
    ]
)

gstdrpai_tvm = library('gstdrpai-tvm',
    gstdrpai_yolo_sources,
    include_directories: incdir,
    dependencies: [gstdrpai_dep, gst_dep, libtvm_runtime_dep],
    install : true,
)
