project('opentrace-cli', 'c',
  version : '0.8.0',
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

# Dependencies
libopentrace_dep = dependency('libopentrace', version : '>= 0.6.0')
glib_dep = dependency('glib-2.0', version : '>= 2.32.0')

# Optional dependencies
libopentracedecode_dep = dependency('libopentracedecode', version : '>= 0.6.0', required : false)

# Configuration
conf_data = configuration_data()
conf_data.set_quoted('PACKAGE_VERSION', meson.project_version())
conf_data.set_quoted('PACKAGE_NAME', meson.project_name())
conf_data.set('HAVE_SRD', libopentracedecode_dep.found())

configure_file(
  output : 'config.h',
  configuration : conf_data
)

# Include directories
inc_dir = include_directories('.')

# Source files
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
deps = [libopentrace_dep, glib_dep]
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
