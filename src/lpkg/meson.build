
lpkg_srcs = [
  'dbio.c',
  'lpkg.c',
  'main.c',
  'mode_dbquery.c',
  'mode_init.c',
  'mode_install.c',
  'mode_unimplemented.c',
  'prompt.c',
]

lpkg = executable(
  'lpkg', 
  lpkg_srcs, 
  dependencies : [
    libsqlite3_dep,
  ],
)

# end of file
