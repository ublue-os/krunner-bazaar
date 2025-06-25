image_name := "localhost/krunner-bazaar:dev"
default: build

build-container:
    #!/usr/bin/env bash
    echo "Building development container image..."
    podman build -t {{image_name}} -f Containerfile .

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

gdb:
    gdb -ex "run" -ex "bt" --args ./build/bin/bazaar-dbus-tool -s spotify

rpm: build-container
    #!/usr/bin/env bash
    set -euo pipefail
    # Get project version from CMakeLists.txt or use default
    VERSION=${VERSION:-1.0.0}

    # Create RPM build structure
    mkdir -p rpmbuild/{SOURCES,SPECS,BUILD,RPMS,SRPMS}

    # Create source tarball
    echo "Creating source tarball..."
    git archive --format=tar.gz --prefix=krunner-bazaar-${VERSION}/ HEAD > rpmbuild/SOURCES/krunner-bazaar-${VERSION}.tar.gz

    # Copy spec file
    cp krunner-bazaar.spec rpmbuild/SPECS/

    # Build RPM using development container
    podman run --rm \
        --userns=keep-id \
        --volume "$(pwd):/workspace" \
        --workdir /workspace \
        {{image_name}} \
        bash -c "
            set -euo pipefail
            
            # Build the RPM
            rpmbuild --define '_topdir /workspace/rpmbuild' \
                     --define 'version ${VERSION}' \
                     -ba rpmbuild/SPECS/krunner-bazaar.spec
        "

    echo "RPM build complete!"
    echo "Built packages:"
    find rpmbuild/RPMS -name "*.rpm" -type f
    find rpmbuild/SRPMS -name "*.rpm" -type f