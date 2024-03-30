# $NetBSD: t_usage.sh,v 1.19 2024/03/30 17:23:13 rillig Exp $
#
# Copyright (c) 2023 The NetBSD Foundation, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
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
#

: "${lint1:=/usr/libexec/lint1}"


atf_test_case 'suppress_messages'
suppress_messages_body()
{
	printf 'typedef int dummy;\n' > code.c

	# Message IDs are 0-based.
	atf_check \
	    "$lint1" -X 0 code.c /dev/null

	# The largest known message.
	atf_check \
	    "$lint1" -X 378 code.c /dev/null

	# Larger than the largest known message.
	atf_check \
	    -s 'exit:1' \
	    -e "inline:lint1: invalid message ID '379'\n" \
	    "$lint1" -X 379 code.c /dev/null

	# Whitespace is not allowed before a message ID.
	atf_check \
	    -s 'exit:1' \
	    -e "inline:lint1: invalid message ID ' 1'\n" \
	    "$lint1" -X ' 1' code.c /dev/null

	# Whitespace is not allowed after a message ID.
	atf_check \
	    -s 'exit:1' \
	    -e "inline:lint1: invalid message ID '1 '\n" \
	    "$lint1" -X '1 ' code.c /dev/null

	# Multiple message IDs can be comma-separated.
	atf_check \
	    "$lint1" -X '1,2,3,4' code.c /dev/null

	# Whitespace is not allowed after a comma.
	atf_check \
	    -s 'exit:1' \
	    -e "inline:lint1: invalid message ID ' 2'\n" \
	    "$lint1" -X '1, 2, 3, 4' code.c /dev/null

	# Trailing commas are not allowed.
	atf_check \
	    -s 'exit:1' \
	    -e "inline:lint1: invalid message ID ''\n" \
	    "$lint1" -X '1,,,,,,,' code.c /dev/null
}

atf_test_case 'enable_queries'
enable_queries_body()
{
	printf 'typedef int dummy;\n' > code.c

	# Query IDs are 1-based.
	atf_check \
	    -s 'exit:1' \
	    -e "inline:lint1: invalid query ID '0'\n" \
	    "$lint1" -q 0 code.c /dev/null

	# The largest known query.
	atf_check \
	    "$lint1" -q 19 code.c /dev/null

	# Larger than the largest known query.
	atf_check \
	    -s 'exit:1' \
	    -e "inline:lint1: invalid query ID '20'\n" \
	    "$lint1" -q 20 code.c /dev/null

	# Whitespace is not allowed before a query ID.
	atf_check \
	    -s 'exit:1' \
	    -e "inline:lint1: invalid query ID ' 1'\n" \
	    "$lint1" -q ' 1' code.c /dev/null

	# Whitespace is not allowed after a query ID.
	atf_check \
	    -s 'exit:1' \
	    -e "inline:lint1: invalid query ID '1 '\n" \
	    "$lint1" -q '1 ' code.c /dev/null

	# Multiple query IDs can be comma-separated.
	atf_check \
	    "$lint1" -q '1,2,3,4' code.c /dev/null

	# Whitespace is not allowed after a comma.
	atf_check \
	    -s 'exit:1' \
	    -e "inline:lint1: invalid query ID ' 2'\n" \
	    "$lint1" -q '1, 2, 3, 4' code.c /dev/null

	# Trailing commas are not allowed.
	atf_check \
	    -s 'exit:1' \
	    -e "inline:lint1: invalid query ID ''\n" \
	    "$lint1" -q '1,,,,,,,' code.c /dev/null
}

atf_init_test_cases()
{
	atf_add_test_case 'suppress_messages'
	atf_add_test_case 'enable_queries'
}
