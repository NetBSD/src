#! /usr/bin/env python3

# Copyright (C) 2011-2025 Free Software Foundation, Inc.
#
# This file is part of GDB.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# This script updates the list of years in the copyright notices in
# most files maintained by the GDB project.
#
# Usage: ./gdb/copyright.py
#
# Always review the output of this script before committing it!
#
# A useful command to review the output is:
#
#     $ filterdiff -x \*.c -x \*.cc -x \*.h -x \*.exp updates.diff
#
# This removes the bulk of the changes which are most likely to be correct.

# pyright: strict

import argparse
import locale
import os
import os.path
import pathlib
import subprocess
import sys
from typing import Iterable


def get_update_list():
    """Return the list of files to update.

    Assumes that the current working directory when called is the root
    of the GDB source tree (NOT the gdb/ subdirectory!).  The names of
    the files are relative to that root directory.
    """
    result = (
        subprocess.check_output(
            [
                "git",
                "ls-files",
                "-z",
                "--",
                "gdb",
                "gdbserver",
                "gdbsupport",
                "gnulib",
                "sim",
                "include/gdb",
            ],
            text=True,
        )
        .rstrip("\0")
        .split("\0")
    )

    full_exclude_list = EXCLUDE_LIST + BY_HAND

    def include_file(filename: str):
        path = pathlib.Path(filename)
        for pattern in full_exclude_list:
            if path.full_match(pattern):
                return False

        return True

    return filter(include_file, result)


def update_files(update_list: Iterable[str]):
    """Update the copyright header of the files in the given list.

    We use gnulib's update-copyright script for that.
    """
    # We want to use year intervals in the copyright notices, and
    # all years should be collapsed to one single year interval,
    # even if there are "holes" in the list of years found in the
    # original copyright notice (OK'ed by the FSF, case [gnu.org #719834]).
    os.environ["UPDATE_COPYRIGHT_USE_INTERVALS"] = "2"

    # Perform the update, and save the output in a string.
    update_cmd = ["bash", "gnulib/import/extra/update-copyright"]
    update_cmd += update_list

    p = subprocess.Popen(
        update_cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        encoding=locale.getpreferredencoding(),
    )
    update_out = p.communicate()[0]

    # Process the output.  Typically, a lot of files do not have
    # a copyright notice :-(.  The update-copyright script prints
    # a well defined warning when it did not find the copyright notice.
    # For each of those, do a sanity check and see if they may in fact
    # have one.  For the files that are found not to have one, we filter
    # the line out from the output, since there is nothing more to do,
    # short of looking at each file and seeing which notice is appropriate.
    # Too much work! (~4,000 files listed as of 2012-01-03).
    update_out = update_out.splitlines(keepends=False)
    warning_string = ": warning: copyright statement not found"
    warning_len = len(warning_string)

    for line in update_out:
        if line.endswith(warning_string):
            filename = line[:-warning_len]
            if may_have_copyright_notice(filename):
                print(line)
        else:
            # Unrecognized file format. !?!
            print("*** " + line)


def may_have_copyright_notice(filename: str):
    """Check that the given file does not seem to have a copyright notice.

    The filename is relative to the root directory.
    This function assumes that the current working directory is that root
    directory.

    The algorithm is fairly crude, meaning that it might return
    some false positives.  I do not think it will return any false
    negatives...  We might improve this function to handle more
    complex cases later...
    """
    # For now, it may have a copyright notice if we find the word
    # "Copyright" at the (reasonable) start of the given file, say
    # 50 lines...
    MAX_LINES = 50

    # We don't really know what encoding each file might be following,
    # so just open the file as a byte stream. We only need to search
    # for a pattern that should be the same regardless of encoding,
    # so that should be good enough.
    with open(filename, "rb") as fd:
        for lineno, line in enumerate(fd, start=1):
            if b"Copyright" in line:
                return True
            if lineno > MAX_LINES:
                break
    return False


def get_parser() -> argparse.ArgumentParser:
    """Get a command line parser."""
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    return parser


