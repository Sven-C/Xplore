build_dir := build
intermediate_build_dir := $(build_dir)/intermediates
intermediate_assembly_build_dir := $(intermediate_build_dir)/assembly_objects
intermediate_shared_object_build_dir := $(intermediate_build_dir)/library_objects
intermediate_source_object_build_dir := $(intermediate_build_dir)/src_objects
bin_build_dir := $(build_dir)/bin
hook_build_dir := $(build_dir)/hooks
hook_assembly_build_dir := $(hook_build_dir)/assembly
hook_shared_object_build_dir := $(hook_build_dir)/so
build_dirs := $(bin_build_dir) $(hook_build_dir) $(hook_assembly_build_dir) $(hook_shared_object_build_dir) $(intermediate_assembly_build_dir) $(intermediate_shared_object_build_dir) $(intermediate_source_object_build_dir)
executable := $(bin_build_dir)/xplore
source_dir := src
hook_source_dir := $(source_dir)/hooks
hook_assembly_source_dir := $(hook_source_dir)/assembly
hook_shared_object_source_dir := $(hook_source_dir)/c
sources := $(wildcard $(source_dir)/*.c)
objects := $(patsubst $(source_dir)/%.c,$(intermediate_source_object_build_dir)/%.o,$(sources))
headers := $(wildcard $(source_dir)/*.h)
hook_assembly_sources := $(wildcard $(hook_assembly_source_dir)/*.S)
hook_assembly_targets := $(patsubst $(hook_assembly_source_dir)/%.S,$(hook_assembly_build_dir)/%.text,$(hook_assembly_sources))
hook_shared_object_sources := $(wildcard $(hook_shared_object_source_dir)/*.c)
hook_shared_object_objects := $(patsubst $(hook_shared_object_source_dir)/%.c,$(intermediate_shared_object_build_dir)/%.o,$(hook_shared_object_sources))
hook_shared_object_targets := $(hook_shared_object_build_dir)/libxploration.so
xplore_hook_sources := $(wildcard $(source_dir)/*.S)
xplore_hook_targets := $(patsubst $(source_dir)/%.S,$(hook_assembly_build_dir)/%.text,$(xplore_hook_sources))
target := build

all: $(target) $(hook_assembly_targets) $(hook_shared_object_targets) $(xplore_hook_targets)

$(build_dirs):
	@echo "Creating build dir: $@"
	mkdir -p $@

$(hook_assembly_targets): $(hook_assembly_build_dir)/%.text: $(hook_assembly_source_dir)/%.S | $(hook_assembly_build_dir) $(intermediate_assembly_build_dir)
	@echo "Building hook $< -> $@"
	yasm -f elf64 -o $(patsubst $(hook_assembly_build_dir)/%.text,$(intermediate_assembly_build_dir)/%.o,$@) $<
	objcopy -O binary --only-section=.text $(patsubst $(hook_assembly_build_dir)/%.text,$(intermediate_assembly_build_dir)/%.o,$@) $@

$(hook_shared_object_objects): $(intermediate_shared_object_build_dir)/%.o: $(hook_shared_object_source_dir)/%.c | $(intermediate_shared_object_build_dir)
	gcc -c -fPIC -o $@ $<

$(hook_shared_object_targets): $(hook_shared_object_objects) | $(hook_shared_object_build_dir)
	@echo "Building hook $(hook_shared_object_objects) -> $@"
	gcc -shared -o $@ $(hook_shared_object_objects)

$(xplore_hook_targets): $(hook_assembly_build_dir)/%.text: $(source_dir)/%.S | $(hook_assembly_build_dir) $(intermediate_assembly_build_dir)
	@echo "Building hook $< -> $@"
	yasm -f elf64 -o $(patsubst $(hook_assembly_build_dir)/%.text,$(intermediate_assembly_build_dir)/%.o,$@) $<
	objcopy -O binary --only-section=.text $(patsubst $(hook_assembly_build_dir)/%.text,$(intermediate_assembly_build_dir)/%.o,$@) $@

$(objects): $(intermediate_source_object_build_dir)/%.o: $(source_dir)/%.c $(headers) | $(intermediate_source_object_build_dir)
	gcc -c -o $@ $<

$(executable): $(objects) | $(bin_build_dir)
	gcc -o $(executable) $(objects)

$(target): $(executable)
