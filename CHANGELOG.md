# Changelog

## 0.1.0

### Added

- Initial CMake project for `rvdata2_extractor` (C++17).
- `cmake/FindDependencies.cmake` to locate zlib via `find_package(ZLIB)`, keeping the root `CMakeLists.txt` lean.
- Extract path for RPG Maker VX Ace `Scripts.rvdata2` into numbered `.rb` files under `Scripts/`.

### Changed

- Emit the `rvdata2_extractor` binary into the repository root via `RUNTIME_OUTPUT_DIRECTORY`, so it can be run next to `Scripts.rvdata2` without copying out of `build/`.
- Skip writing empty script bodies during extract (section headers and blank entries produce no `.rb` file).
- Record each extracted script’s marshal id, original name, and relative path in `scripts.tree`, drop the numeric filename prefix, and group scripts into directories named from `▼` section titles. Section source is ignored, with a warning when that body is non-empty.
- Mount `scripts.tree` on extract so known marshal ids overwrite their stored `.rb` paths, new ids get reserved-unique paths, and the tree is rewritten in current list order.
- Enable `CMAKE_EXPORT_COMPILE_COMMANDS` so CMake writes `build/compile_commands.json` for ccls and other IDE language servers.
