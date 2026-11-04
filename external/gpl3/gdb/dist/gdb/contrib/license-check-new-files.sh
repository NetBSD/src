#!/usr/bin/env python3

# Copyright (C) 2025 Free Software Foundation, Inc.
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

# This program requires the python modules GitPython (git) and scancode-toolkit.
# It builds a list of all the newly added files to the repository and scans
# each file for a license, printing it to the terminal.  If "--skip" is used,
# it will only output non-"common" licenses, e.g., omitting "GPL-3.0-or-later".
# This makes it a little bit easier to detect any possible new licenses.
#
# Example:
# bash$ cd /path/to/binutils-gdb/gdb
# bash$ ./contrib/license-check-new-files.sh -s gdb-15-branchpoint gdb-16-branchpoint
# Scanning directories gdb*/...
# gdb/contrib/common-misspellings.txt: no longer in repo?
# gdb/contrib/spellcheck.sh: no longer in repo?
# gdbsupport/unordered_dense.h: MIT

import os
import sys
import argparse
from pathlib import PurePath
from git import Repo
from scancode import api

# A list of "common" licenses. If "--skip" is used, any file
# with a license in this list will be omitted from the output.
COMMON_LICENSES = ["GPL-2.0-or-later", "GPL-3.0-or-later"]

# Default list of directories to scan. Default scans are limited to
# gdb-specific git directories because much of the rest of binutils-gdb
# is actually owned by other projects/packages.
DEFAULT_SCAN_DIRS = "gdb*"


# Get the commit object associated with the string commit CSTR
# from the git repository REPO.
#
# Returns the object or prints an error and exits.
def get_commit(repo, cstr):
    try:
        return repo.commit(cstr)
    except:
        print(f'unknown commit "{cstr}"')
        sys.exit(2)


# Uses scancode-toolkit package to scan FILE's licenses.
# Returns the full license dict from scancode on success or
# propagates any exceptions.
def get_licenses_for_file(file):
    return api.get_licenses(file)


# Helper function to print FILE to the terminal if skipping
# common licenses.
def skip_print_file(skip, file):
    if skip:
        print(f"{file}: ", end="")


def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument("from_commit")
    parser.add_argument("to_commit")
    parser.add_argument(
        "-s", "--skip", help="skip common licenses in output", action="store_true"
    )
    parser.add_argument(
        "-p",
        "--paths",
        help=f'paths to scan (default is "{DEFAULT_SCAN_DIRS}")',
        type=str,
        default=DEFAULT_SCAN_DIRS,
    )
    args = parser.parse_args()

    # Commit boundaries to search for new files
    from_commit = args.from_commit
    to_commit = args.to_commit

    # Get the list of new files from git. Try the current directory,
    # looping up to the root attempting to find a valid git repository.
    path = PurePath(os.getcwd())
    paths = list(path.parents)
    paths.insert(0, path)
    for dir in paths:
        try:
            repo = Repo(dir)
            break
        except:
            pass

    if dir == path.parents[-1]:
        print(f'not a git repository (or any parent up to mount point "{dir}")')
        sys.exit(2)

    # Get from/to commits
    fc = get_commit(repo, from_commit)
    tc = get_commit(repo, to_commit)

    # Loop over new files
    paths = [str(dir) for dir in args.paths.split(",")]
    print(f'Scanning directories {",".join(f"{s}/" for s in paths)}...')
    for file in fc.diff(tc, paths=paths).iter_change_type("A"):
        filename = file.a_path
        if not args.skip:
            print(f"checking licenses for {filename}... ", end="", flush=True)
        try:
            f = dir.joinpath(dir, filename).as_posix()
            lic = get_licenses_for_file(f)
            if len(lic["license_clues"]) > 1:
                print("multiple licenses detected")
            elif (
                not args.skip
                or lic["detected_license_expression_spdx"] not in COMMON_LICENSES
            ):
                skip_print_file(args.skip, filename)
                print(f"{lic['detected_license_expression_spdx']}")
        except OSError:
            # Likely hit a file that was added to the repo and subsequently removed.
            skip_print_file(args.skip, filename)
            print("no longer in repo?")
        except KeyboardInterrupt:
            print("interrupted")
            break
        except Exception as e:
            # If scanning fails, there is little we can do but print an error.
            skip_print_file(args.skip, filename)
            print(e)


if __name__ == "__main__":
    main(sys.argv)
