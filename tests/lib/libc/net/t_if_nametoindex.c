/*	$NetBSD: t_if_nametoindex.c,v 1.1 2018/08/06 04:50:11 msaitoh Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: t_if_nametoindex.c,v 1.1 2018/08/06 04:50:11 msaitoh Exp $");

#include <atf-c.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <net/if.h>
#include <err.h>
#include <string.h>
#include <errno.h>

ATF_TC(tc_if_nametoindex);
ATF_TC_HEAD(tc_if_nametoindex, tc)
{
	atf_tc_set_md_var(tc, "descr", "Check that if_nametoindex(3) works");  
}
 
ATF_TC_BODY(tc_if_nametoindex, tc)
{
	unsigned int r;

	r = if_nametoindex("lo0");
	if (r == 0)
		atf_tc_fail("failed on lo0");

	r = if_nametoindex("foo");
	if (errno != ENXIO)
		atf_tc_fail("foo's errno != ENXIO");
}

ATF_TP_ADD_TCS(tp)
{       
 
	ATF_TP_ADD_TC(tp, tc_if_nametoindex);
        
        return atf_no_error();
}       
