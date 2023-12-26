tracker_test_sources = [
    '../src/tracker.cpp',
    '../src/box.cpp',
    'tracker_test.cpp'
]

e = executable('tracker_test', tracker_test_sources, dependencies: [thread_dep])
test('Tracking Add', e)
test('Tracking DOA', e, args: ['doa'])
test('Tracking Time', e, args: ['time'])