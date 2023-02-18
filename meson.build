project('gst-drpai', 'c', version : '1.19.0.1', license : 'LGPL')

plugins_install_dir = join_paths(get_options('libdir'), 'gstreamer-1.0')

cc = meson.get_compiler('c')

gst_version = meson.project_version()

api_version = '1.0'

gst_dep = dependency('gstreamer-1.0', version : '>=1.16.3',
    required : true, fallback : ['gstreamer', 'gst_dep'])
gstbase_dep = dependency('gstreamer-base-1.0', version : '>=1.16.3',
  fallback : ['gstreamer', 'gst_base_dep'])

subdir('gst-app')
subdir('gst-plugin')
