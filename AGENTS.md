Map Panel Recap (Current)

- Map is a native dock/split panel in the main plot layout (same split workflow as standard plots), not a toolbox window.
- Plot context menu now includes `Add Map View` (creates a new split map panel) and existing XY-only `Convert to Map panel`.
- Map panel state (lat/lon selections) is persisted in dock XML and restored with layout loading.
- Map panel receives time updates during tracker moves/playback and updates marker position in sync.

UX and Interaction Changes

- Latitude/Longitude autodetection is intentionally simple: keyword match for `Latitude` and `Longitude` only.
- Removed the auto-detect checkbox; detection now runs as part of refresh/fit flow.
- Top-right button is `Fit to View` and performs refresh + detect + route fit.
- `Fit to View` button includes `zoom_max` icon.
- Right-click inside the map is forwarded to app code (WebEngine default context menu suppressed).
- Map right-click menu now includes:
	- `Fit to View` (icon: `zoom_max.svg`)
	- `Split Horizontally` (icon: `add_column.svg`)
	- `Split Vertically` (icon: `add_row.svg`)
	- `Add Map View` (icon: `scatter.svg`)

Key Files

- `plotjuggler_app/map_dock_panel.h`
- `plotjuggler_app/map_dock_panel.cpp`
- `plotjuggler_app/plot_docker.h`
- `plotjuggler_app/plot_docker.cpp`
- `plotjuggler_app/plotwidget.h`
- `plotjuggler_app/plotwidget.cpp`
- `plotjuggler_app/CMakeLists.txt`

Notes

- WebEngine remains optional (`PJ_HAS_WEBENGINE`); map panel falls back to informational text when unavailable.
- Tile provider is configurable via environment variables in map HTML generation:
	- `PJ_MAP_TILES_URL`
	- `PJ_MAP_ATTRIBUTION`
- Avoid defaulting to `tile.openstreetmap.org` in app code to prevent policy/403 issues.

Building

cmake -S . -B build\PlotJuggler-webmap `
>>   -G "Visual Studio 17 2022" -A x64 `
>>   -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake `
>>   -DCMAKE_INSTALL_PREFIX="$PWD\install" `
>>   -DQt5_DIR="C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5" `
>>   -DQt5WebEngineWidgets_DIR="C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5WebEngineWidgets"

cmake --build build\PlotJuggler-webmap --config Release --target install