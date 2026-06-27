---
## 🗺️ Development Roadmap

### Phase 0 — IDE & Build System Setup
- [x] Install Vulkan SDK 1.4+ and verify `glslc` is on PATH
- [x] Install .NET 8.0 SDK
- [x] Install GCC/Clang (via MSYS2/MinGW or LLVM) and GNU Make
- [x] Configure IDE (VS Code or JetBrains) with C and C# extensions
- [x] Set up clangd for C language server support
- [x] Initialize git repository and `.gitignore` (build outputs, `.dll`, `.exe`, `obj/`)
- [x] Scaffold top-level directory structure (`src/`, `managed_src/`, `assets/`, `build/`)
- [x] Create `makefile.windows.mak` with targets for native build, managed build, and clean
- [x] Create `Gridworks.sln` with three projects: `Gridworks.Engine`, `Gridworks.Editor`, `UserProject`
- [x] Verify full build pipeline end-to-end with a Hello World `main.c` and empty C# assemblies


### Phase 1 — Foundation
- [x] Arena & pool allocators with custom memory management
- [x] 3D math library (vectors, matrices, quaternions)
- [x] Thread-safe logger
- [x] Length-prefixed string system
- [x] Work-stealing job queue
- [x] Custom container types

### Phase 2 — Platform Layer
- [x] Win32 window creation and lifecycle
- [x] Input handling (keyboard, mouse)
- [x] Timing and delta-time
- [x] Filesystem utilities
- [x] File watcher (`ReadDirectoryChangesW`)

### Phase 3 — Vulkan Renderer
- [x] Dynamic Vulkan loader
- [x] Device and queue selection
- [x] Swapchain creation and resize handling
- [x] Render pass and framebuffers
- [x] SPIR-V pipeline compilation (`glslc`)
- [x] Vertex/index buffer management
- [x] Double-buffered frame synchronization
- [x] Clustered Forward shading

### Phase 4 — ECS
- [X] Sparse-set entity component system in native C
- [X] Component registration and archetype layout
- [X] Entity create / destroy / query API

### Phase 5 — .NET Host & Interop
- [x] CoreCLR host via `hostfxr`
- [x] Collectible `AssemblyLoadContext` setup
- [x] Zero-allocation pointer exchange between C and C# ECS
- [x] C# ECS bindings (`Gridworks.Engine`)
- [x] Hot reload loop (file watcher → unload → GC → reload)

### Phase 6 — Editor Viewport
- [x] Dear ImGui (cimgui) integration
- [x] Default editor layout (viewport, hierarchy, inspector, console)
- [x] stb_truetype atlas-based text rendering
- [x] UI vertex/UV extension for editor widgets
- [x] `[EditorCustomization]` attribute — C# custom panel system (`Gridworks.Editor`)

### Phase 7 — Gizmo & Scene Interaction
- [ ] Viewport camera controls (orbit, pan, fly)
- [ ] Selection system (ray-cast pick)
- [ ] Translate / rotate / scale gizmos
- [ ] Gizmo registry (mode and overlay pools)
- [ ] Undo/redo command stack

### Phase 8 — Project Manager
- [ ] Project manager dialog (create / open project)
- [ ] `ProjectConfig` runtime structure holding user-chosen project root
- [ ] Dynamic file watcher path set at project-open time
- [ ] Dynamic .NET host load path resolving `UserProject.dll` from `ProjectConfig`
- [ ] Separation of engine SDK build from user project build
- [ ] Recent projects list with persistence
- [ ] New project scaffolding (generates `UserProject.csproj` and starter files in chosen folder)

### Phase 9 — Simulation Systems
- [ ] Rigid-body integrator
- [ ] Collider shapes (box, sphere, capsule)
- [ ] Impulse-based constraint solver
- [ ] Audio mixer via miniaudio integration
- [ ] Spatial audio (3D positional sources)

### Phase 10 — Content Pipeline
- [ ] Asset importer (meshes via cgltf, textures via stb_image)
- [ ] Compiled binary asset format with fast loading
- [ ] Shader hot-reloading (GLSL → SPIR-V at runtime)
- [ ] Asset dependency graph and incremental rebuild

### Phase 11 — Renderer Expansion
- [ ] DirectX 12 backend
- [ ] OpenGL 4.6 fallback backend
- [ ] Runtime backend swap via renderer abstraction
- [ ] Shadow mapping
- [ ] Post-processing stack (bloom, tonemapping, FXAA)
- [ ] GPU frustum and occlusion culling

### Phase 12 — Shipping
- [ ] Game runtime build (editor and dev tools stripped)
- [ ] Project export and packaging
- [ ] Linux platform abstraction layer (Wayland/X11)
