# $NetBSD: unif.awk,v 1.3 2003/08/30 21:54:42 dsl Exp $

# Copyright (c) 2003 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by David Laight.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of The NetBSD Foundation nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

# 'unif' lines of file
#
# usage: awk -f unif.awk -v defines=varlist file
#
# looks for blocks of the form:
#
# .if var [|| var]
# ...
# .else
# ...
# .endif
#
# and removes the unwanted lines
# There is some error detection...

BEGIN {
	split(defines, defns)
	for (v in defns)
		deflist[defns[v]] = 1
	delete defns
	nested = 0
	skip = 0
}

/^\.if/ {
	nested++
	else_ok[nested] = 1
	if (skip)
		next
	for (i = 2; i <= NF; i += 2) {
		if ($i in deflist)
			next
		if ($(i+1) != "" && $(i+1) != "||")
			exit 1
	}
	if (!skip)
		skip = nested
	next
}

/^\.else/  {
	if (!else_ok[nested])
		exit 1
	else_ok[nested] = 0
	if (skip == nested)
		skip = 0
	else if (!skip)
		skip = nested
	next
}

/^\.endif/ {
	if (nested == 0)
		exit 1
	if (skip == nested)
		skip = 0
	nested--
	next
}

{
	if (skip == 0)
		print
}
	
END {
	if (nested != 0)
		exit 1
}
