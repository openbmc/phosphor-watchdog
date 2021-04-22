project(
  'phosphor-watchdog',
  'cpp',
  version: '0.1',
  meson_version: '>=0.57.0',
  default_options: [
    'warning_level=3',
    'cpp_std=c++20',
  ])

subdir('src')

if not get_option('tests').disabled()
  subdir('test')
endif
