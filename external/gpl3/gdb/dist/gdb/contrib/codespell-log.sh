#!/usr/bin/env bash

# Copyright (C) 2025 Free Software Foundation, Inc.
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

# Script to be used as pre-commit commit-msg hook to spell-check the commit
# log using codespell.
#
# Using codespell directly as a pre-commit commit-msg hook has the drawback
# that:
# - if codespell fails, the commit fails
# - if the commit log mentions a typo correction, it'll require a
#   codespell:ignore annotation.
#
# This script works around these problems by treating codespell output as a
# hint, and ignoring codespell exit status.
#
# Implementation note: rather than using codespell directly, this script uses
# pre-commit to call codespell, because it allows us to control the codespell
# version that is used.

# Exit on error.
set -e

# Initialize temporary file names.
cfg=""
output=""

cleanup()
{
    for f in "$cfg" "$output"; do
	if [ "$f" != "" ]; then
	    rm -f "$f"
	fi
    done
}

# Schedule cleanup.
trap cleanup EXIT

# Create temporary files.
cfg=$(mktemp)
output=$(mktemp)

gen_cfg ()
{
    cat > "$1" <<EOF
repos:
- repo: https://github.com/codespell-project/codespell
  rev: v2.4.1
  hooks:
  - id: codespell
    name: codespell-log-internal
    stages: [manual]
    args: [--config, gdb/contrib/setup.cfg]
EOF
}

# Generate pre-commit configuration file.
gen_cfg "$cfg"

# Setup pre-commit command to run.
cmd=(pre-commit \
    run \
    -c "$cfg" \
    codespell \
    --hook-stage manual \
    --files "$@")

# Run pre-commit command.
if "${cmd[@]}" \
     > "$output" \
     2>&1; then
    # Command succeeded quietly, we're done.
    exit 0
fi

# Command failed quietly, now show the output.
#
# Simply doing "cat $output" doesn't produce colored output, so we just
# run the command again, that should be fast enough.
#
# Ignore codespell exit status.
"${cmd[@]}" || true
