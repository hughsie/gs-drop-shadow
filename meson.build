project('gs-drop-shadow', 'c',
  version : '0.0.2',
  license : 'GPL-2.0+',
  default_options : ['warning_level=1', 'c_std=c99'],
  meson_version : '>=0.40.0'
)

# get suported warning flags
test_args = [
  '-Warray-bounds',
  '-Wcast-align',
  '-Wclobbered',
  '-Wdeclaration-after-statement',
  '-Wextra',
  '-Wformat=2',
  '-Wformat-nonliteral',
  '-Wformat-signedness',
  '-Wignored-qualifiers',
  '-Wimplicit-function-declaration',
  '-Wmissing-format-attribute',
  '-Wmissing-include-dirs',
  '-Wmissing-noreturn',
  '-Wmissing-parameter-type',
  '-Wmissing-prototypes',
  '-Wno-discarded-qualifiers',
  '-Wno-missing-field-initializers',
  '-Wno-strict-aliasing',
  '-Wno-suggest-attribute=format',
  '-Wno-unused-parameter',
  '-Wold-style-definition',
  '-Woverride-init',
  '-Wpointer-arith',
  '-Wredundant-decls',
  '-Wreturn-type',
  '-Wshadow',
  '-Wsign-compare',
  '-Wstrict-aliasing',
  '-Wstrict-prototypes',
  '-Wswitch-default',
  '-Wtype-limits',
  '-Wuninitialized',
  '-Wunused-but-set-variable',
  '-Wwrite-strings'
]
cc = meson.get_compiler('c')
foreach arg: test_args
  if cc.has_argument(arg)
    add_project_arguments(arg, language : 'c')
  endif
endforeach

libcairo = dependency('cairo')
libglib = dependency('gio-unix-2.0', version : '>= 2.56.0')
libm = cc.find_library('m', required: false)

e = executable(
  'gs-drop-shadow',
  sources : [
    'gs-drop-shadow.c'
  ],
  dependencies : [
    libcairo,
    libglib,
    libm,
  ],
)
