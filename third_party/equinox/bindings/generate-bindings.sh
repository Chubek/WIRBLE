#!/usr/bin/env bash
# generate-bindings.sh - Generate SWIG bindings for Equinox with XFeats support

set -e

# ============================================================================
# Configuration
# ============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BINDINGS_DIR="$SCRIPT_DIR"
INTERFACE_FILE="$BINDINGS_DIR/Equinox.i"

# Source directories
INCLUDE_DIR="$PROJECT_ROOT/include"
SRC_DIR="$PROJECT_ROOT/src"

# Output directories
OUT_DIR="$BINDINGS_DIR/generated"
PYTHON_OUT="$OUT_DIR/python"
JAVA_OUT="$OUT_DIR/java"
CSHARP_OUT="$OUT_DIR/csharp"
RUBY_OUT="$OUT_DIR/ruby"
GO_OUT="$OUT_DIR/go"
LUA_OUT="$OUT_DIR/lua"
PERL_OUT="$OUT_DIR/perl"
JS_OUT="$OUT_DIR/javascript"

# SWIG executable
SWIG="${SWIG:-swig}"

# XFeats version
XFEATS_VERSION="v1.0.0"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# ============================================================================
# Helper Functions
# ============================================================================

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_swig() {
    if ! command -v "$SWIG" &> /dev/null; then
        log_error "SWIG not found. Please install SWIG 4.0 or later."
        exit 1
    fi
    
    local version=$("$SWIG" -version | grep -oP 'SWIG Version \K[0-9.]+')
    log_info "Using SWIG version $version"
    
    if [[ $(echo "$version" | cut -d. -f1) -lt 4 ]]; then
        log_warning "SWIG 4.0+ recommended for best XFeats support"
    fi
}

check_interface() {
    if [[ ! -f "$INTERFACE_FILE" ]]; then
        log_error "Interface file not found: $INTERFACE_FILE"
        exit 1
    fi
    log_info "Using interface file: $INTERFACE_FILE"
}

create_output_dirs() {
    log_info "Creating output directories..."
    mkdir -p "$PYTHON_OUT" "$JAVA_OUT" "$CSHARP_OUT" "$RUBY_OUT" \
             "$GO_OUT" "$LUA_OUT" "$PERL_OUT" "$JS_OUT"
}

# ============================================================================
# Language-Specific Generators
# ============================================================================

generate_python() {
    log_info "Generating Python bindings with XFeats..."
    
    "$SWIG" -python -py3 \
        -I"$INCLUDE_DIR" \
        -outdir "$PYTHON_OUT" \
        -o "$PYTHON_OUT/equinox_wrap.c" \
        -builtin \
        -threads \
        "$INTERFACE_FILE"
    
    if [[ $? -eq 0 ]]; then
        log_success "Python bindings generated"
        
        # Create setup.py
        cat > "$PYTHON_OUT/setup.py" << 'EOF'
from setuptools import setup, Extension
import os

equinox_module = Extension(
    '_Equinox',
    sources=['equinox_wrap.c'],
    include_dirs=['../../include'],
    library_dirs=['../../build'],
    libraries=['equinox'],
    extra_compile_args=['-std=c11', '-O2'],
)

setup(
    name='equinox',
    version='0.1.0',
    description='Python bindings for Equinox E-graph library',
    ext_modules=[equinox_module],
    py_modules=['Equinox'],
    python_requires='>=3.7',
)
EOF
        
        # Create __init__.py
        cat > "$PYTHON_OUT/__init__.py" << 'EOF'
"""
Equinox - E-graph library Python bindings
XFeats-enhanced interface for idiomatic Python usage
"""

from .Equinox import *

__version__ = '0.1.0'
__xfeats_version__ = 'v1.0.0'

__all__ = [
    'EGraph',
    'ENode',
    'EClass',
    'HashCons',
    'RewriteRule',
]
EOF
        
        log_info "Created setup.py and __init__.py"
    else
        log_error "Failed to generate Python bindings"
        return 1
    fi
}

