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

BEGIN { n = 0
    printf "static const char %s[] = {\n", arrayname
    for (i = 0; i < 255; i++)
	_ord_[sprintf("%c", i)] = i
}

{
    split($0, line, "");
    printf "  "
    for (i = 1; i <= length($0); i++) {
	c = line[i]
	if (c == "'") {
	    printf "'\\''"
	} else if (c == "\\") {
	    printf "'\\\\'"
	} else if (_ord_[c] >= 32 && _ord_[c] < 127) {
	    printf "'%s'", c
	} else {
	    printf "'\\%03o'", _ord_[c]
	}
	printf ", "
	if (i % 10 == 0)
	    printf "\n   "
    }
    printf "'\\n', \n"
}

END {
    print "  0 };"
}
