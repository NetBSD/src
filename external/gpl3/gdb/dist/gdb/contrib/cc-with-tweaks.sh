#! /bin/sh
# Wrapper around gcc to tweak the output in various ways when running
# the testsuite.

# Copyright (C) 2010-2014 Free Software Foundation, Inc.
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

# This program requires gdb and objcopy in addition to gcc.
# The default values are gdb from the build tree and objcopy from $PATH.
# They may be overridden by setting environment variables GDB and OBJCOPY
# respectively.  Note that GDB should contain the gdb binary as well as the
# -data-directory flag, e.g., "foo/gdb -data-directory foo/data-directory".
# We assume the current directory is either $obj/gdb or $obj/gdb/testsuite.
#
# Example usage:
#
# bash$ cd $objdir/gdb/testsuite
# bash$ runtest \
#   CC_FOR_TARGET="/bin/sh $srcdir/gdb/contrib/cc-with-tweaks.sh ARGS gcc" \
#   CXX_FOR_TARGET="/bin/sh $srcdir/gdb/contrib/cc-with-tweaks.sh ARGS g++"
#
# For documentation on Fission and dwp files:
#     http://gcc.gnu.org/wiki/DebugFission
#     http://gcc.gnu.org/wiki/DebugFissionDWP
# For documentation on index files: info -f gdb.info -n "Index Files"
# For information about 'dwz', see the announcement:
#     http://gcc.gnu.org/ml/gcc/2012-04/msg00686.html
# (More documentation is to come.)

# ARGS determine what is done.  They can be:
# -Z invoke objcopy --compress-debug-sections
# -z compress using dwz
# -m compress using dwz -m
# -i make an index
# -p create .dwp files (Fission), you need to also use gcc option -gsplit-dwarf
# If nothing is given, no changes are made

myname=cc-with-tweaks.sh

if [ -z "$GDB" ]
then
    if [ -f ./gdb ]
    then
	GDB="./gdb -data-directory data-directory"
    elif [ -f ../gdb ]
    then
	GDB="../gdb -data-directory ../data-directory"
    elif [ -f ../../gdb ]
    then
	GDB="../../gdb -data-directory ../../data-directory"
    else
	echo "$myname: unable to find usable gdb" >&2
	exit 1
    fi
fi

OBJCOPY=${OBJCOPY:-objcopy}
READELF=${READELF:-readelf}

DWZ=${DWZ:-dwz}
DWP=${DWP:-dwp}

have_link=unknown
next_is_output_file=no
output_file=a.out

want_index=false
want_dwz=false
want_multi=false
want_dwp=false
want_objcopy_compress=false

while [ $# -gt 0 ]; do
    case "$1" in
	-Z) want_objcopy_compress=true ;;
	-z) want_dwz=true ;;
	-i) want_index=true ;;
	-m) want_multi=true ;;
	-p) want_dwp=true ;;
	*) break ;;
    esac
    shift
done

for arg in "$@"
do
    if [ "$next_is_output_file" = "yes" ]
    then
	output_file="$arg"
	next_is_output_file=no
	continue
    fi

    # Poor man's gcc argument parser.
    # We don't need to handle all arguments, we just need to know if we're
    # doing a link and what the output file is.
    # It's not perfect, but it seems to work well enough for the task at hand.
    case "$arg" in
    "-c") have_link=no ;;
    "-E") have_link=no ;;
    "-S") have_link=no ;;
    "-o") next_is_output_file=yes ;;
    esac
done

if [ "$next_is_output_file" = "yes" ]
then
    echo "$myname: Unable to find output file" >&2
    exit 1
fi

if [ "$have_link" = "no" ]
then
    "$@"
    exit $?
fi

index_file="${output_file}.gdb-index"
if [ "$want_index" = true ] && [ -f "$index_file" ]
then
    echo "$myname: Index file $index_file exists, won't clobber." >&2
    exit 1
fi

output_dir="${output_file%/*}"
[ "$output_dir" = "$output_file" ] && output_dir="."

"$@"
rc=$?
[ $rc != 0 ] && exit $rc
if [ ! -f "$output_file" ]
then
    echo "$myname: Internal error: $output_file missing." >&2
    exit 1
fi

if [ "$want_objcopy_compress" = true ]; then
    $OBJCOPY --compress-debug-sections "$output_file"
    rc=$?
    [ $rc != 0 ] && exit $rc
fi

if [ "$want_index" = true ]; then
    $GDB --batch-silent -nx -ex "set auto-load no" -ex "file $output_file" -ex "save gdb-index $output_dir"
    rc=$?
    [ $rc != 0 ] && exit $rc

    # GDB might not always create an index.  Cope.
    if [ -f "$index_file" ]
    then
	$OBJCOPY --add-section .gdb_index="$index_file" \
	    --set-section-flags .gdb_index=readonly \
	    "$output_file" "$output_file"
	rc=$?
    else
	rc=0
    fi
    [ $rc != 0 ] && exit $rc
fi

if [ "$want_dwz" = true ]; then
    $DWZ "$output_file" > /dev/null 2>&1
elif [ "$want_multi" = true ]; then
    cp $output_file ${output_file}.alt
    $DWZ -m ${output_file}.dwz "$output_file" ${output_file}.alt > /dev/null 2>&1
fi

if [ "$want_dwp" = true ]; then
    dwo_files=$($READELF -wi "${output_file}" | grep _dwo_name | \
	sed -e 's/^.*: //' | sort | uniq)
    rc=0
    if [ -n "$dwo_files" ]; then
	$DWP -o "${output_file}.dwp" ${dwo_files} > /dev/null
	rc=$?
	[ $rc != 0 ] && exit $rc
	rm -f ${dwo_files}
    fi
fi

rm -f "$index_file"
exit $rc
