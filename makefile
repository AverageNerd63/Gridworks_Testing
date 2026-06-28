SHELL := cmd

CC       := clang
CXX      := clang++
OUTDIR   := build
DOTNET   := dotnet
SLN      := Gridworks.sln
PLATFORM := win32
CFLAGS   := -Wall -Wextra -std=c11  -Isrc -Ithird_party/imgui -Ithird_party -I"$(VULKAN_SDK)/Include" -DVK_NO_PROTOTYPES -D_CRT_SECURE_NO_WARNINGS -g
CXXFLAGS := -Wall -Wextra -std=c++17 -Isrc -Ithird_party/imgui -Ithird_party -I"$(VULKAN_SDK)/Include" -DVK_NO_PROTOTYPES -D_CRT_SECURE_NO_WARNINGS -g
LDFLAGS  := -lkernel32 -luser32 -lgdi32

rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# C sources only
ALL_C_SRCS := $(call rwildcard,src,*.c)
C_SRCS     := $(filter-out src/platform/linux/% src/platform/macos/%,$(ALL_C_SRCS))
C_OBJS     := $(patsubst src/%.c,$(OUTDIR)/obj/%.o,$(C_SRCS))

# C++ sources — src/ files and third_party imgui separately
SRC_CXX_SRCS := src/ui/imgui_bridge.cpp src/ui/editor_ui.cpp
SRC_CXX_OBJS := $(patsubst src/%.cpp,$(OUTDIR)/obj/%.o,$(SRC_CXX_SRCS))

IMGUI_SRCS := third_party/imgui/imgui.cpp \
              third_party/imgui/imgui_draw.cpp \
              third_party/imgui/imgui_tables.cpp \
              third_party/imgui/imgui_widgets.cpp \
              third_party/imgui/backends/imgui_impl_vulkan.cpp \
              third_party/imgui/backends/imgui_impl_win32.cpp \
              third_party/ImGuizmo/ImGuizmo.cpp

IMGUI_OBJS := $(patsubst third_party/%.cpp,$(OUTDIR)/obj/third_party/%.o,$(IMGUI_SRCS))

CXX_OBJS := $(SRC_CXX_OBJS) $(IMGUI_OBJS)
OBJS     := $(C_OBJS) $(CXX_OBJS)

SHADER_SRC_DIR := assets/shaders
SHADER_OUT_DIR := build/shaders
VERT_SRCS := $(wildcard $(SHADER_SRC_DIR)/*.vert)
FRAG_SRCS := $(wildcard $(SHADER_SRC_DIR)/*.frag)
SPVS      := $(patsubst $(SHADER_SRC_DIR)/%.vert,$(SHADER_OUT_DIR)/%.vert.spv,$(VERT_SRCS)) \
             $(patsubst $(SHADER_SRC_DIR)/%.frag,$(SHADER_OUT_DIR)/%.frag.spv,$(FRAG_SRCS))

bs  = $(subst /,\,$1)
mkd = if not exist "$(call bs,$(patsubst %/,%,$1))" md "$(call bs,$(patsubst %/,%,$1))"

.PHONY: all native managed shaders compdb clean

all: shaders $(OBJS) compdb
	@$(call mkd,$(OUTDIR))
	@echo [link] $(OUTDIR)/gridworks.exe
	@$(CXX) $(CXXFLAGS) -o $(OUTDIR)/gridworks.exe $(OBJS) $(LDFLAGS)
	@echo [done] native
	@echo [dotnet] building managed...
	@$(DOTNET) build $(SLN) -c Debug --nologo -v quiet
	@echo [done] managed

native: shaders $(OBJS) compdb
	@$(call mkd,$(OUTDIR))
	@echo [link] $(OUTDIR)/gridworks.exe
	@$(CXX) $(CXXFLAGS) -o $(OUTDIR)/gridworks.exe $(OBJS) $(LDFLAGS)
	@echo [done] native

managed:
	@echo [dotnet] building managed...
	@$(DOTNET) build $(SLN) -c Debug --nologo -v quiet
	@echo [done] managed

shaders: $(SPVS)

compdb: $(OBJS)
	@echo [compdb] merging compile_commands.json
	@python -c "import glob; frags=[open(f).read().rstrip().rstrip(',') for f in glob.glob('build/obj/**/*.json',recursive=True)]; open('compile_commands.json','w').write('['+','.join(frags)+']')"

$(SHADER_OUT_DIR)/%.vert.spv: $(SHADER_SRC_DIR)/%.vert
	@$(call mkd,$(SHADER_OUT_DIR))
	@glslc -o $@ $<
	@echo [spv]   $@

$(SHADER_OUT_DIR)/%.frag.spv: $(SHADER_SRC_DIR)/%.frag
	@$(call mkd,$(SHADER_OUT_DIR))
	@glslc -o $@ $<
	@echo [spv]   $@

$(OUTDIR)/obj/%.o: src/%.c
	@$(call mkd,$(dir $@))
	@echo [cc]   $<
	@$(CC) $(CFLAGS) -MJ "$(OUTDIR)/obj/$*.json" -c -o $@ $<

$(OUTDIR)/obj/%.o: src/%.cpp
	@$(call mkd,$(dir $@))
	@echo [c++]  $<
	@$(CXX) $(CXXFLAGS) -MJ "$(OUTDIR)/obj/$*.json" -c -o $@ $<

$(OUTDIR)/obj/third_party/%.o: third_party/%.cpp
	@$(call mkd,$(dir $@))
	@echo [c++]  $<
	@$(CXX) $(CXXFLAGS) -MJ "$(OUTDIR)/obj/third_party/$*.json" -c -o $@ $<

clean:
	@echo [clean] removing build artifacts...
	@if exist "$(call bs,$(OUTDIR))" rmdir /s /q "$(call bs,$(OUTDIR))"
	@$(DOTNET) clean $(SLN) --nologo -v quiet
	@echo [clean] done