generate_java() {
    log_info "Generating Java bindings with XFeats..."
    
    local java_package="io.warble.equinox"
    local java_package_dir="${java_package//./\/}"
    local java_out_pkg="$JAVA_OUT/$java_package_dir"
    
    mkdir -p "$java_out_pkg"
    
    "$SWIG" -java \
        -I"$INCLUDE_DIR" \
        -package "$java_package" \
        -outdir "$java_out_pkg" \
        -o "$JAVA_OUT/equinox_wrap.c" \
        "$INTERFACE_FILE"
    
    if [[ $? -eq 0 ]]; then
        log_success "Java bindings generated"
        
        # Create Maven pom.xml
        cat > "$JAVA_OUT/pom.xml" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<project xmlns="http://maven.apache.org/POM/4.0.0"
         xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
         xsi:schemaLocation="http://maven.apache.org/POM/4.0.0
         http://maven.apache.org/xsd/maven-4.0.0.xsd">
    <modelVersion>4.0.0</modelVersion>
    
    <groupId>io.warble</groupId>
    <artifactId>equinox</artifactId>
    <version>0.1.0</version>
    <packaging>jar</packaging>
    
    <name>Equinox Java Bindings</name>
    <description>XFeats-enhanced Java bindings for Equinox E-graph library</description>
    
    <properties>
        <maven.compiler.source>11</maven.compiler.source>
        <maven.compiler.target>11</maven.compiler.target>
        <project.build.sourceEncoding>UTF-8</project.build.sourceEncoding>
    </properties>
    
    <build>
        <sourceDirectory>$java_package_dir</sourceDirectory>
    </build>
</project>
EOF
        
        log_info "Created pom.xml"
    else
        log_error "Failed to generate Java bindings"
        return 1
    fi
}

generate_csharp() {
    log_info "Generating C# bindings with XFeats..."
    
    "$SWIG" -csharp \
        -I"$INCLUDE_DIR" \
        -namespace Warble.Equinox \
        -outdir "$CSHARP_OUT" \
        -o "$CSHARP_OUT/equinox_wrap.c" \
        -dllimport equinox \
        "$INTERFACE_FILE"
    
    if [[ $? -eq 0 ]]; then
        log_success "C# bindings generated"
        
        # Create .csproj file
        cat > "$CSHARP_OUT/Equinox.csproj" << 'EOF'
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <TargetFramework>net6.0</TargetFramework>
    <RootNamespace>Warble.Equinox</RootNamespace>
    <AssemblyName>Equinox</AssemblyName>
    <Version>0.1.0</Version>
    <Description>XFeats-enhanced C# bindings for Equinox E-graph library</Description>
    <AllowUnsafeBlocks>true</AllowUnsafeBlocks>
  </PropertyGroup>
  
  <ItemGroup>
    <Compile Include="*.cs" />
  </ItemGroup>
</Project>
EOF
        
        log_info "Created Equinox.csproj"
    else
        log_error "Failed to generate C# bindings"
        return 1
    fi
}

generate_ruby() {
    log_info "Generating Ruby bindings with XFeats..."
    
    "$SWIG" -ruby \
        -I"$INCLUDE_DIR" \
        -outdir "$RUBY_OUT" \
        -o "$RUBY_OUT/equinox_wrap.c" \
        "$INTERFACE_FILE"
    
    if [[ $? -eq 0 ]]; then
        log_success "Ruby bindings generated"
        
        # Create extconf.rb
        cat > "$RUBY_OUT/extconf.rb" << 'EOF'
require 'mkmf'

dir_config('equinox', '../../include', '../../build')

have_library('equinox') or raise 'libequinox not found'

$CFLAGS << ' -std=c11 -O2'

create_makefile('Equinox')
EOF
        
        log_info "Created extconf.rb"
    else
        log_error "Failed to generate Ruby bindings"
        return 1
    fi
}

