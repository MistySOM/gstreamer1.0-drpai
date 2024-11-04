#message('Merging TVM include files with DRPAI extensions...')
#run_command('bash', '-c', 'cp -r ' +
#            meson.project_source_root() / meson.current_source_dir() / 'rzv_drp-ai_tvm/tvm/include ' +
#            meson.current_build_dir() / 'extended_tvm && cp ' +
#            meson.project_source_root() / meson.current_source_dir() / 'rzv_drp-ai_tvm/setup/include/* ' +
#            meson.current_build_dir() / 'extended_tvm/tvm/runtime/',
#            check:true)

libtvm_runtime_lib = cxx.find_library(
    'tvm_runtime',
    dirs: [meson.project_source_root() / meson.current_source_dir() /
           'rzv_drp-ai_tvm/obj/build_runtime/V2L/']
)

libtvm_runtime_dep = declare_dependency(
    dependencies: libtvm_runtime_lib,
    include_directories: [
        include_directories('rzv_drp-ai_tvm/tvm/3rdparty/dlpack/include', is_system: true),
        include_directories('rzv_drp-ai_tvm/tvm/3rdparty/dmlc-core/include', is_system: true),
        include_directories('rzv_drp-ai_tvm/tvm/3rdparty/compiler-rt', is_system: true),
        include_directories('rzv_drp-ai_tvm/tvm', is_system: true),
        include_directories('rzv_drp-ai_tvm/apps', is_system: true),
    ]
)

gstdrpai_tvm_sources = files(
    'tvm_drpai.cpp',
    'rzv_drp-ai_tvm/apps/PreRuntime.cpp',
    'rzv_drp-ai_tvm/apps/MeraDrpRuntimeWrapper.cpp',
)
