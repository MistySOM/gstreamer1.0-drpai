yolov2_sources = [
  'yolov2.c'
]

yolov2 = library('yolov2',
  yolov2_sources,
  c_args: ['-DYOLOV2'],
  dependencies: m_dep
)

tinyyolov2 = library('tinyyolov2',
  yolov2_sources,
  c_args: ['-DTINYYOLOV2'],
  dependencies: m_dep
)