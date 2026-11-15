#!/usr/bin/env python3
"""Check C/C++ files against the repository's approximate KNF clang-format style.

No files are modified.  This is a formatting aid, not a full KNF validator.
"""

import argparse
import difflib
import pathlib
import shutil
import subprocess
import sys


def check_file(formatter, source):
    original = source.read_bytes()
    result = subprocess.run(
        [formatter, "--style=file", str(source)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if result.returncode:
        message = result.stderr.decode("utf-8", errors="replace").strip()
        raise RuntimeError(message or "clang-format failed")

    if original == result.stdout:
        return True

    old = original.decode("utf-8", errors="replace").splitlines(keepends=True)
    new = result.stdout.decode("utf-8", errors="replace").splitlines(keepends=True)
    sys.stdout.writelines(
        difflib.unified_diff(
            old,
            new,
            fromfile=str(source),
            tofile=str(source) + " (clang-format)",
        )
    )
    return False


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("files", nargs="+", type=pathlib.Path,
                        help="C/C++ source files to check (not modify)")
    parser.add_argument("--clang-format", default="clang-format",
                        help="path to clang-format executable")
    args = parser.parse_args(argv)

    formatter = shutil.which(args.clang_format)
    if formatter is None:
        parser.error("clang-format not found: " + args.clang_format)

    failed = False
    for source in args.files:
        if not source.is_file():
            print(f"{source}: not a regular file", file=sys.stderr)
            failed = True
            continue
        try:
            if not check_file(formatter, source.resolve()):
                failed = True
        except (OSError, RuntimeError) as error:
            print(f"{source}: {error}", file=sys.stderr)
            failed = True

    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
