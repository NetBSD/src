#!/bin/sh

# Copyright (C) 1989-2014 Free Software Foundation, Inc.

# This file is part of GDB.

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

# Create version.c from $1/../gdb/version.in.
# Please keep the same API as $1/../gdb/common/create-version.sh so
# they can be called the same way, from e.g. $srcroot/src-release.
# Usage:
#    create-version.sh PATH-TO-SIM-SRCDIR HOST_ALIAS \
#        TARGET_ALIAS OUTPUT-FILE-NAME

srcdir="$1"
host_alias="$2"
target_alias="$3"
output="$4"

rm -f version.c-tmp $output version.tmp
date=`sed -n -e 's/^.* BFD_VERSION_DATE \(.*\)$/\1/p' $srcdir/../bfd/version.h`
sed -e "s/DATE/$date/" < $srcdir/../gdb/version.in > version.tmp
echo '#include "version.h"' >> version.c-tmp
echo 'const char version[] = "'"`sed q version.tmp`"'";' >> version.c-tmp
mv version.c-tmp $output
rm -f version.tmp
