if install_manifest
  install_data(
    'SourceManifest.yaml',
    'ToolchainManifest.yaml',
    install_dir : get_option('datadir') / 'wirble' / 'manifests'
  )

  install_data(
    'api/WRIBLE-MAL-API.zspec',
    'api/WRIBLE-MDS-API.zspec',
    'api/WRIBLE-TOS-API.zspec',
    'api/WRIBLE-WIL-API.zspec',
    'api/WRIBLE-WRS-API.zspec',
    'api/WRIBLE-WVM-API.zspec',
    install_dir : get_option('datadir') / 'wirble' / 'manifests' / 'api'
  )

  install_data(
    'grammars/WRS.ebnf',
    install_dir : get_option('datadir') / 'wirble' / 'manifests' / 'grammars'
  )

  install_data(
    'schema/MDS-Schema.json',
    'schema/MDS-Schema.xsd',
    'schema/MDS-Schema.yaml',
    install_dir : get_option('datadir') / 'wirble' / 'manifests' / 'schema'
  )
endif
