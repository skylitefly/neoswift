# AGENTS.md

Guidance for AI agents working in the pilotclient repository.

## UI Strings and i18n

- **Source language is English.** All user-visible strings in C++ (`tr("...")`) and Qt Designer `.ui` files (`<string>` / `<source>`) must be written in English. Do not embed Chinese or other non-English text in source or UI files.
- **Chinese is provided via Qt translations.** Add or update entries in [`translations/neoswift_zh_CN.ts`](translations/neoswift_zh_CN.ts) with a `<translation>` for each new English source string.
- **Ship translations with UI changes.** Adding English UI text without a matching Chinese translation in `neoswift_zh_CN.ts` is incomplete work.
- **Update workflow:** After adding strings, run the project's `lupdate` target (see root `CMakeLists.txt`), edit `neoswift_zh_CN.ts`, then run `lrelease` to produce `neoswift_zh_CN.qm`.

Misc-layer strings may use `QObject::tr("...", "Context")` (for example `"CNetwork"`); GUI strings use the widget class context via `tr()` in `QObject` subclasses or `.ui` file contexts.

## Build

```bash
conan install . --output-folder=build_conan --deployer=full_deploy -pr=ci/profile_win --build=missing
cmake --preset dev-debug
cmake --build build --parallel
ctest --output-on-failure  # in build/
```
