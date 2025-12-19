
project(
  'Leaf Package Manager',
  [ 'c' ],
  version : '0.1.0-dev',
  license : 'MIT',
  default_options : [
    'c_std=c99',
    'warning_level=3',
  ],
  meson_version : '>= 0.1',
)

libsqlite3_dep = dependency('sqlite3', required : true)

subdir('src')

# end of file
