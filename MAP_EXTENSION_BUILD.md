# Map Extension Build and Launch Notes

This repository has a local PlotJuggler build with the native map panel extension enabled.

## Launching the Map-Enabled Build

Run the installed executable from the repository root:

```powershell
.\install\bin\plotjuggler.exe
```

Full path:

```powershell
C:\Users\caleb\EV\_TREV4\github\PlotJugglerPlus\install\bin\plotjuggler.exe
```

The adjacent `QtWebEngineProcess.exe` is required by Qt WebEngine, but it is not launched directly.

## Rebuilding

Configure the build:

```powershell
cmake -S . -B build\PlotJuggler-webmap `
  -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake `
  -DCMAKE_INSTALL_PREFIX="$PWD\install" `
  -DQt5_DIR="C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5" `
  -DQt5WebEngineWidgets_DIR="C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5WebEngineWidgets"
```

Build and install:

```powershell
cmake --build build\PlotJuggler-webmap --config Release --target install
```

After the install target finishes, launch:

```powershell
.\install\bin\plotjuggler.exe
```

## Map Tile Configuration

The app does not default to `tile.openstreetmap.org`, to avoid tile policy and 403 issues.
Set a tile provider before launching if the map needs online tiles:

```powershell
$env:PJ_MAP_TILES_URL="https://your.tile.server/{z}/{x}/{y}.png"
$env:PJ_MAP_ATTRIBUTION="Your attribution text"
.\install\bin\plotjuggler.exe
```

## Map Panel Features

- `Add Map View` is available from the plot context menu and creates a native split/dock map panel.
- `Convert to Map panel` remains available for XY-only plots.
- Latitude and longitude selections are persisted in dock XML and restored with saved layouts.
- Map markers update in sync with tracker movement and playback.
- `Fit to View` refreshes the route, runs simple latitude/longitude detection, and fits the visible route.
- Latitude/longitude autodetection intentionally matches only `Latitude` and `Longitude` keywords.
- WebEngine is optional. If it is unavailable, the map panel shows informational fallback text.

## Useful Files

- `plotjuggler_app/map_dock_panel.h`
- `plotjuggler_app/map_dock_panel.cpp`
- `plotjuggler_app/plot_docker.h`
- `plotjuggler_app/plot_docker.cpp`
- `plotjuggler_app/plotwidget.h`
- `plotjuggler_app/plotwidget.cpp`
- `plotjuggler_app/CMakeLists.txt`
