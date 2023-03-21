#
#   Copyright (C) 2020 Free Software Foundation, Inc.
#
# This file is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not see
# <http://www.gnu.org/licenses/>.
#

# Only process lines in the error-define block
/= ECTF_BASE/,/ECTF_NERR/ {
    # Do not process non-errors (braces, ECTF_NERR, etc).
    /^ *ECTF_/!n;
    # Strip out the base initializer.
    s, = ECTF_BASE,,;
    # Transform errors into _STR(...).
    s@^ *\(ECTF_[^[:blank:],]*\),\{0,1\}[[:blank:]]*/\* \(.*\).  \*/$@_CTF_STR (\1, "\2")@;
    p;
  }
