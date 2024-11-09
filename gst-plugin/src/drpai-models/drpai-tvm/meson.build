
enable_tvm = get_option('tvm')

libtvm_runtime_dir = meson.current_source_dir() / 'rzv_drp-ai_tvm/obj/build_runtime/V2L/'

libtvm_runtime_lib = cxx.find_library(
    'tvm_runtime',
    required: enable_tvm,
    dirs: [libtvm_runtime_dir],
)

if (enable_tvm.enabled())
    install_subdir(libtvm_runtime_dir, install_dir: '/usr/lib64')

    message('Merging TVM include files with DRPAI extensions...')
    run_command('bash', '-c', 'cp -r ' +
                meson.current_source_dir() / 'rzv_drp-ai_tvm/tvm/include ' +
                meson.current_build_dir() / 'extended_tvm && cp ' +
                meson.current_source_dir() / 'rzv_drp-ai_tvm/setup/include/* ' +
                meson.current_build_dir() / 'extended_tvm/tvm/runtime/',
                check:true)

    plugin_dependencies += declare_dependency(
        dependencies: libtvm_runtime_lib,
        include_directories: [
            include_directories('rzv_drp-ai_tvm/tvm/3rdparty/dlpack/include', is_system: true),
            include_directories('rzv_drp-ai_tvm/tvm/3rdparty/dmlc-core/include', is_system: true),
            include_directories('rzv_drp-ai_tvm/tvm/3rdparty/compiler-rt', is_system: true),
            include_directories('extended_tvm', is_system: true),
            include_directories('rzv_drp-ai_tvm/apps', is_system: true),
        ]
    )

    plugin_sources += files(
        'tvm_drpai.cpp',
        'rzv_drp-ai_tvm/apps/PreRuntime.cpp',
        'rzv_drp-ai_tvm/apps/MeraDrpRuntimeWrapper.cpp',
    )

    plugin_c_args += '-DENABLE_TVM'
endif
