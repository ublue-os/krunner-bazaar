# krunner-bazaar

A KRunner plugin for searching and installing Flatpak applications through the [Bazaar](https://github.com/kolunmi/bazaar) store.

## Installation

### Fedora (copr)

```bash
sudo dnf5 copr enable copr.fedorainfracloud.org/ublue-os/packages
sudo dnf5 install krunner-bazaar
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

### Internationalization

The plugin supports multiple languages. To contribute translations. See `po/README.md` for detailed translation instructions.

### Release Process

1. Create and push a version tag:
   ```bash
   just bump-version 1.x.x
   ```

2. GitHub Actions will automatically:
   - Build the RPM packages
   - Create a GitHub release
   - Upload RPM artifacts with checksums

## Implementation Details

This plugin implements the GNOME Shell Search Provider 2 interface to communicate with Bazaar. When you select a result, it calls the `ActivateResult` method which triggers Bazaar to open and show the selected application.

References:
- [Bazaar's D-Bus API](https://github.com/kolunmi/bazaar/blob/master/src/shell-search-provider-dbus-interfaces.xml)
- [Bazaar's Search Provider Implementation](https://github.com/kolunmi/bazaar/blob/master/src/bz-gnome-shell-search-provider.c)
