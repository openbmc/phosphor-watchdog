project(
  'phosphor-watchdog',
  'cpp',
  version: '0.1',
  meson_version: '>=1.1.1',
  default_options: [
    'warning_level=3',
    'cpp_std=c++23',
  ])

cpp = meson.get_compiler('cpp')

subdir('src')

if not get_option('tests').disabled()
  subdir('test')
endif
