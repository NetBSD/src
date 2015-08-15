#! /bin/sh

# Add a .gdb_index section to a file.

# Copyright (C) 2010-2015 Free Software Foundation, Inc.
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

# This program assumes gdb and objcopy are in $PATH.
# If not, or you want others, pass the following in the environment
GDB=${GDB:=gdb}
OBJCOPY=${OBJCOPY:=objcopy}

myname="${0##*/}"

if test $# != 1; then
    echo "usage: $myname FILE" 1>&2
    exit 1
fi

file="$1"

if test ! -r "$file"; then
    echo "$myname: unable to access: $file" 1>&2
    exit 1
fi

dir="${file%/*}"
test "$dir" = "$file" && dir="."
index="${file}.gdb-index"

rm -f $index
# Ensure intermediate index file is removed when we exit.
trap "rm -f $index" 0

$GDB --batch -nx -iex 'set auto-load no' \
    -ex "file $file" -ex "save gdb-index $dir" || {
    # Just in case.
    status=$?
    echo "$myname: gdb error generating index for $file" 1>&2
    exit $status
}

# In some situations gdb can exit without creating an index.  This is
# not an error.
# E.g., if $file is stripped.  This behaviour is akin to stripping an
# already stripped binary, it's a no-op.
status=0

if test -f "$index"; then
    $OBJCOPY --add-section .gdb_index="$index" \
	--set-section-flags .gdb_index=readonly "$file" "$file"
    status=$?
else
    echo "$myname: No index was created for $file" 1>&2
    echo "$myname: [Was there no debuginfo? Was there already an index?]" 1>&2
fi

exit $status