def main(argv: list[str]) -> int | None:
    """The main subprogram."""
    parser = get_parser()
    _ = parser.parse_args(argv)

    if not os.path.isfile("gnulib/import/extra/update-copyright"):
        sys.exit("Error: This script must be called from the top-level directory.")

    update_list = get_update_list()
    update_files(update_list)

    # Remind the user that some files need to be updated by HAND...

    if MULTIPLE_COPYRIGHT_HEADERS:
        print()
        print(
            "\033[31m"
            "REMINDER: Multiple copyright headers must be updated by hand:"
            "\033[0m"
        )
        for filename in MULTIPLE_COPYRIGHT_HEADERS:
            print("  ", filename)

    if BY_HAND:
        print()
        print(
            "\033[31mREMINDER: The following files must be updated by hand:" "\033[0m"
        )
        for filename in BY_HAND:
            print("  ", filename)

    print()
    print(
        "\033[31mREMINDER: The following files contain code to print a copyright "
        "notice at runtime, they must be updated by hand:\033[0m"
    )
    print("  gdb/top.c")
    print("  gdbserver/gdbreplay.cc")
    print("  gdbserver/server.cc")


############################################################################
#
# Some constants, placed at the end because they take up a lot of room.
# The actual value of these constants is not significant to the understanding
# of the script.
#
############################################################################

# Files which should not be modified, either because they are
# generated, non-FSF, or otherwise special (e.g. license text,
# or test cases which must be sensitive to line numbering).
#
# Entries are treated as glob patterns.
EXCLUDE_LIST = (
    "**/aclocal.m4",
    "**/configure",
    "**/COPYING.LIB",
    "**/COPYING",
    "**/fdl.texi",
    "**/gpl.texi",
    "gdb/copying.c",
    "gdb/nat/glibc_thread_db.h",
    "gdb/CONTRIBUTE",
    "gdbsupport/Makefile.in",
    "gdbsupport/unordered_dense.h",
    "gnulib/doc/gendocs_template",
    "gnulib/doc/gendocs_template_min",
    "gnulib/import/**",
    "gnulib/config.in",
    "gnulib/Makefile.in",
    "sim/Makefile.in",
    # The files below have a copyright, but not held by the FSF.
    "gdb/exc_request.defs",
    "gdb/gdbtk",
    "gdb/testsuite/gdb.gdbtk/",
    "sim/arm/armemu.h",
    "sim/arm/armos.c",
    "sim/arm/gdbhost.c",
    "sim/arm/dbg_hif.h",
    "sim/arm/dbg_conf.h",
    "sim/arm/communicate.h",
    "sim/arm/armos.h",
    "sim/arm/armcopro.c",
    "sim/arm/armemu.c",
    "sim/arm/kid.c",
    "sim/arm/thumbemu.c",
    "sim/arm/armdefs.h",
    "sim/arm/armopts.h",
    "sim/arm/dbg_cp.h",
    "sim/arm/dbg_rdi.h",
    "sim/arm/parent.c",
    "sim/arm/armsupp.c",
    "sim/arm/armrdi.c",
    "sim/arm/bag.c",
    "sim/arm/armvirt.c",
    "sim/arm/main.c",
    "sim/arm/bag.h",
    "sim/arm/communicate.c",
    "sim/arm/gdbhost.h",
    "sim/arm/armfpe.h",
    "sim/arm/arminit.c",
    "sim/common/cgen-fpu.c",
    "sim/common/cgen-fpu.h",
    "sim/common/cgen-accfp.c",
    "sim/mips/m16run.c",
    "sim/mips/sim-main.c",
    "sim/moxie/moxie-gdb.dts",
    # Not a single file in sim/ppc/ appears to be copyright FSF :-(.
    "sim/ppc/**",
    "sim/testsuite/mips/mips32-dsp2.s",
)

# The list of files to update by hand.
#
# Entries are treated as glob patterns.
BY_HAND: tuple[str, ...] = (
    # Nothing at the moment :-).
)

# Files containing multiple copyright headers.  This script is only
# fixing the first one it finds, so we need to finish the update
# by hand.
#
# Entries are treated as glob patterns.
MULTIPLE_COPYRIGHT_HEADERS = (
    "gdb/doc/gdb.texinfo",
    "gdb/doc/refcard.tex",
    "gdb/syscalls/update-netbsd.sh",
)

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
