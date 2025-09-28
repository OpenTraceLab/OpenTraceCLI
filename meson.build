project('opentrace-cli', 'c',
  version : '0.1.0',
  license : 'GPL-3.0-or-later',
  meson_version : '>= 0.56.0',
  default_options : [
    'c_std=c99',
    'warning_level=2',
    'werror=false'
  ]
)

# Project information
project_description = 'Command-line frontend for OpenTrace'
project_url = 'http://www.opentrace.org'

# Compiler setup
cc = meson.get_compiler('c')

# Add extra C flags for C99 support and warnings
extra_cflags = []
warning_flags = []

# Check for C99 support
if cc.has_argument('-std=c99')
  extra_cflags += '-std=c99'
elif cc.has_argument('-c99')
  extra_cflags += '-c99'
elif cc.has_argument('-AC99')
  extra_cflags += '-AC99'
elif cc.has_argument('-qlanglvl=extc99')
  extra_cflags += '-qlanglvl=extc99'
endif

# Warning flags
if cc.has_argument('-Wall')
  warning_flags += '-Wall'
endif
if cc.has_argument('-Wextra')
  warning_flags += '-Wextra'
endif

add_project_arguments(extra_cflags + warning_flags, language : 'c')

# Large file support
add_project_arguments('-D_FILE_OFFSET_BITS=64', language : 'c')

# POSIX C source
add_project_arguments('-D_POSIX_C_SOURCE=200112L', language : 'c')

# Required dependencies
glib_dep = dependency('glib-2.0', version : '>= 2.32.0')
libopentracecapture_dep = dependency('opentracecapture')

# Optional dependencies
libopentracedecode_dep = dependency('libopentracedecode', 
  version : '>= 0.5.0', 
  required : get_option('decode'),
  disabler : true
)

# Configuration data
conf_data = configuration_data()
conf_data.set_quoted('PACKAGE_VERSION', meson.project_version())
conf_data.set_quoted('PACKAGE_NAME', meson.project_name())
conf_data.set('HAVE_OTD', libopentracedecode_dep.found())

# Get library versions for summary
glib_version = glib_dep.version()
libopentracecapture_version = libopentracecapture_dep.version()

configure_file(
  output : 'config.h',
  configuration : conf_data
)

# Include directories
inc_dir = include_directories('.')

# Source files - always include all sources like original Makefile.am
# Conditional compilation happens at preprocessor level with #ifdef HAVE_OTD
opentrace_cli_sources = [
  'main.c',
  'show.c', 
  'device.c',
  'session.c',
  'input.c',
  'output.c',
  'decode.c',
  'parsers.c',
  'anykey.c',
  'options.c'
]

# Dependencies list
deps = [libopentracecapture_dep, glib_dep]
if libopentracedecode_dep.found()
  deps += libopentracedecode_dep
endif

# Main executable
executable('opentrace-cli',
  opentrace_cli_sources,
  dependencies : deps,
  include_directories : inc_dir,
  install : true
)

# Install man page
install_man('doc/opentrace-cli.1')

# Install desktop file and icon
install_data('contrib/org.opentrace.opentrace-cli.desktop',
  install_dir : get_option('datadir') / 'applications'
)

install_data('contrib/opentrace-cli.svg',
  install_dir : get_option('datadir') / 'icons/hicolor/scalable/apps'
)

# Configuration summary
summary({
  'Package version' : meson.project_version(),
  'Prefix' : get_option('prefix'),
  'C compiler' : cc.get_id() + ' ' + cc.version(),
  'C compiler flags' : ' '.join(get_option('c_args')),
  'Extra C flags' : ' '.join(extra_cflags),
  'Warning flags' : ' '.join(warning_flags),
}, section : 'Compile configuration')

summary({
  'glib-2.0' : glib_version,
  'libopentracecapture' : libopentracecapture_version,
}, section : 'Required libraries')

if libopentracedecode_dep.found()
  summary({
    'libopentracedecode' : libopentracedecode_dep.version(),
  }, section : 'Optional libraries')
else
  summary({
    'libopentracedecode' : 'NO (protocol decoding disabled)',
  }, section : 'Optional libraries')
endif
