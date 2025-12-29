
lpkg_srcs = [
  'conio.c',
  'dbio.c',
  'dbmap.c',
  'fileio.c',
  'lpkg.c',
  'main.c',
  'mode_dbquery.c',
  'mode_init.c',
  'mode_install.c',
  'mode_remove.c',
  'mode_unimplemented.c',
  'package.c',
  'stb_ds.c',
  'unpack.c',
  'version.c',
]

lpkg = executable(
  'lpkg', 
  lpkg_srcs, 
  dependencies : [
    inih_dep,
    libarchive_dep,
    sqlite3_dep,
  ],
)

# end of file
