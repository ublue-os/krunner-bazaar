# Containerfile for krunner-bazaar development environment
FROM fedora:43@sha256:ef3f778641a15ea30551806395ca87217379a0040a20e08e1ceb989958e0db27

LABEL description="Development environment for krunner-bazaar KDE plugin"
LABEL maintainer="Adam Fidel <adam@fidel.cloud>"

# Update system and install build dependencies
RUN dnf5 update -y && \
    dnf5 install -y \
        # Core build tools
        cmake \
        make \
        gcc \
        gcc-c++ \
        # RPM build tools
        rpm-build \
        rpmdevtools \
        # KDE Frameworks 6 development packages
        extra-cmake-modules \
        kf6-krunner-devel \
        kf6-ki18n-devel \
        kf6-kcoreaddons-devel \
        kf6-kdbusaddons  \
        # Qt6 development packages
        qt6-qtbase-devel \
        dbus-tools \
        # Development utilities
        sudo \
        vim \
        && dnf5 clean all

# Set up workspace directory
RUN mkdir -p /workspace
WORKDIR /workspace

# Set environment variables for development
ENV CMAKE_BUILD_TYPE=Release
ENV CMAKE_INSTALL_PREFIX=/usr
ENV QT_QPA_PLATFORM=offscreen

# Default command
CMD ["/bin/bash"]
