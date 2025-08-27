#!/bin/sh

# Copyright (C) 2022-2024 Free Software Foundation, Inc.
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

# Used to generate .xml.in files, like so:
#
# ./update-linux.sh amd64-linux.xml.in
# ./update-linux.sh i386-linux.xml.in -m32

if [ $# -lt 1 ]; then
    echo "file argument needed"
    exit 1
fi

f="$1"
shift

if [ ! -f "$f" ]; then
    echo "cannot find $f"
    exit 1
fi

startyear=2009
case "$f" in
    *aarch64-linux.xml.in)
	startyear=2015
	;;
esac

year=$(date +%Y)

(
    cat <<EOF
<?xml version="1.0"?>
<!-- Copyright (C) $startyear-$year Free Software Foundation, Inc.

     Copying and distribution of this file, with or without modification,
     are permitted in any medium without royalty provided the copyright
     notice and this notice are preserved.  -->

<!DOCTYPE feature SYSTEM "gdb-syscalls.dtd">

<!-- This file was generated using the following file:

     <sys/syscall.h>

     The file mentioned above belongs to the Linux Kernel.  -->


EOF

    echo '<syscalls_info>'

# There are __NR_ and __NR3264_ prefixed syscall numbers, handle them
# automatically in this script. Here are the examples of the two types:
#
# #define __NR_io_setup 0
# #define __NR3264_fcntl 25

    echo '#include <asm/unistd.h>' \
	| gcc -E - -dD "$@" \
	| grep -E '#define (__NR_|__NR3264_)' \
	| while read -r line; do
	line=$(echo "$line" | awk '$2 ~ "__NR" && $3 !~ "__NR3264_" {
	     sub("^#define __NR(3264)?_", ""); print | "sort -k2 -n"}')
	if [ -z "$line" ]; then
		continue
	fi
	name=$(echo "$line" | awk '{print $1}')
	nr=$(echo "$line" | awk '{print $2}')
	echo "  <syscall name=\"$name\" number=\"$nr\"/>"
    done

    echo '</syscalls_info>'
) > "$f"
