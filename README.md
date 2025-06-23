# krunner-bazaar

A KRunner plugin for searching and installing Flatpak applications through the [Bazaar](https://github.com/kolunmi/bazaar) store.

## Building

Requires Podman. Tested on [Aurora](https://getaurora.dev).

```bash
just build
just install
```

## Implementation Details

This plugin implements the GNOME Shell Search Provider 2 interface to communicate with Bazaar. When you select a result, it calls the `ActivateResult` method which triggers Bazaar to open and show the selected application.

References:
- [Bazaar's D-Bus API](https://github.com/kolunmi/bazaar/blob/master/src/shell-search-provider-dbus-interfaces.xml)
- [Bazaar's Search Provider Implementation](https://github.com/kolunmi/bazaar/blob/master/src/bz-gnome-shell-search-provider.c)
