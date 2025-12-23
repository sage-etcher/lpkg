
project(
  'Leaf Package Manager',
  'c',
  version : '0.1.0-dev',
  license : 'MIT',
  default_options : [
    'c_std=c99',
    'warning_level=3',
    'b_ndebug=false', # build with asserts regardless of buildtype
  ],
  meson_version : '>= 0.50.0',
)

cc = meson.get_compiler('c')

# check for optional features
accepts_ggdb = cc.compiles(
  'int main() { return 0; }',
  args : '-ggdb',
  name : 'cflag -ggdb',
)

accepts_defines = cc.compiles(
  'int main() { return TEST_DEFINE; }',
  args : '-DTEST_DEFINE=10',
  name : 'cflag -Dname=value',
)
if not accepts_defines
  error('compiler does not support -Dname=value defines, aborting...')
endif

if get_option('buildtype').startswith('debug')

  # use -ggdb if compiler supports it
  if accepts_ggdb
    add_project_arguments('-ggdb', language : 'c')
  endif

  add_project_arguments('-D_POSIX_C_SOURCE=202809L', language : 'c')
endif

# check for non-standard headers and functions
cc.has_header('getopt.h',   required : true)
cc.has_header('sys/stat.h', required : true)
cc.has_header('unistd.h',   required : true)

# get dependencies
sqlite3_dep = dependency('sqlite3', required : true)
inih_dep = dependency('inih', required : true)
libarchive_dep = dependency('libarchive', required : true)

# build instructions
subdir('src')

# end of file
