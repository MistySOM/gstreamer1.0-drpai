yolov3_sources = [
  'yolov3.c'
]

yolov3 = library('yolov3',
  yolov3_sources,
  c_args: ['-DYOLOV3'],
  dependencies: m_dep
)

tinyyolov3 = library('tinyyolov3',
  yolov3_sources,
  c_args: ['-DTINYYOLOV3'],
  dependencies: m_dep
)