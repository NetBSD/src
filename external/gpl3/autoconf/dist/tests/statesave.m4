# statesave.m4 serial 2

# Copyright (C) 2000-2012 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# AC_STATE_SAVE(FILE)
# -------------------
# Save the shell variables and directory listing.  AT_CHECK_ENV uses these to
# confirm that no test modifies variables outside the Autoconf namespace or
# leaves temporary files.  AT_CONFIG_CMP uses the variable dumps to confirm
# that tests have the same side effects regardless of caching.
#
# The sed script duplicates uniq functionality (thanks to 'info sed
# uniq' for the recipe), in order to avoid a MacOS 10.5 bug where
# readdir can list a file multiple times in a rapidly changing
# directory, while avoiding yet another fork.
m4_defun([AC_STATE_SAVE],
[(set) 2>&1 | sort >state-env.$1
ls | sed '/^at-/d;/^state-/d;/^config\./d
  h
  :b
  $b
  N
  /^\(.*\)\n\1$/ {
    g
    bb
  }
  $b
  P
  D' >state-ls.$1
])# AC_STATE_SAVE
