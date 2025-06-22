image_name := "localhost/krunner-bazaar:dev"
default: build

# Build the development container image
build-container:
    #!/usr/bin/env bash
    echo "Building development container image..."
    podman build -t {{image_name}} -f Containerfile .
    echo "✓ Development container built: {{image_name}}"

# Build the project using the development container
build: build-container
    #!/usr/bin/env bash
    set -euo pipefail
    
    echo "Building krunner-bazaar..."
    
    # Create build directory if it doesn't exist
    mkdir -p build
    
    # Build using development container
    podman run --rm \
        --userns=keep-id \
        --volume "$(pwd):/workspace" \
        --workdir /workspace \
        {{image_name}} \
        bash -c '
            set -euo pipefail
            # Configure and build the project
            cd build
            cmake .. \
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Release} \
                -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX:-/usr}
            
            make -j$(nproc)
        '

# Clean build artifacts
clean:
    rm -rf build/ build-debug/

install: build
    #!/usr/bin/env bash
    set -euo pipefail

    # Install using the development container with host system access
    podman run --rm \
        --userns=keep-id \
        --volume "$(pwd):/workspace" \
        --workdir /workspace/build \
        {{image_name}} \
        bash -c '
            # Install to host system
            echo "Installing to host system..."
            mkdir -p /workspace/build/prefix
            make install DESTDIR=/workspace/build/prefix
        '
    
    sudo cp build/prefix/usr/lib64/qt6/plugins/kf6/krunner/bazaarrunner.so /usr/lib64/qt6/plugins/kf6/krunner/bazaarrunner.so


# Debug bazaar-dbus-tool with GDB
gdb:
    gdb -ex "run" -ex "bt" --args ./build/bin/bazaar-dbus-tool -s spotify

# Quick rebuild and install for testing
quick-install: build
    #!/usr/bin/env bash
    echo "Quick install for testing..."
    sudo cp build/src/libbazaarrunner.so /usr/lib64/qt6/plugins/krunner/
    sudo cp src/bazaarrunner.json /usr/lib64/qt6/plugins/krunner/
    echo "Plugin installed! Restart KRunner to test changes."
    echo "Run: kquitapp6 krunner && krunner --replace"

# Restart KRunner after plugin changes
restart-krunner:
    #!/usr/bin/env bash
    echo "Restarting KRunner..."
    kquitapp6 krunner 2>/dev/null || true
    sleep 1
    krunner --replace &
    echo "KRunner restarted"


# Enter development container shell
shell: build-container
    #!/usr/bin/env bash
    echo "Starting development shell..."
    podman run --rm -it \
        --userns=keep-id \
        --volume "$(pwd):/workspace" \
        --workdir /workspace \
        {{image_name}} \
        /bin/bash


# Test the D-Bus interface (requires Bazaar to be running)
test-dbus:
    #!/usr/bin/env bash
    echo "Testing Bazaar D-Bus interface..."
    
    # Check if Bazaar service is available
    if busctl --user list | grep -q "io.github.kolunmi.bazaar"; then
        echo "✓ Bazaar D-Bus service is running"
        
        # Test GetInitialResultSet
        echo "Testing search for 'firefox'..."
        busctl --user call \
            io.github.kolunmi.bazaar \
            /io/github/kolunmi/bazaar/SearchProvider \
            org.gnome.Shell.SearchProvider2 \
            GetInitialResultSet \
            as 1 "firefox"
    else
        echo "✗ Bazaar D-Bus service not found. Make sure Bazaar is running."
        echo "Available services:"
        busctl --user list | grep -i bazaar || echo "No Bazaar services found"
    fi

# Debug KRunner plugin installation and loading
debug-krunner:
    #!/usr/bin/env bash
    echo "=== KRunner Plugin Debug Information ==="
    
    echo "1. Checking plugin installation..."
    PLUGIN_PATHS=(
        "/usr/lib64/qt6/plugins/krunner"
        "/usr/lib/qt6/plugins/krunner"
        "$HOME/.local/lib/qt6/plugins/krunner"
        "/usr/local/lib64/qt6/plugins/krunner"
    )
    
    for path in "${PLUGIN_PATHS[@]}"; do
        if [ -d "$path" ]; then
            echo "Checking $path:"
            ls -la "$path" | grep bazaar || echo "  No bazaar plugin found"
        else
            echo "$path: Directory does not exist"
        fi
    done
    
    echo ""
    echo "2. Checking plugin metadata..."
    find /usr -name "bazaarrunner.json" 2>/dev/null || echo "bazaarrunner.json not found"
    
    echo ""
    echo "3. Checking Qt plugin paths..."
    echo "QT_PLUGIN_PATH: $QT_PLUGIN_PATH"
    
    echo ""
    echo "4. Testing plugin loading with krunner..."
    echo "Starting krunner in debug mode (check console output)..."
    echo "Run: QT_LOGGING_RULES='*.debug=true' krunner --replace"
    echo "Or: QT_LOGGING_RULES='kf.runner.core.debug=true' krunner --replace"
    
    echo ""
    echo "5. Checking if Bazaar is running..."
    if pgrep -f bazaar > /dev/null; then
        echo "✓ Bazaar process is running"
        if busctl --user list | grep -q "io.github.kolunmi.bazaar"; then
            echo "✓ Bazaar D-Bus service is available"
        else
            echo "✗ Bazaar D-Bus service not found"
        fi
    else
        echo "✗ Bazaar is not running"
    fi

# Start KRunner in debug mode
debug-krunner-start:
    #!/usr/bin/env bash
    echo "Starting KRunner in debug mode..."
    echo "Watch for BazaarRunner debug messages..."
    QT_LOGGING_RULES='*.debug=true' krunner --replace

# Check KRunner logs
debug-logs:
    #!/usr/bin/env bash
    echo "Recent KRunner-related logs:"
    journalctl --user -n 50 | grep -i krunner || echo "No recent KRunner logs found"
    echo ""
    echo "To monitor live logs, run:"
    echo "journalctl --user -f | grep -E '(krunner|BazaarRunner)'"

# Show project information
info:
    @echo "krunner-bazaar - KRunner plugin for Bazaar Flatpak store"
    @echo "=================================================="
    @echo "Build system: CMake"
    @echo "Target: KDE Frameworks 6 / Qt 6"
    @echo "Container: {{image_name}}"
    @echo ""
    @echo "Available recipes:"
    @echo "  build-container     - Build the development container image"
    @echo "  build               - Build the project in container"
    @echo "  build-debug         - Build with debug symbols"
    @echo "  install             - Build and install the plugin"
    @echo "  clean               - Clean build artifacts"
    @echo "  shell               - Enter development container shell"
    @echo "  debug-user          - Debug user mapping in container"
    @echo "  debug-krunner       - Debug KRunner plugin installation"
    @echo "  debug-krunner-start - Start KRunner in debug mode"
    @echo "  debug-logs          - Check KRunner logs"
    @echo "  test-dbus           - Test Bazaar D-Bus interface"
    @echo "  check-req           - Check system requirements"
    @echo "  clean-container     - Remove development container and image"
    @echo "  info                - Show this information"
