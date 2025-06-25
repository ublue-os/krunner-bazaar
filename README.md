# krunner-bazaar

A KRunner plugin for searching and installing Flatpak applications through the [Bazaar](https://github.com/kolunmi/bazaar) store.

## Installation

### From RPM (Fedora/RHEL/CentOS)

Download the latest RPM from the [releases page](https://github.com/ublue-os/krunner-bazaar/releases):

```bash
# Install the main plugin
sudo dnf install krunner-bazaar-*.rpm

# Optional: Install CLI debugging tools
sudo dnf install krunner-bazaar-tools-*.rpm
```

### From Source

Requires Podman. Tested on [Aurora](https://getaurora.dev).

```bash
just build
just install
```

### Building RPM Locally

```bash
# Build RPM package
just rpm

```

## Development

### Release Process

1. Create and push a version tag:
   ```bash
   git tag v1.0.0
   git push origin v1.0.0
   ```

2. GitHub Actions will automatically:
   - Build the RPM packages
   - Create a GitHub release
   - Upload RPM artifacts with checksums

### Requirements

- KDE Frameworks 6
- Qt 6.5+
- Flatpak
- Bazaar

## Implementation Details

This plugin implements the GNOME Shell Search Provider 2 interface to communicate with Bazaar. When you select a result, it calls the `ActivateResult` method which triggers Bazaar to open and show the selected application.

References:
- [Bazaar's D-Bus API](https://github.com/kolunmi/bazaar/blob/master/src/shell-search-provider-dbus-interfaces.xml)
- [Bazaar's Search Provider Implementation](https://github.com/kolunmi/bazaar/blob/master/src/bz-gnome-shell-search-provider.c)
