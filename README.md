# krunner-bazaar

A KRunner plugin for searching and installing Flatpak applications through the Bazaar store.

## Building

```bash
mkdir build
cd build
cmake ..
make
sudo make install
```

## Implementation References
- [Bazaar's D-Bus API](https://github.com/kolunmi/bazaar/blob/master/src/shell-search-provider-dbus-interfaces.xml)
- https://github.com/kolunmi/bazaar/blob/master/src/bz-gnome-shell-search-provider.c
