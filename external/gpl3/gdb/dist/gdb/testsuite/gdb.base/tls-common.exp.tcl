# Copyright 2024-2025 Free Software Foundation, Inc.
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# Require statement, variables and procs used by tls-nothreads.exp,
# tls-multiobj.exp, and tls-dlobj.exp.

# The tests listed above are known to work for the targets listed on
# the 'require' line, below.
#
# At the moment, only the Linux target is listed, but, ideally, these
# tests should be run on other targets too.  E.g, testing on FreeBSD
# shows many failures which should be addressed in some fashion before
# enabling it for that target.

require {is_any_target "*-*-linux*"}

# These are the targets which have support for internal TLS lookup:

set internal_tls_linux_targets {"x86_64-*-linux*" "aarch64-*-linux*"
				"riscv*-*-linux*" "powerpc64*-*-linux*"
				"s390x*-*-linux*"}

# The "maint set force-internal-tls-address-lookup" command is only
# available for certain Linux architectures.  Don't attempt to force
# use of internal TLS support for architectures which don't support
# it.

if [is_any_target {*}$internal_tls_linux_targets] {
    set internal_tls_iters { false true }
} else {
    set internal_tls_iters { false }
}

# Set up a kfail with message KFAIL_MSG when KFAIL_COND holds, then
# issue gdb_test with command CMD and regular expression RE.

proc gdb_test_with_kfail {cmd re kfail_cond kfail_msg} {
    if [uplevel 1 [list expr $kfail_cond]] {
	setup_kfail $kfail_msg *-*-*
    }
    gdb_test $cmd $re
}
