manifestdir = $(datadir)/wirble/manifests
manifest_api_dir = $(manifestdir)/api
manifest_grammar_dir = $(manifestdir)/grammars
manifest_schema_dir = $(manifestdir)/schema

EXTRA_DIST = \
	SourceManifest.yaml \
	ToolchainManifest.yaml \
	api/WRIBLE-MAL-API.zspec \
	api/WRIBLE-MDS-API.zspec \
	api/WRIBLE-TOS-API.zspec \
	api/WRIBLE-WIL-API.zspec \
	api/WRIBLE-WRS-API.zspec \
	api/WRIBLE-WVM-API.zspec \
	grammars/WRS.ebnf \
	schema/MDS-Schema.json \
	schema/MDS-Schema.xsd \
	schema/MDS-Schema.yaml

if INSTALL_MANIFEST
manifest_DATA = \
	SourceManifest.yaml \
	ToolchainManifest.yaml

manifest_api_DATA = \
	api/WRIBLE-MAL-API.zspec \
	api/WRIBLE-MDS-API.zspec \
	api/WRIBLE-TOS-API.zspec \
	api/WRIBLE-WIL-API.zspec \
	api/WRIBLE-WRS-API.zspec \
	api/WRIBLE-WVM-API.zspec

manifest_grammar_DATA = \
	grammars/WRS.ebnf

manifest_schema_DATA = \
	schema/MDS-Schema.json \
	schema/MDS-Schema.xsd \
	schema/MDS-Schema.yaml
endif
