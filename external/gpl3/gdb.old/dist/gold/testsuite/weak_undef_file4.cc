// weak_undef_file4.cc -- test handling of weak undefined symbols for gold

// Copyright (C) 2014-2020 Free Software Foundation, Inc.
// Written by Cary Coutant <ccoutant@google.com>.

// This file is part of gold.

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
// MA 02110-1301, USA.

// This file tests that we correctly deal with weak undefined symbols
// when searching archive libraries.  If we have a weak undefined symbol,
// it should not cause us to link an archive library member that defines
// that symbol.  However, if the symbol is also listed in a -u option on
// the command line, it should cause the archive member to be linked.

int weak_undef_2 = 2;
