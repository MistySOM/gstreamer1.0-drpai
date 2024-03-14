gstdrpai_yolo_dep = declare_dependency(link_with : gstdrpai_yolo, include_directories : incdir)

tracker_test = executable('tracker_test', 'tracker_test.cpp', include_directories : incdir,
    dependencies: [thread_dep, gstdrpai_yolo_dep]
)
test('Tracking Add', tracker_test)
test('Tracking DOA', tracker_test, args: ['doa'])
test('Tracking Time', tracker_test, args: ['time'])
test('Tracking History', tracker_test, args: ['history'])

filter_test = executable('filter_test', 'filter_test.cpp', include_directories : incdir,
    dependencies: [thread_dep, gstdrpai_yolo_dep]
)
test('Filtering Inactive', filter_test)
test('Filtering Classes', filter_test, args: ['class'])