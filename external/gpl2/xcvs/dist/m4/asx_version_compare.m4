# Compare two strings possibly containing shell variables as version strings.

# Copyright (c) 2003
#               Derek R. Price, Ximbiot <http://ximbiot.com>,
#               and the Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# ASX_VERSION_COMPARE(VERSION-1, VERSION-2,
#                     [ACTION-IF-LESS], [ACTION-IF-EQUAL], [ACTION-IF-GREATER])
# -----------------------------------------------------------------------------
# Compare two strings possibly containing shell variables as version strings.
AC_DEFUN([ASX_VERSION_COMPARE],
[if test "x[$1]" = "x[$2]"; then
  # the strings are equal.  run ACTION-IF-EQUAL and bail
  m4_default([$4], :)
m4_ifvaln([$3$5], [else
  # first unletter the versions
  # this only works for a single trailing letter
  dnl echo has a double-quoted arg to allow for shell expansion.
  asx_version_1=`echo "[$1]" |
                 sed 's/\([abcedfghi]\)/.\1/;
                      s/\([jklmnopqrs]\)/.1\1/;
                      s/\([tuvwxyz]\)/.2\1/;
                      y/abcdefghijklmnopqrstuvwxyz/12345678901234567890123456/;'`
  asx_version_2=`echo "[$2]" |
                 sed 's/\([abcedfghi]\)/.\1/;
                      s/\([jklmnopqrs]\)/.1\1/;
                      s/\([tuvwxyz]\)/.2\1/;
                      y/abcdefghijklmnopqrstuvwxyz/12345678901234567890123456/;'`
  asx_count=1
  asx_save_IFS=$IFS
  IFS=.
  asx_retval=-1
  for vsub1 in $asx_version_1; do
    vsub2=`echo "$asx_version_2" |awk -F. "{print \\\$$asx_count}"`
    if test -z "$vsub2" || test $vsub1 -gt $vsub2; then
      asx_retval=1
      break
    elif test $vsub1 -lt $vsub2; then
      break
    fi
    asx_count=`expr $asx_count + 1`
  done
  IFS=$asx_save_IFS
  if test $asx_retval -eq -1; then
    m4_default([$3], :)
  m4_ifval([$5], [else
$5])
  fi])
fi
]) # ASX_VERSION_COMPARE
