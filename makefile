build_dir := build
intermediate_build_dir := $(build_dir)/intermediates
bin_build_dir := $(build_dir)/bin
hook_build_dir := $(build_dir)/hooks
build_dirs := $(bin_build_dir) $(hook_build_dir) $(intermediate_build_dir)
executable := $(bin_build_dir)/xplore
source_dir := src
hook_source_dir := $(source_dir)/hooks/assembly
sources := $(wildcard $(source_dir)/*.c)
objects := $(patsubst $(source_dir)/%.c,$(intermediate_build_dir)/%.o,$(sources))
headers := $(wildcard $(source_dir)/*.h)
hook_sources := $(wildcard $(hook_source_dir)/*.S)
hook_targets := $(patsubst $(hook_source_dir)/%.S,$(hook_build_dir)/%.text,$(hook_sources))
xplore_hook_sources := $(wildcard $(source_dir)/*.S)
xplore_hook_targets := $(patsubst $(source_dir)/%.S,$(hook_build_dir)/%.text,$(xplore_hook_sources))
target := build

all: $(target) $(hook_targets) $(xplore_hook_targets)

$(build_dirs):
	@echo "Creating build dir: $@"
	mkdir -p $@

$(hook_targets): $(hook_build_dir)/%.text: $(hook_source_dir)/%.S | $(hook_build_dir)
	@echo "Building hook $< -> $@"
	yasm -f elf64 -o $(patsubst $(hook_build_dir)/%.text,$(intermediate_build_dir)/%.o,$@) $<
	objcopy -O binary --only-section=.text $(patsubst $(hook_build_dir)/%.text,$(intermediate_build_dir)/%.o,$@) $@

$(xplore_hook_targets): $(hook_build_dir)/%.text: $(source_dir)/%.S | $(hook_build_dir)
	@echo "Building hook $< -> $@"
	yasm -f elf64 -o $(patsubst $(hook_build_dir)/%.text,$(intermediate_build_dir)/%.o,$@) $<
	objcopy -O binary --only-section=.text $(patsubst $(hook_build_dir)/%.text,$(intermediate_build_dir)/%.o,$@) $@

$(objects): $(intermediate_build_dir)/%.o: $(source_dir)/%.c $(headers) | $(intermediate_build_dir)
	gcc -c -o $@ $<

$(executable): $(objects) | $(bin_build_dir)
	gcc -o $(executable) $(objects)

$(target): $(executable)
