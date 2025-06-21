# krunner-bazaar

A KRunner plugin for searching and installing Flatpak applications through the Bazaar store.

## Features

- Search for Flatpak applications using Bazaar's D-Bus interface
- Filter out already installed applications
- Install applications directly through Bazaar
- Minimum 2-character search queries (matching Bazaar's behavior)

## Building

```bash
mkdir build
cd build
cmake ..
make
sudo make install
```

## Usage

1. Make sure Bazaar is installed and running
2. Open KRunner (Alt+Space or Alt+F2)
3. Type your search query (minimum 2 characters)
4. Select a Flatpak application from the results to install it via Bazaar

## Configuration

The plugin connects to Bazaar's D-Bus interface using:
- **Service Name**: `io.github.kolunmi.bazaar`
- **Object Path**: `/io/github/kolunmi/bazaar/SearchProvider`
- **Interface**: `org.gnome.Shell.SearchProvider2`

## Requirements

- Bazaar must be running with its search provider enabled
- The plugin requires Qt6 DBus support
- KDE Frameworks 6 (KRunner)

## Troubleshooting

If the plugin doesn't work:

1. **Check if Bazaar is running**: `ps aux | grep bazaar`
2. **Verify D-Bus service**: `busctl --user list | grep io.github.kolunmi.bazaar`
3. **Test D-Bus interface manually**: 
   ```bash
   busctl --user call io.github.kolunmi.bazaar /io/github/kolunmi/bazaar/SearchProvider org.gnome.Shell.SearchProvider2 GetInitialResultSet as 1 "firefox"
   ```
4. **Check KRunner logs**: `journalctl --user -f | grep krunner`

## Implementation Details

This plugin implements the GNOME Shell Search Provider 2 interface to communicate with Bazaar. When you select a result, it calls the `ActivateResult` method which triggers Bazaar to open and show the selected application.

The search results include:
- Application name and description
- Application icons (when available)
- Filtering of already installed applications

## Implementation References
- [Bazaar's D-Bus API](https://github.com/kolunmi/bazaar/blob/master/src/shell-search-provider-dbus-interfaces.xml)
- [Bazaar's Search Provider Implementation](https://github.com/kolunmi/bazaar/blob/master/src/bz-gnome-shell-search-provider.c)
