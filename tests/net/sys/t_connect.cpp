//
// Automated Testing Framework (atf)
//
// Copyright (c) 2007 The NetBSD Foundation, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. All advertising materials mentioning features or use of this
//    software must display the following acknowledgement:
//        This product includes software developed by the NetBSD
//        Foundation, Inc. and its contributors.
// 4. Neither the name of The NetBSD Foundation nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
// CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

extern "C" {
#include <arpa/inet.h>
#include <netinet/in.h>
#include <err.h>
#include <unistd.h>
}

#include <cerrno>
#include <cstring>

#include <atf.hpp>

ATF_TEST_CASE(low_port);
ATF_TEST_CASE_HEAD(low_port)
{
    set("descr", "Checks that low-port allocation works");
    set("require.user", "root");
}
ATF_TEST_CASE_BODY(low_port)
{
	struct sockaddr_in sin;
	int sd, val;

	sd = ::socket(AF_INET, SOCK_STREAM, 0);

	val = IP_PORTRANGE_LOW;
	if (::setsockopt(sd, IPPROTO_IP, IP_PORTRANGE, &val,
	                 sizeof(val)) == -1)
		ATF_FAIL(std::string("setsockopt failed: ") +
		         std::strerror(errno));

	std::memset(&sin, 0, sizeof(sin));

	sin.sin_port = htons(80);
	sin.sin_addr.s_addr = inet_addr("204.152.190.12"); // www.NetBSD.org
	sin.sin_family = AF_INET;

	if (::connect(sd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		int err = errno;
		std::string reason = "connect failed: ";
		reason += std::strerror(err);
		if (err == EACCES)
			reason += " (see http://mail-index.netbsd.org/"
			          "source-changes/2007/12/16/0011.html)";
		ATF_FAIL(reason);
	}

	::close(sd);
}

ATF_INIT_TEST_CASES(tcs)
{
    ATF_ADD_TEST_CASE(tcs, low_port);
}