generate_go() {
    log_info "Generating Go bindings with XFeats..."
    
    "$SWIG" -go \
        -I"$INCLUDE_DIR" \
        -intgosize 64 \
        -outdir "$GO_OUT" \
        -o "$GO_OUT/equinox_wrap.c" \
        "$INTERFACE_FILE"
    
    if [[ $? -eq 0 ]]; then
        log_success "Go bindings generated"
        
        # Create go.mod
        cat > "$GO_OUT/go.mod" << 'EOF'
module github.com/warble/equinox

go 1.19

// XFeats-enhanced Go bindings for Equinox E-graph library
EOF
        
        log_info "Created go.mod"
    else
        log_error "Failed to generate Go bindings"
        return 1
    fi
}

generate_lua() {
    log_info "Generating Lua bindings with XFeats..."
    
    "$SWIG" -lua \
        -I"$INCLUDE_DIR" \
        -outdir "$LUA_OUT" \
        -o "$LUA_OUT/equinox_wrap.c" \
        "$INTERFACE_FILE"
    
    if [[ $? -eq 0 ]]; then
        log_success "Lua bindings generated"
    else
        log_error "Failed to generate Lua bindings"
        return 1
    fi
}

generate_perl() {
    log_info "Generating Perl bindings with XFeats..."
    
    "$SWIG" -perl5 \
        -I"$INCLUDE_DIR" \
        -outdir "$PERL_OUT" \
        -o "$PERL_OUT/equinox_wrap.c" \
        "$INTERFACE_FILE"
    
    if [[ $? -eq 0 ]]; then
        log_success "Perl bindings generated"
        
        # Create Makefile.PL
        cat > "$PERL_OUT/Makefile.PL" << 'EOF'
use ExtUtils::MakeMaker;

WriteMakefile(
    NAME         => 'Equinox',
    VERSION      => '0.1.0',
    LIBS         => ['-L../../build -lequinox'],
    INC          => '-I../../include',
    OBJECT       => 'equinox_wrap.o',
);
EOF
        
        log_info "Created Makefile.PL"
    else
        log_error "Failed to generate Perl bindings"
        return 1
    fi
}

generate_javascript() {
    log_info "Generating JavaScript (Node.js) bindings with XFeats..."
    
    "$SWIG" -javascript -node \
        -I"$INCLUDE_DIR" \
        -outdir "$JS_OUT" \
        -o "$JS_OUT/equinox_wrap.cxx" \
        "$INTERFACE_FILE"
    
    if [[ $? -eq 0 ]]; then
        log_success "JavaScript bindings generated"
        
        # Create binding.gyp
        cat > "$JS_OUT/binding.gyp" << 'EOF'
{
  "targets": [
    {
      "target_name": "equinox",
      "sources": [ "equinox_wrap.cxx" ],
      "include_dirs": [
        "../../include"
      ],
      "libraries": [
        "-L../../build",
        "-lequinox"
      ],
      "cflags": [ "-std=c++11", "-O2" ]
    }
  ]
}
EOF
        
        # Create package.json
        cat > "$JS_OUT/package.json" << 'EOF'
{
  "name": "equinox",
  "version": "0.1.0",
  "description": "XFeats-enhanced Node.js bindings for Equinox E-graph library",
  "main": "equinox.js",
  "scripts": {
    "install": "node-gyp rebuild"
  },
  "keywords": ["equinox", "egraph", "xfeats"],
  "license": "MIT",
  "gypfile": true
}
EOF
        
        log_info "Created binding.gyp and package.json"
    else
        log_error "Failed to generate JavaScript bindings"
        return 1
    fi
}

# ============================================================================
# Build Summary
# ============================================================================

