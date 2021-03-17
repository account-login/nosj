import os
import glob


CXX = os.environ.get('CXX') or 'g++'
CXXFLAGS = '-std=gnu++11 -Wall -Wextra -g'.split()
CXXFLAGS += '-Og --coverage'.split()
LD = CXX
LD_FLAGS = ['-coverage']


def o(file):
    return '_out/' + file.replace('.cpp', '.o')


def d(file):
    return '_out/' + file.replace('.cpp', '.d')


def rules(ctx):
    c_lib_files = [
        'j/j_dumper.cpp',
        'j/j_parser.cpp',
        'j/j_reader.cpp',
        'j/j_writer.cpp',
        'j/j_quick.cpp',
    ]
    o_lib_files = [o(file) for file in c_lib_files]
    c_test_files = [
        'tests/test_parser.cpp',
        'tests/test_dumper.cpp',
        'tests/test_reader.cpp',
        'tests/test_writer.cpp',
        'tests/test_quick.cpp',
        'tests/test_run_json_test_suite.cpp',
    ]

    # compile objects
    for file in c_lib_files + c_test_files + ['tests/main.cpp']:
        cmd = [CXX, *CXXFLAGS, '-o', o(file), '-c', file, '-MD', '-MP']
        ctx.add_rule(o(file), [file], cmd, d_file=d(file))

    # compile test binaries
    test_exe_files = []
    for file in c_test_files:
        exe_file = file.replace('tests/', '').replace('.cpp', '')
        o_files = o_lib_files + [o(file), o('tests/main.cpp')]
        cmd = [LD, *LD_FLAGS, '-o', exe_file, *o_files]
        ctx.add_rule(exe_file, o_files, cmd)
        test_exe_files.append(exe_file)

    # C++98 compile test
    cxx98_flags = ['-std=c++98' if x.startswith('-std=') else x for x in CXXFLAGS]
    cxx98_o_files = []
    for file in c_lib_files:
        o_file = '_out/' + file.replace('.cpp', '.c++98.o')
        d_file = '_out/' + file.replace('.cpp', '.c++98.d')
        cmd = [CXX, *cxx98_flags, '-o', o_file, '-c', file, '-MD', '-MP']
        ctx.add_rule(o_file, [file], cmd, d_file=d_file)
        cxx98_o_files.append(o_file)

    # dummy test target
    ctx.add_rule('test', test_exe_files + cxx98_o_files, ['true'])

    # coverage
    ctx.add_rule('lcov-zero', [], ['lcov --directory . --zerocounters'.split()])
    ctx.add_rule('lcov-html', glob.glob('_out/**/*.gcda'), [
        'lcov --directory . --capture --include'.split() + [os.getcwd() + '/*'] + '--rc lcov_branch_coverage=1 --output-file _out/cov.info'.split(),
        'genhtml --prefix'.split() + [os.getcwd()] + '--rc lcov_branch_coverage=1 _out/cov.info --output-directory=lcov-html'.split(),
    ])
