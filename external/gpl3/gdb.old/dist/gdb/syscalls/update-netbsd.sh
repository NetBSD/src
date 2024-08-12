#! /bin/sh

# Copyright (C) 2020-2023 Free Software Foundation, Inc.
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

# Usage: update-netbsd.sh <path-to-syscall.h>
# Update the netbsd.xml file.
#
# NetBSD uses the same list of system calls on all architectures.
# The list is defined in the sys/kern/syscalls.master file in the
# NetBSD source tree.  This file is used as an input to generate
# several files that are also stored in NetBSD's source tree.  This
# script parses one of those generated files (sys/sys/syscall.h)
# rather than syscalls.master as syscall.h is easier to parse.

if [ $# -ne 1 ]; then
   echo "Error: Path to syscall.h missing. Aborting."
   echo "Usage: update-netbsd.sh <path-to-syscall.h>"
   exit 1
fi

cat > netbsd.xml.tmp <<EOF
<?xml version="1.0"?> <!-- THIS FILE IS GENERATED -*- buffer-read-only: t -*-  -->
<!-- vi:set ro: -->
<!-- Copyright (C) 2020-2023 Free Software Foundation, Inc.

     Copying and distribution of this file, with or without modification,
     are permitted in any medium without royalty provided the copyright
     notice and this notice are preserved.  -->

<!DOCTYPE feature SYSTEM "gdb-syscalls.dtd">

<!-- This file was generated using the following file:

     /usr/src/sys/sys/syscall.h

     The file mentioned above belongs to the NetBSD Kernel.  -->

<syscalls_info>
EOF

awk '
/MAXSYSCALL/ || /_SYS_SYSCALL_H_/ || /MAXSYSARGS/ || /syscall/ || /NSYSENT/ {
    next
}
/^#define/ {
    sub(/^SYS_/,"",$2);
    printf "  <syscall name=\"%s\" number=\"%s\"", $2, $3
    if (sub(/^netbsd[0-9]*_/,"",$2) != 0)
        printf " alias=\"%s\"", $2
    printf "/>\n"
}
/\/\* [0-9]* is obsolete [a-z_]* \*\// {
    printf "  <syscall name=\"%s\" number=\"%s\"/>\n", $5, $2
}
/\/\* [0-9]* is netbsd[0-9]* [a-z_]* \*\// {
    printf "  <syscall name=\"%s_%s\" number=\"%s\" alias=\"%s\"/>\n", $4, $5, $2, $5
}' "$1" >> netbsd.xml.tmp

cat >> netbsd.xml.tmp <<EOF
</syscalls_info>
EOF

../../move-if-change netbsd.xml.tmp netbsd.xml
