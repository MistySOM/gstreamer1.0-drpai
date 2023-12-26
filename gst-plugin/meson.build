plugin_c_args = ['-DHAVE_CONFIG_H']

cdata = configuration_data()
cdata.set_quoted('PACKAGE_VERSION', gst_version)
cdata.set_quoted('PACKAGE', 'gst-drpai-plugin')
cdata.set_quoted('GST_LICENSE', 'LGPL')
cdata.set_quoted('GST_API_VERSION', api_version)
cdata.set_quoted('GST_PACKAGE_NAME', 'GStreamer DRP-AI Plug-in')
cdata.set_quoted('GST_PACKAGE_ORIGIN', 'https://mistysom.com')
configure_file(output : 'config.h', configuration : cdata)

plugin_sources = [
  'src/gstdrpai.cpp',
  'src/drpai_controller.cpp',
  'src/drpai-models/drpai_connection.cpp',
  'src/drpai-models/drpai_yolo.cpp',
  'src/image.cpp',
  'src/box.cpp',
  'src/tracker.cpp',
  'src/dynamic-post-process/postprocess.cpp'
 ]

gstplugindrpai = library('gstdrpai',
  plugin_sources,
  c_args: plugin_c_args,
  cpp_args: plugin_c_args,
  dependencies : [gst_dep, thread_dep, dl_dep],
  install : true,
  install_dir : join_paths('/usr/lib64', 'gstreamer-1.0'),
)

subdir('tests')
subdir('src/dynamic-post-process/yolo')
