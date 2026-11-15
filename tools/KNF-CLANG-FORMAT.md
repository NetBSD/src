# Optional KNF-oriented clang-format checks

NetBSD's authoritative C style guidance is in `share/misc/style`.
The repository-root `.clang-format` is an **approximation** for local editing;
clang-format is not a complete KNF implementation. Do not mass-format existing
NetBSD source files with it.

To build `clang-format` from LLVM source:

```sh
git clone https://github.com/llvm/llvm-project.git
cmake -S llvm-project/llvm -B llvm-build -G Ninja \
  -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=Release
cmake --build llvm-build --target clang-format
```

From the root of this NetBSD source tree, run:

```sh
python3 tools/knf-format-check.py \
  --clang-format /path/to/llvm-build/bin/clang-format \
  path/to/changed/file.c
```

The command prints a unified diff and exits with status 1 for files that differ.
It leaves the original files untouched. Exit status 0 means the file agrees
with **this formatter configuration**, not that it is fully KNF compliant.

Run the self-tests, which use a fake formatter and need no LLVM installation:

```sh
python3 -m unittest discover -s tools -p 'test_knf_format_check.py'
```

Potential improvements: expand KNF test cases for include grouping, bit-field
layout, and comments; verify mismatches against a current upstream clang-format
before submitting LLVM formatter bug reports.
