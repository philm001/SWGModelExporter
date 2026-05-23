# SWGModelExporter repository instructions

## Build, test, and lint commands

- This is a Windows Visual Studio/MSBuild C++20 console project. The solution builds `SWGModelExporter` plus the sibling `..\DirectXTex\DirectXTex\DirectXTex_Desktop_2022_Win10.vcxproj`.
- External prerequisites are manual, not package-managed: install Autodesk FBX SDK 2020.3.7, extract `DirectXTex-main.zip` as `..\DirectXTex`, extract `boost.7z` into `Boost/`, and extract `lib.7z` into `lib/`. The README still mentions older FBX SDK paths; prefer the 2020.3.7 paths currently referenced by `SWGModelExporter.vcxproj`.
- Build from a VS 2022 Developer Command Prompt:
  - `msbuild SWGModelExporter.sln /m /p:Configuration=Debug /p:Platform=x64`
  - `msbuild SWGModelExporter.sln /m /p:Configuration=Release /p:Platform=x64`
- There is no automated test suite or lint target checked into this repository.
- The narrowest runtime validation is a targeted extractor run, but note that the checked-in source currently defines `DEBUG_MODE` at the top of `SWGModelExporter.cpp`, so CLI arguments are bypassed until that macro is removed or gated. In the current state, the fastest validation is to edit the hardcoded `swg_path`, `output_pathname`, and `object_name` values in `SWGModelExporter.cpp` and run the built executable. If `DEBUG_MODE` is disabled, a targeted run looks like:
  - `x64\Release\SWGModelExporter.exe --swg-path C:\SWG --object appearance/mesh/acklay_l0.mgn --output-path C:\extraction --overwrite-result 1 --verbose`
- Batch exports are driven by `batch:<ext>` object names. The main top-level entry points called out in `SWGModelExporter.cpp` are `batch:apt`, `batch:sat`, and `batch:pob`.

## High-level architecture

- `SWGModelExporter.cpp` is the orchestration entry point. It sets debug/runtime flags, opens the SWG TRE library, resolves either a single asset or a batch selection, and then runs each request through `SWGMainObject::beginParsingProcess()`, `resolveDependecies()`, and `storeObject()`.
- `tre_library.*` is the resource index over all `.tre` archives under the SWG install path. It supports exact lookup, partial-name lookup, and extension-based batch selection.
- `IFF_file.*` is the low-level chunk walker for SWG IFF data. `Parser_selector` inspects the top-level FORM tag at depth 1 and dispatches to the right parser in `parsers/` (`cat`, `lmg`, `mgn`, `skt`, `sht`, `anim`, `latt`, `apt`, `pob`, plus the static `DTLA`/`MESH` path in `mesh_file.*`).
- Parsed assets are `Base_object` implementations stored in `Context.object_list`. Dependency discovery is graph-driven: each parsed object advertises follow-on resources via `get_referenced_objects()`, and `SWGMainObject` keeps provenance in `Context.opened_by` so dependency resolution can later wire meshes to shaders, skeletons, animation maps, and attachment skeletons.
- The animated export path is centered on `SWGMainObject`, but the implementation is intentionally split across multiple `.cpp` files:
  - `SWGMainObject.cpp` handles parse queueing and quaternion helpers.
  - `SWGDependencyResolver.cpp` wires parsed objects together after the graph is loaded.
  - `SWGFileAccess.cpp` performs the final object export dispatch.
  - `SWGSkeletonExport.cpp` creates the FBX armature, clusters, and bind pose.
  - `SWGAnimationParsing.cpp` authors FBX animation stacks and curves.
- Static mesh export is separate from the animated path. `mesh_file.*` owns `DTLAFORM`/`MESHFORM` parsing plus FBX export for static meshes and LOD-only resources.
- `objects/animated_object.*` contains the in-memory domain objects shared by parsers and exporters, and it still includes legacy skeletal export/math logic. When changing skeleton or animation behavior, check both the newer `SWGMainObject` pipeline and the older `objects/animated_object.*` implementation for drift.

## Key conventions

- Normalize SWG asset names to forward-slash paths before lookup. The queueing and TRE lookup code assumes normalized resource names and usually reasons about file type by the last 3 characters.
- New asset support should follow the existing parser/object contract instead of adding special cases to the entry point: add an `IFF_visitor` parser, return a `Base_object`, and implement `get_referenced_objects()` plus `resolve_dependencies()` so the queue-based dependency walk keeps working.
- `p_CompleteModels` is grouped by LOD, and LOD 0 is the canonical skeletal/animation path. `--no-lod` and `--no-lod-animation` are global switches routed through `DebugConfig`, not ad hoc checks scattered through exporters.
- Use `INIT_LOGGER` and the `LOG_*` macros for diagnostics. Deep tracing is expected to stay behind `DebugConfig::shouldLog()` so normal runs do not flood stdout.
- FBX skeleton export intentionally creates an `Armature` node as a sibling of the mesh node and drives bones through local TRS values instead of FBX pre/post rotations. That setup is deliberate to avoid Blender import cycles and rotation conflicts.
- Rotation math is a cross-file convention, not a local implementation detail. The bind pose path in `SWGSkeletonExport.cpp`, the animation path in `SWGAnimationParsing.cpp`/`SWGMainObject.cpp`, and the legacy helpers in `objects/animated_object.cpp` all need to stay logically aligned when quaternion/Euler handling changes.
- Shader export assumes referenced DDS textures will also be exported and rewritten to `.tga` paths in the output tree. Material texture hookups in the FBX exporter depend on that conversion side effect.
- Batch mode is extension-based on the last 3 characters, so some file types use non-obvious tokens; for example, `.latt` assets are selected with `batch:att`.
