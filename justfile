executable-name := "xplore"
build-dir := "build"
build-hooks-dir := build-dir / "hooks"
src-dir := "src"
executable-path := build-dir / "bin" / executable-name
fread-hook := "fread_hook.text"
fopen-hook := "fopen_hook.text"

pwn TARGET: build
    ./{{executable-path}} -h {{build-hooks-dir}}/{{fopen-hook}} -h {{build-hooks-dir}}/{{fread-hook}} {{TARGET}}

build:
    make all
