project('equinox', 'c',
  version: '1.0.0',
  default_options: [
    'c_std=c11',
    'warning_level=3',
    'werror=false',
  ],
  license: 'MIT',
  meson_version: '>=0.56.0'
)

# Project information
project_description = 'Equinox E-graph library'
project_url = 'https://github.com/warble/equinox'

# Build options
eqx_build_tests = get_option('build_tests')
eqx_build_docs = get_option('build_docs')
eqx_build_shared = get_option('default_library') in ['shared', 'both']
eqx_build_static = get_option('default_library') in ['static', 'both']

# Compiler setup
cc = meson.get_compiler('c')

# Compiler flags
add_project_arguments(
  cc.get_supported_arguments([
    '-Wall',
    '-Wextra',
    '-pedantic',
  ]),
  language: 'c'
)

# Include directories
inc = include_directories(
  'include',
  'third_party',
  'third_party/uthash/src',
  'third_party/tlsf',
  'third_party/libdag-ethash',
)

# Third-party dependencies
uthash_dep = dependency('uthash', 
  required: false,
  fallback: ['uthash', 'uthash_dep']
)

if not uthash_dep.found()
  uthash_dep = declare_dependency(
    include_directories: include_directories('third_party/uthash/src')
  )
endif

tlsf_proj = subproject('tlsf', required: false)
if tlsf_proj.found()
  tlsf_dep = tlsf_proj.get_variable('tlsf_dep')
else
  tlsf_lib = static_library('tlsf',
    'third_party/tlsf/tlsf.c',
    include_directories: include_directories('third_party/tlsf'),
    install: false
  )
  tlsf_dep = declare_dependency(
    link_with: tlsf_lib,
    include_directories: include_directories('third_party/tlsf')
  )
endif

libdag_proj = subproject('libdag-ethash', required: false)
if libdag_proj.found()
  libdag_dep = libdag_proj.get_variable('libdag_dep')
else
  libdag_lib = static_library('dag-ethash',
    'third_party/libdag-ethash/dag.c',
    'third_party/libdag-ethash/ethash.c',
    include_directories: include_directories('third_party/libdag-ethash'),
    install: false
  )
  libdag_dep = declare_dependency(
    link_with: libdag_lib,
    include_directories: include_directories('third_party/libdag-ethash')
  )
endif

# Source files
equinox_sources = files(
  'src/enode.c',
  'src/eclass.c',
  'src/egraph.c',
  'src/unionfind.c',
  'src/hashcons.c',
  'src/rewrite.c',
)

# Header files
equinox_headers = files(
  'include/equinox/equinox.h',
  'include/equinox/enode.h',
  'include/equinox/eclass.h',
  'include/equinox/egraph.h',
  'include/equinox/unionfind.h',
  'include/equinox/hashcons.h',
  'include/equinox/rewrite.h',
  'include/equinox/libutils.h',
)

# Dependencies
equinox_deps = [
  uthash_dep,
  tlsf_dep,
  libdag_dep,
]

# Library target
equinox_lib = library('equinox',
  equinox_sources,
  include_directories: inc,
  dependencies: equinox_deps,
  version: meson.project_version(),
  soversion: '1',
  install: true,
  c_args: ['-DEQUINOX_BUILD'],
)

# Declare dependency for use as subproject
equinox_dep = declare_dependency(
  link_with: equinox_lib,
  include_directories: inc,
  dependencies: equinox_deps,
)

# Install headers
install_headers(equinox_headers, subdir: 'equinox')

# pkg-config file
pkg = import('pkgconfig')
pkg.generate(
  equinox_lib,
  name: 'equinox',
  description: project_description,
  url: project_url,
  subdirs: 'equinox',
  version: meson.project_version(),
)

# Tests
if eqx_build_tests
  subdir('tests')
endif

# Documentation
if eqx_build_docs
  subdir('docs')
endif

# Summary
summary({
  'Version': meson.project_version(),
  'Build type': get_option('buildtype'),
  'C standard': get_option('c_std'),
  'Library type': get_option('default_library'),
  'Tests': eqx_build_tests,
  'Documentation': eqx_build_docs,
}, section: 'Configuration')

summary({
  'prefix': get_option('prefix'),
  'libdir': get_option('libdir'),
  'includedir': get_option('includedir'),
}, section: 'Directories')
