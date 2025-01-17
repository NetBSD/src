#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2021 Michael Jeanson <mjeanson@efficios.com>
#
# SPDX-License-Identifier: GPL-2.0-only

set -eu

INDIR=$1
OUTDIR=$2

ARGS=(
	"--annotate" # Add comments to the xml output
	"--no-corpus-path" # Do not put the path in the abi-corpus
)

# Older version of the reuse tool are a bit dumb, split the tags string to make
# it happy.
spdx="SPDX"
copyright="FileCopyrightText"
license="License-Identifier"

for lib in "${INDIR}"/liburcu*.so.?
do
	abidw "${ARGS[@]}" --out-file "${OUTDIR}/$(basename "$lib").xml" "$lib"

	# Clean the full paths
	sed -i "s#$(pwd)/##g" "${OUTDIR}/$(basename "$lib").xml"

	# Add SPDX headers
	sed -i "2 i <!--\n${spdx}-${copyright}: $(date +%Y) EfficiOS Inc.\n\n${spdx}-${license}: CC0-1.0\n-->" "${OUTDIR}/$(basename "$lib").xml"
done

