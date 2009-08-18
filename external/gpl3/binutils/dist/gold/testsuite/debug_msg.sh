#!/bin/sh

# debug_msg.sh -- a test case for printing debug info for missing symbols.

# Copyright 2006, 2007, 2008 Free Software Foundation, Inc.
# Written by Ian Lance Taylor <iant@google.com>.

# This file is part of gold.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
# MA 02110-1301, USA.

# This file goes with debug_msg.cc, a C++ source file constructed to
# have undefined references.  We compile that file with debug
# information and then try to link it, and make sure the proper errors
# are displayed.  The errors will be found in debug_msg.err.

check()
{
    if ! grep -q "$2" "$1"
    then
	echo "Did not find expected error in $1:"
	echo "   $2"
	echo ""
	echo "Actual error output below:"
	cat "$1"
	exit 1
    fi
}

check_missing()
{
    if grep -q "$2" "$1"
    then
	echo "Found unexpected error in $1:"
	echo "   $2"
	echo ""
	echo "Actual error output below:"
	cat "$1"
	exit 1
    fi
}

# We don't know how the compiler might order these variables, so we
# can't test for the actual offset from .data, hence the regexp.
check debug_msg.err "debug_msg.o: in function fn_array:debug_msg.cc(.data+0x[0-9a-fA-F]*): undefined reference to 'undef_fn1()'"
check debug_msg.err "debug_msg.o: in function fn_array:debug_msg.cc(.data+0x[0-9a-fA-F]*): undefined reference to 'undef_fn2()'"
check debug_msg.err "debug_msg.o: in function badref1:debug_msg.cc(.data+0x[0-9a-fA-F]*): undefined reference to 'undef_int'"

check debug_msg.err "debug_msg.o: in function Base::virtfn():${srcdir}/debug_msg.cc:50: undefined reference to 'undef_fn1()'"
check debug_msg.err "debug_msg.o: in function Derived::virtfn():${srcdir}/debug_msg.cc:55: undefined reference to 'undef_fn2()'"
check debug_msg.err "debug_msg.o: in function int testfn<int>(int):${srcdir}/debug_msg.cc:43: undefined reference to 'undef_fn1()'"
check debug_msg.err "debug_msg.o: in function int testfn<int>(int):${srcdir}/debug_msg.cc:44: undefined reference to 'undef_fn2()'"
check debug_msg.err "debug_msg.o: in function int testfn<int>(int):${srcdir}/debug_msg.cc:45: undefined reference to 'undef_int'"
check debug_msg.err "debug_msg.o: in function int testfn<double>(double):${srcdir}/debug_msg.cc:43: undefined reference to 'undef_fn1()'"
check debug_msg.err "debug_msg.o: in function int testfn<double>(double):${srcdir}/debug_msg.cc:44: undefined reference to 'undef_fn2()'"
check debug_msg.err "debug_msg.o: in function int testfn<double>(double):${srcdir}/debug_msg.cc:45: undefined reference to 'undef_int'"

# Check we detected the ODR (One Definition Rule) violation.
check debug_msg.err ": symbol 'Ordering::operator()(int, int)' defined in multiple places (possible ODR violation):"
check debug_msg.err "odr_violation1.cc:5"
check debug_msg.err "odr_violation2.cc:5"

# When linking together .so's, we don't catch the line numbers, but we
# still find all the undefined variables, and the ODR violation.
check debug_msg_so.err "debug_msg.so: undefined reference to 'undef_fn1()'"
check debug_msg_so.err "debug_msg.so: undefined reference to 'undef_fn2()'"
check debug_msg_so.err "debug_msg.so: undefined reference to 'undef_int'"
check debug_msg_so.err ": symbol 'Ordering::operator()(int, int)' defined in multiple places (possible ODR violation):"
check debug_msg_so.err "odr_violation1.cc:5"
check debug_msg_so.err "odr_violation2.cc:5"

# These messages shouldn't need any debug info to detect:
check debug_msg_ndebug.err "debug_msg_ndebug.so: undefined reference to 'undef_fn1()'"
check debug_msg_ndebug.err "debug_msg_ndebug.so: undefined reference to 'undef_fn2()'"
check debug_msg_ndebug.err "debug_msg_ndebug.so: undefined reference to 'undef_int'"
# However, we shouldn't detect or declare any ODR violation
check_missing debug_msg_ndebug.err "(possible ODR violation)"

exit 0
