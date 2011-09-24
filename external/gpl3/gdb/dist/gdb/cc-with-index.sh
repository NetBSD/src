#! /bin/sh
# Wrapper around gcc to add the .gdb_index section when running the testsuite.

# Copyright (C) 2010, 2011 Free Software Foundation, Inc.
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
# The default values are gdb from the build tree and objdump from $PATH.
# They may be overridden by setting environment variables GDB and OBJDUMP
# respectively.
# We assume the current directory is either $obj/gdb or $obj/gdb/testsuite.
#
# Example usage:
#
# bash$ cd $objdir/gdb/testsuite
# bash$ runtest \
#   CC_FOR_TARGET="/bin/sh $srcdir/cc-with-index.sh gcc" \
#   CXX_FOR_TARGET="/bin/sh $srcdir/cc-with-index.sh g++"
#
# For documentation on index files: info -f gdb.info -n "Index Files"

myname=cc-with-index.sh

if [ -z "$GDB" ]
then
    if [ -f ./gdb ]
    then
	GDB="./gdb"
    elif [ -f ../gdb ]
    then
	GDB="../gdb"
    else
	echo "$myname: unable to find usable gdb" >&2
	exit 1
    fi
fi

OBJCOPY=${OBJCOPY:-objcopy}

have_link=unknown
next_is_output_file=no
output_file=a.out

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
if [ -f "$index_file" ]
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

$GDB --batch-silent -nx -ex "file $output_file" -ex "save gdb-index $output_dir"
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

rm -f "$index_file"
exit $rc
