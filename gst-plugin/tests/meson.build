tracker_test = executable('tracker_test', 'tracker_test.cpp', include_directories : incdir,
    dependencies: [thread_dep, gstdrpai_dep]
)
test('Tracking Add', tracker_test)
test('Tracking DOA', tracker_test, args: ['doa'])
test('Tracking Time', tracker_test, args: ['time'])
test('Tracking History', tracker_test, args: ['history'])

filter_test = executable('filter_test', 'filter_test.cpp', include_directories : incdir,
    dependencies: [thread_dep, gstdrpai_dep]
)
test('Filtering Inactive', filter_test)
test('Filtering Classes', filter_test, args: ['class'])

image_test = executable('image_test', 'image_test.cpp', include_directories : incdir,
    dependencies: [thread_dep, gstdrpai_dep]
)
test('Image Save Bitmap File', image_test, args: ['save_bmp'])
