plugin_c_args = ['-DHAVE_CONFIG_H']

cdata = configuration_data()
cdata.set_quoted('PACKAGE_VERSION', gst_version)
cdata.set_quoted('PACKAGE', 'gst-drpai-plugin')
cdata.set_quoted('GST_LICENSE', 'LGPL')
cdata.set_quoted('GST_API_VERSION', api_version)
cdata.set_quoted('GST_PACKAGE_NAME', 'GStreamer DRP-AI Plug-in')
cdata.set_quoted('GST_PACKAGE_ORIGIN', 'https://mistysom.com')
configure_file(output : 'config.h', configuration : cdata)

incdir = include_directories('src')

plugin_sources = [
  'src/gstdrpai.cpp',
  'src/drpai_controller.cpp',
  'src/drpai-models/drpai_base.cpp',
  'src/image.cpp',
  'src/box.cpp'
 ]

gstplugindrpai = library('gstdrpai',
  plugin_sources,
  c_args: plugin_c_args,
  cpp_args: plugin_c_args,
  dependencies : [gst_dep, thread_dep, dl_dep],
  install : true,
  include_directories: incdir,
  install_dir : join_paths(get_option('libdir'), 'gstreamer-1.0'),
)
gstdrpai_dep = declare_dependency(link_with : gstplugindrpai, include_directories : incdir)

gstdrpai_dep = declare_dependency(link_with : gstplugindrpai, include_directories : incdir)
enable_deeppose = get_option('enable-deeppose')

subdir('src/drpai-models/drpai-yolo')
if enable_deeppose
    subdir('src/drpai-models/drpai-deeppose')
endif
subdir('tests')
