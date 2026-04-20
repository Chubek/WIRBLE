project(
  'wirble',
  'c',
  version : '0.4',
  default_options : ['c_std=c11', 'warning_level=2']
)

install_docs = get_option('install_docs')
install_manfiles = get_option('install_manfiles')
install_manifest = get_option('install_manifest')
install_toolchain = get_option('install_toolchain')
no_install_machines = get_option('no_install_machines')

subdir('dependencies')
subdir('stdrewrite')
subdir('manifests')
subdir('machines')
subdir('toolchain')
subdir('docs')
