#! /bin/sh
# Deleting members with long file names.

# Copyright (C) 2001 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

. ./preset
. $srcdir/before

set -e
prefix=This_is_a_very_long_file_name_prefix_that_is_designed_to_cause_problems_with_file_names_that_run_into_a_limit_of_the_posix_tar_formatXX
rm -f $prefix*
for i in 1 2 3 4 5 6 7 8 9
do touch $prefix$i
done
tar -cf archive ./$prefix*
tar --delete -f archive ./${prefix}5
tar -tf archive

out="\
./${prefix}1
./${prefix}2
./${prefix}3
./${prefix}4
./${prefix}6
./${prefix}7
./${prefix}8
./${prefix}9
"

. $srcdir/after
