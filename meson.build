project('gst-drpai', 'c', 'cpp', version : '1.19.0.1', license : 'LGPL', default_options : [
'buildtype=release'
])

plugins_install_dir = join_paths('/usr/lib64', 'gstreamer-1.0')

cc = meson.get_compiler('c')
cxx = meson.get_compiler('cpp')

gst_version = meson.project_version()

api_version = '1.0'

gst_dep = dependency('gstreamer-1.0', version : '>=1.16.3',
    required : true, fallback : ['gstreamer', 'gst_dep'])
gstbase_dep = dependency('gstreamer-base-1.0', version : '>=1.16.3',
  fallback : ['gstreamer', 'gst_base_dep'])
thread_dep = dependency('threads')
m_dep = cc.find_library('m', required : true)

subdir('gst-app')
subdir('gst-plugin')
