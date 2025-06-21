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

# Build and install the plugin (requires sudo)
install: build
    #!/usr/bin/env bash
    set -euo pipefail
    
    echo "Installing krunner-bazaar plugin..."
    
    # Install using the development container with host system access
    podman run --rm \
        --userns=keep-id \
        --volume "$(pwd):/workspace" \
        --volume "/usr:/host-usr" \
        --workdir /workspace/build \
        --privileged \
        {{image_name}} \
        bash -c '
            # Install to host system
            echo "Installing to host system..."
            sudo make install DESTDIR=/host-usr/..
        '
    
    echo "Plugin installed! Restart KRunner or logout/login to load the plugin."


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