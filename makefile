SHELL := cmd

CC       := clang
OUTDIR   := build
DOTNET   := dotnet
SLN      := Gridworks.sln
PLATFORM := win32
CFLAGS   := -Wall -Wextra -std=c11 -Isrc -I"$(VULKAN_SDK)/Include" -DVK_NO_PROTOTYPES -D_CRT_SECURE_NO_WARNINGS -g
LDFLAGS  := -lkernel32 -luser32 -lgdi32

rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

ALL_SRCS := $(call rwildcard,src,*.c)
SRCS     := $(filter-out src/platform/linux/% src/platform/macos/%,$(ALL_SRCS))
OBJS     := $(patsubst src/%.c,$(OUTDIR)/obj/%.o,$(SRCS))

SHADER_SRC_DIR := assets/shaders
SHADER_OUT_DIR := build/shaders
VERT_SRCS := $(wildcard $(SHADER_SRC_DIR)/*.vert)
FRAG_SRCS := $(wildcard $(SHADER_SRC_DIR)/*.frag)
SPVS      := $(patsubst $(SHADER_SRC_DIR)/%.vert,$(SHADER_OUT_DIR)/%.vert.spv,$(VERT_SRCS)) \
             $(patsubst $(SHADER_SRC_DIR)/%.frag,$(SHADER_OUT_DIR)/%.frag.spv,$(FRAG_SRCS))

bs  = $(subst /,\,$1)
mkd = if not exist "$(call bs,$(patsubst %/,%,$1))" md "$(call bs,$(patsubst %/,%,$1))"

.PHONY: all native managed shaders clean

all: shaders $(OBJS) compdb
	@$(call mkd,$(OUTDIR))
	@echo [link] $(OUTDIR)/gridworks.exe
	@$(CC) $(CFLAGS) -o $(OUTDIR)/gridworks.exe $(OBJS) $(LDFLAGS)
	@echo [done] native
	@echo [dotnet] building managed...
	@$(DOTNET) build $(SLN) -c Debug --nologo -v quiet
	@echo [done] managed

native: shaders $(OBJS) compdb
	@$(call mkd,$(OUTDIR))
	@echo [link] $(OUTDIR)/gridworks.exe
	@$(CC) $(CFLAGS) -o $(OUTDIR)/gridworks.exe $(OBJS) $(LDFLAGS)
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

clean:
	@echo [clean] removing build artifacts...
	@if exist "$(call bs,$(OUTDIR))" rmdir /s /q "$(call bs,$(OUTDIR))"
	@$(DOTNET) clean $(SLN) --nologo -v quiet
	@echo [clean] done