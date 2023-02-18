plugin_c_args = ['-DHAVE_CONFIG_H']

cdata = configuration_data()
cdata.set_quoted('PACKAGE_VERSION', gst_version)
cdata.set_quoted('PACKAGE', 'gst-drpai-plugin')
cdata.set_quoted('GST_LICENSE', 'LGPL')
cdata.set_quoted('GST_API_VERSION', api_version)
cdata.set_quoted('GST_PACKAGE_NAME', 'GStreamer DRP-AI Plug-in')
cdata.set_quoted('GST_PACKAGE_ORIGIN', 'https://mistysom.com')
configure_file(output : 'config.h', configuration : cdata)

### gstaudio_dep = dependency('gstreamer-audio-1.0',
###     fallback: ['gst-plugins-base', 'audio_dep'])

# Plugin 1
plugin_sources = [
  'src/gstdrpai.c'
  ]

gstplugindrpai = library('gstdrpai',
  plugin_sources,
  c_args: plugin_c_args,
  dependencies : [gst_dep],
  install : true,
  install_dir : plugins_install_dir,
)

# Plugin 2 (audio filter example)
### audiofilter_sources = [
###   'src/gstaudiofilter.c',
###   ]

### gstaudiofilterexample = library('gstaudiofilterexample',
###   audiofilter_sources,
###   c_args: plugin_c_args,
###   dependencies : [gst_dep, gstaudio_dep],
###   install : true,
###   install_dir : plugins_install_dir,
### )

# The TEMPLATE Plugin
### gstTEMPLATE_sources = [
###  'src/gstTEMPLATE.c',
###  ]

###gstTEMPLATEexample = library('gstTEMPLATE',
###  gstTEMPLATE_sources,
###  c_args: plugin_c_args,
###  dependencies : [gst_dep, gstbase_dep],
###  install : true,
###  install_dir : plugins_install_dir,
###)
