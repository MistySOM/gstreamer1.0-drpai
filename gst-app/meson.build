app_sources = [
  'src/main.cpp'
]

executable('gst-launch', app_sources, dependencies : [gst_dep])
executable('gst-launch-split', app_sources, dependencies : [gst_dep, thread_dep], cpp_args : '-DENABLE_SPLIT')