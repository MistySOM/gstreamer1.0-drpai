plugin_c_args = ['-DHAVE_CONFIG_H']

cdata = configuration_data()
cdata.set_quoted('PACKAGE_VERSION', gst_version)
cdata.set_quoted('PACKAGE', 'gst-drpai-plugin')
cdata.set_quoted('GST_LICENSE', 'LGPL')
cdata.set_quoted('GST_API_VERSION', api_version)
cdata.set_quoted('GST_PACKAGE_NAME', 'GStreamer DRP-AI Plug-in')
cdata.set_quoted('GST_PACKAGE_ORIGIN', 'https://mistysom.com')
cdata.set('HAVE_MMNGR', enable_mmngr.enabled())
configure_file(output: 'config.h', configuration: cdata)

incdir = include_directories('src')

plugin_sources = [
  'src/gstdrpai.cpp',
  'src/gst_udma_buffer_pool.cpp',
  'src/drpai_controller.cpp',
  'src/tracker.cpp',
  'src/filterer.cpp',
  'src/drivers/dmabuf.cpp',
  'src/drivers/drpai_native.cpp',
  'src/drpai-models/base_post_processor.cpp',
  'src/image.cpp',
  'src/box.cpp',
]

plugin_dependencies = [gst_dep, thread_dep, dl_dep, mmngr_dep, mmngrbuf_dep]

subdir('src/drivers/drpai-tvm')

gstplugindrpai = library(
  'gstdrpai',
  plugin_sources,
  c_args: plugin_c_args,
  cpp_args: plugin_c_args,
  dependencies: plugin_dependencies,
  install: true,
  include_directories: incdir,
  install_dir: join_paths(get_option('libdir'), 'gstreamer-1.0'),
)
gstdrpai_dep = declare_dependency(link_with: gstplugindrpai, include_directories: incdir)

subdir('src/drpai-models/drpai-yolo')
subdir('src/drpai-models/drpai-dummy')
subdir('src/drpai-models/drpai-mobilenet')
subdir('tests')