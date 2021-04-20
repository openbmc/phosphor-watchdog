project(
  'phosphor-watchdog',
  'cpp',
  version: '0.1',
  meson_version: '>=0.55.0',
  default_options: [
    'warning_level=3',
    'cpp_std=c++17',
  ])

watchdog_headers = include_directories('.')

watchdog_deps = [
  dependency('phosphor-dbus-interfaces'),
  dependency('phosphor-logging'),
  dependency('sdbusplus', fallback: ['sdbusplus', 'sdbusplus_dep']),
  dependency('sdeventplus', fallback: ['sdeventplus', 'sdeventplus_dep']),
]

watchdog_lib = static_library(
  'watchdog',
  'watchdog.cpp',
  implicit_include_directories: false,
  include_directories: watchdog_headers,
  dependencies: watchdog_deps)

watchdog_dep = declare_dependency(
  dependencies: watchdog_deps,
  include_directories: watchdog_headers,
  link_with: watchdog_lib)

executable(
  'phosphor-watchdog',
  'mainapp.cpp',
  implicit_include_directories: false,
  dependencies: watchdog_dep,
  install: true,
  install_dir: get_option('bindir'))

if not get_option('tests').disabled()
  subdir('test')
endif
