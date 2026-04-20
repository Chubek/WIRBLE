doxygen = find_program('doxygen', required : false)
dot = find_program('dot', required : false)
makeprog = find_program('gmake', required : false)
if not makeprog.found()
  makeprog = find_program('make', required : false)
endif

doc_output_dir = meson.current_build_dir() / 'doxygen'
doc_html_dir = doc_output_dir / 'html'
doc_latex_dir = doc_output_dir / 'latex'
doc_man_dir = doc_output_dir / 'man' / 'man3wil'

doxy_conf = configuration_data()
doxy_conf.set('WIRBLE_PROJECT_VERSION', meson.project_version())
doxy_conf.set('WIRBLE_TOP_SRCDIR', meson.project_source_root())
doxy_conf.set('WIRBLE_OUTPUT_DIRECTORY', doc_output_dir)
doxy_conf.set('WIRBLE_STRIP_FROM_INC_PATH', meson.project_source_root() / 'include' + ' ' + meson.project_source_root() / 'src')
doxy_conf.set('WIRBLE_LAYOUT_FILE', meson.project_source_root() / 'docs' / 'layout.xml')
doxy_conf.set('WIRBLE_WARN_LOGFILE', meson.current_build_dir() / 'doxygen-warnings.log')
doxy_conf.set('WIRBLE_HAVE_DOT', dot.found() ? 'YES' : 'NO')

doxyfile = configure_file(
  input : 'Doxyfile.in',
  output : 'Doxyfile',
  configuration : doxy_conf
)

if doxygen.found()
  run_target(
    'wirble-docs',
    command : [doxygen, doxyfile]
  )

  if makeprog.found()
    run_target(
      'wirble-docs-pdf',
      command : [
        find_program('sh'),
        '-c',
        'set -eu; "$1" "$2"; if [ -f "$3/Makefile" ]; then "$4" -C "$3" pdf; fi',
        'sh',
        doxygen.full_path(),
        doxyfile.full_path(),
        doc_latex_dir,
        makeprog.full_path(),
      ]
    )
  endif

  if install_docs or install_manfiles
    install_script = [
      find_program('sh'),
      '-c',
      'set -eu; builddir="$1"; prefix="$2"; docroot="$3"; mandir="$4"; doxy="$5"; doxyfile="$6"; makeprog="$7"; want_docs="$8"; want_man="$9"; "$doxy" "$doxyfile"; if [ -n "$makeprog" ] && [ -f "$builddir/doxygen/latex/Makefile" ]; then "$makeprog" -C "$builddir/doxygen/latex" pdf || true; fi; if [ "$want_docs" = "true" ]; then mkdir -p "$prefix/$docroot/html" "$prefix/$docroot/latex"; if [ -d "$builddir/doxygen/html" ]; then cp -R "$builddir/doxygen/html/." "$prefix/$docroot/html/"; fi; if [ -d "$builddir/doxygen/latex" ]; then cp -R "$builddir/doxygen/latex/." "$prefix/$docroot/latex/"; fi; if [ -f "$builddir/doxygen/latex/refman.pdf" ]; then mkdir -p "$prefix/$docroot/pdf"; cp "$builddir/doxygen/latex/refman.pdf" "$prefix/$docroot/pdf/refman.pdf"; fi; fi; if [ "$want_man" = "true" ]; then mkdir -p "$prefix/$mandir/man3wil"; if [ -d "$builddir/doxygen/man/man3wil" ]; then cp -R "$builddir/doxygen/man/man3wil/." "$prefix/$mandir/man3wil/"; fi; fi',
      'sh',
      meson.current_build_dir(),
      get_option('prefix'),
      get_option('datadir') / 'doc' / meson.project_name(),
      get_option('mandir'),
      doxygen.full_path(),
      doxyfile.full_path(),
      makeprog.found() ? makeprog.full_path() : '',
      install_docs ? 'true' : 'false',
      install_manfiles ? 'true' : 'false',
    ]
    meson.add_install_script(install_script[0], install_script[1], install_script[2], install_script[3], install_script[4], install_script[5], install_script[6], install_script[7], install_script[8], install_script[9], install_script[10], install_script[11])
  endif
endif
