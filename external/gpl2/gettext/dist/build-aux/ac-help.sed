# Sed script for post-processing the trace output of subordinate configures.
# Copyright (C) 2003, 2005 Free Software Foundation, Inc.
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
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

# Replace invocations of some libtool macros.
s|.AC_ENABLE_SHARED_DEFAULT.|yes|g
s|.AC_ENABLE_STATIC_DEFAULT.|yes|g
s|.AC_ENABLE_FAST_INSTALL_DEFAULT.|yes|g
# Avoid loss of brackets, such as  --with-tags[=TAGS] => --with-tags=TAGS
s|\[=TAGS\]|@<:@=TAGS@:>@|g
s|\[=PKGS\]|@<:@=PKGS@:>@|g
# Avoid unwanted comma, such as  [default=use both] => [default=use], [both]
s|\[default=use both\]|@<:@default=use both@:>@|g
