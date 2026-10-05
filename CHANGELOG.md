# Changelog

## 0.1.0

### Added

- Initial CMake project for `rvdata2_extractor` (C++17).
- `cmake/FindDependencies.cmake` to locate zlib via `find_package(ZLIB)`, keeping the root `CMakeLists.txt` lean.
- Extract path for RPG Maker VX Ace `Scripts.rvdata2` into numbered `.rb` files under `Scripts/`.

### Changed

- Emit the `rvdata2_extractor` binary into the repository root via `RUNTIME_OUTPUT_DIRECTORY`, so it can be run next to `Scripts.rvdata2` without copying out of `build/`.
- Skip writing empty script bodies during extract (section headers and blank entries produce no `.rb` file).
- Enable `CMAKE_EXPORT_COMPILE_COMMANDS` so CMake writes `build/compile_commands.json` for ccls and other IDE language servers.
