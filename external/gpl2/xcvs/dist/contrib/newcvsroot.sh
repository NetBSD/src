#! /bin/sh
#
# Copyright 2002 Derek R. Price & Ximbiot <http://ximbiot.com>.
# All rights reserved.
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
#
# ///// ///// ///// ///// ///// ***** \\\\\ \\\\\ \\\\\ \\\\\ \\\\\
#
# newcvsroot.sh
#
# Recursively change the CVSROOT for a sandbox.
#
# INPUTS
#	$1		The new CVSROOT
#	$2+		The list of sandbox directories to convert.
#                       Defaults to the current directory.

usage ()
{
	echo "$0: usage: $prog newcvsroot [startdir]" >&2
}

prog=`basename "$0"`

if test "${1+set}" != set; then
	usage
	exit 2
else :; fi

echo "$1" >/tmp/$prog$$
shift

for dir in `find "${@:-.}" -name CVS`; do
	cp /tmp/$prog$$ "$dir"/Root
done

rm /tmp/$prog$$
