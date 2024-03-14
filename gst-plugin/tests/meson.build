gstdrpai_yolo_dep = declare_dependency(link_with : gstdrpai_yolo, include_directories : incdir)

e = executable('tracker_test', tracker_test_sources, dependencies: [thread_dep])
test('Tracking Add', e)
test('Tracking DOA', e, args: ['doa'])
test('Tracking Time', e, args: ['time'])
test('Tracking History', e, args: ['history'])

filter_test = executable('filter_test',
    'filter_test.cpp',
    include_directories : incdir,
    dependencies: [thread_dep, gstdrpai_yolo_dep]
)
test('Filtering Classes', filter_test, args: ['class'])