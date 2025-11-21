FROM fedora:42@sha256:aa7befe5cfd1f0e062728c16453cd1c479d4134c7b85eac00172f3025ab0d522

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
        # Translation tools
        gettext \
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
