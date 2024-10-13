#	$NetBSD: t_custom_headers.sh,v 1.1.2.2 2024/10/13 16:06:37 martin Exp $
#
# Copyright (c) 2024 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Sunil Nimmagadda.
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

atf_test_case custom_headers cleanup
custom_headers_head()
{
	atf_require_prog ftp
	atf_set "descr" "Check for custom HTTP headers"
}

HTTPD_PID=./.__httpd.pid
custom_headers_body()
{
	# start httpd in daemon mode
	atf_check -s exit:0 \
	    /usr/libexec/httpd -P $HTTPD_PID -I 8080 -b -C .sh /bin/sh \
	    -c "$(atf_get_srcdir)" .

	atf_check \
	    -o inline:'HTTP_X_ORIGIN=example.com\nHTTP_X_RATE_LIMIT=1000\n' \
	    ftp -V -o - \
		-H 'X-Origin: example.com' \
		-H 'X-Rate-Limit: 1000' \
		http://127.0.0.1:8080/cgi-bin/custom_headers.sh
}

custom_headers_cleanup()
{
	if [ -f "$HTTPD_PID" ]; then
		echo kill -9 "$(cat $HTTPD_PID)"
		kill -9 "$(cat $HTTPD_PID)"
		echo '# wait for httpd to exit'
		sleep 1
	fi
}

atf_init_test_cases()
{
	atf_add_test_case custom_headers
}
