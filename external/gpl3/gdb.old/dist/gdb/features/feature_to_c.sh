#!/bin/sh

# Convert text files to compilable C arrays.
#
# Copyright (C) 2007-2024 Free Software Foundation, Inc.
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

if test -z "$1" || test -z "$2"; then
  echo "Usage: $0 OUTPUTFILE INPUTFILE..."
  exit 1
fi

output=$1
shift

if test -e "$output"; then
  echo "Output file \"$output\" already exists; refusing to overwrite."
  exit 1
fi

echo '#include "xml-builtin.h"' >> "$output"

awk_script=$(echo "$0" | sed 's/\.sh$/.awk/')

for input; do
  arrayname=xml_feature_$(echo "$input" | sed 's,.*/,,; s/[-.]/_/g')

  ${AWK:-awk} \
    -v "arrayname=$arrayname" \
    -f "$awk_script" \
    < "$input" \
    >> "$output"
done

echo >> "$output"

echo "extern const char *const xml_builtin[][2] = {" >> "$output"

for input; do
  basename=$(echo "$input" | sed 's,.*/,,')
  arrayname=xml_feature_$(echo "$input" | sed 's,.*/,,; s/[-.]/_/g')
  echo "  { \"$basename\", $arrayname }," >> "$output"
done

echo "  { 0, 0 }" >> "$output"
echo "};" >> "$output"

# Local Variables:
# mode:shell-script
# sh-indentation:2
# End:
# vi:sw=2