generate_summary() {
    local summary_file="$OUT_DIR/BINDINGS_SUMMARY.md"
    
    cat > "$summary_file" << EOF
# Equinox Bindings Generation Summary

**Generated:** $(date)
**XFeats Version:** $XFEATS_VERSION
**SWIG Version:** $("$SWIG" -version | grep -oP 'SWIG Version \K[0-9.]+')

## Applied XFeats

### Universal XFeats
- ✓ null_check_guard
- ✓ string_conversion
- ✓ buffer_to_array
- ✓ enum_mapping
- ✓ opaque_pointer
- ✓ size_t_safe
- ✓ eager_validation
- ✓ range_check
- ✓ default_arguments
- ✓ error_code_to_bool
- ✓ documentation_injection
- ✓ symbol_renaming
- ✓ auto_free_return
- ✓ iterator_adapter
- ✓ index_access
- ✓ equality_operator
- ✓ string_repr
- ✓ hash_support
- ✓ clone_method

### Super XFeats
- ✓ super_safe_core
- ✓ super_developer_friendly

## Generated Bindings

| Language   | Output Directory | Build File |
|------------|------------------|------------|
| Python     | \`python/\`      | setup.py   |
| Java       | \`java/\`        | pom.xml    |
| C#         | \`csharp/\`      | Equinox.csproj |
| Ruby       | \`ruby/\`        | extconf.rb |
| Go         | \`go/\`          | go.mod     |
| Lua        | \`lua/\`         | -          |
| Perl       | \`perl/\`        | Makefile.PL |
| JavaScript | \`javascript/\`  | binding.gyp |

## Building Bindings

### Python
\`\`\`bash
cd python
python setup.py build
python setup.py install
\`\`\`

### Java
\`\`\`bash
cd java
mvn package
\`\`\`

### C#
\`\`\`bash
cd csharp
dotnet build
\`\`\`

### Ruby
\`\`\`bash
cd ruby
ruby extconf.rb
make
\`\`\`

### Go
\`\`\`bash
cd go
go build
\`\`\`

### Perl
\`\`\`bash
cd perl
perl Makefile.PL
make
\`\`\`

### JavaScript
\`\`\`bash
cd javascript
npm install
\`\`\`

## Notes

- All bindings include XFeats safety features (null checks, validation)
- Memory management is handled automatically where possible
- Idiomatic naming conventions applied per language
- Full documentation injected from C API

EOF
    
    log_success "Summary written to $summary_file"
}

# ============================================================================
# Main Execution
# ============================================================================

main() {
    log_info "Equinox Bindings Generator (XFeats $XFEATS_VERSION)"
    echo
    
    # Validation
    check_swig
    check_interface
    
    # Setup
    create_output_dirs
    
    # Parse arguments
    local languages=()
    if [[ $# -eq 0 ]]; then
        languages=(python java csharp ruby go lua perl javascript)
    else
        languages=("$@")
    fi
    
    # Generate bindings
    local failed=0
    for lang in "${languages[@]}"; do
        case "$lang" in
            python)     generate_python || ((failed++)) ;;
            java)       generate_java || ((failed++)) ;;
            csharp|cs)  generate_csharp || ((failed++)) ;;
            ruby)       generate_ruby || ((failed++)) ;;
            go)         generate_go || ((failed++)) ;;
            lua)        generate_lua || ((failed++)) ;;
            perl)       generate_perl || ((failed++)) ;;
            javascript|js|node) generate_javascript || ((failed++)) ;;
            *)
                log_warning "Unknown language: $lang"
                ((failed++))
                ;;
        esac
        echo
    done
    
    # Summary
    generate_summary
    
    # Final status
    echo
    if [[ $failed -eq 0 ]]; then
        log_success "All bindings generated successfully!"
        log_info "Output directory: $OUT_DIR"
        return 0
    else
        log_error "$failed language(s) failed to generate"
        return 1
    fi
}

# Run main with all arguments
main "$@"
