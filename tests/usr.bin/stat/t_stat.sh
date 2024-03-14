# $NetBSD: t_stat.sh,v 1.1 2024/03/14 21:00:33 rillig Exp $
#
# Copyright (c) 2024 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Roland Illig.
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

atf_test_case string_format
string_format_head() {
	atf_set "descr" "Tests the string output format 'S'"
}
string_format_body() {
	echo 'äöü' > './Ümläute'

	atf_check -o 'inline:plain <Ümläute>\n' \
	    stat -f 'plain <%SN>' 'Ümläute'

	atf_check -o 'inline:right-aligned <           Ümläute>\n' \
	    stat -f 'right-aligned <%20SN>' 'Ümläute'

	atf_check -o 'inline:left-aligned <Ümläute           >\n' \
	    stat -f 'left-aligned <%-20SN>' 'Ümläute'

	# FIXME: invokes undefined behavior in snprintf "%+s"
	atf_check -o 'inline:string-plus <Ümläute>\n' \
	    stat -f 'string-plus <%+SN>' 'Ümläute'

	atf_check -o 'inline:vis <\303\234ml\303\244ute>\n' \
	    stat -f 'vis <%#SN>' 'Ümläute'

	atf_check -o 'inline:vis left-aligned <\303\234ml\303\244ute         >\n' \
	    stat -f 'vis left-aligned <%#-30SN>' 'Ümläute'

	atf_check -o 'inline:vis right-aligned <         \303\234ml\303\244ute>\n' \
	    stat -f 'vis right-aligned <%#30SN>' 'Ümläute'
}

atf_init_test_cases() {
	atf_add_test_case string_format
}
