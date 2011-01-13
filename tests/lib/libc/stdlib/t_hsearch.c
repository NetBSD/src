/* $NetBSD: t_hsearch.c,v 1.1 2011/01/13 14:32:35 pgoyette Exp $ */

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

/*
 * Copyright (c) 2001 Christopher G. Demetriou
 * All rights reserved.
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * <<Id: LICENSE,v 1.2 2000/06/14 15:57:33 cgd Exp>>
 */

#include <sys/cdefs.h>
__COPYRIGHT("@(#) Copyright (c) 2008\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_hsearch.c,v 1.1 2011/01/13 14:32:35 pgoyette Exp $");

#include <errno.h>
#include <search.h>
#include <string.h>
#include <stdio.h>

#include <atf-c.h>

#define REQUIRE_ERRNO(x) ATF_REQUIRE_MSG(x, "%s", strerror(errno))

ATF_TC(basic);

ATF_TC_HEAD(basic, tc)
{

	atf_tc_set_md_var(tc, "descr", "Checks basic insertions and searching");
}

ATF_TC_BODY(basic, tc)
{
	ENTRY e, *ep;
	char ch[2];
	int i;

	REQUIRE_ERRNO(hcreate(16) != 0);

	/* ch[1] should be constant from here on down. */
	ch[1] = '\0';

	/* Basic insertions.  Check enough that there'll be collisions. */
	for (i = 0; i < 26; i++) {
		ch[0] = 'a' + i;
		e.key = strdup(ch);	/* ptr to provided key is kept! */
		ATF_REQUIRE(e.key != NULL);
		e.data = (void *)(long)i;

		ep = hsearch(e, ENTER);

		ATF_REQUIRE(ep != NULL);
		ATF_REQUIRE_STREQ(ep->key, ch);
		ATF_REQUIRE_EQ((long)ep->data, i);
	}

	/* e.key should be constant from here on down. */
	e.key = ch;

	/* Basic lookups. */
	for (i = 0; i < 26; i++) {
		ch[0] = 'a' + i;

		ep = hsearch(e, FIND);

		ATF_REQUIRE(ep != NULL);
		ATF_REQUIRE_STREQ(ep->key, ch);
		ATF_REQUIRE_EQ((long)ep->data, i);
	}

	hdestroy();
}

ATF_TC(duplicate);

ATF_TC_HEAD(duplicate, tc)
{

	atf_tc_set_md_var(tc, "descr", "Checks that inserting duplicate "
	    "doesn't overwrite existing data");
}

ATF_TC_BODY(duplicate, tc)
{
	ENTRY e, *ep;

	REQUIRE_ERRNO(hcreate(16));

	e.key = strdup("a");
	ATF_REQUIRE(e.key != NULL);
	e.data = (void *)(long) 0;

	ep = hsearch(e, ENTER);

	ATF_REQUIRE(ep != NULL);
	ATF_REQUIRE_STREQ(ep->key, "a");
	ATF_REQUIRE_EQ((long)ep->data, 0);

	e.data = (void *)(long)12345;

	ep = hsearch(e, ENTER);
	ep = hsearch(e, FIND);

	ATF_REQUIRE(ep != NULL);
	ATF_REQUIRE_STREQ(ep->key, "a");
	ATF_REQUIRE_EQ((long)ep->data, 0);

	hdestroy();
}

ATF_TC(nonexistent);

ATF_TC_HEAD(nonexistent, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks searching for non-existent entry");
}

ATF_TC_BODY(nonexistent, tc)
{
	ENTRY e, *ep;

	REQUIRE_ERRNO(hcreate(16));

	e.key = strdup("A");
	ep = hsearch(e, FIND);
	ATF_REQUIRE_EQ(ep, NULL);

	hdestroy();
}

ATF_TC(two);

ATF_TC_HEAD(two, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Checks that searching doesn't overwrite previous search results");
}

ATF_TC_BODY(two, tc)
{
	ENTRY e, *ep, *ep2;
	char *sa, *sb;

	ATF_REQUIRE((sa = strdup("a")) != NULL);
	ATF_REQUIRE((sb = strdup("b")) != NULL);

	REQUIRE_ERRNO(hcreate(16));

	e.key = sa;
	e.data = (void*)(long)0;

	ep = hsearch(e, ENTER);

	ATF_REQUIRE(ep != NULL);
	ATF_REQUIRE_STREQ(ep->key, "a");
	ATF_REQUIRE_EQ((long)ep->data, 0);

	e.key = sb;
	e.data = (void*)(long)1;

	ep = hsearch(e, ENTER);
	
	ATF_REQUIRE(ep != NULL);
	ATF_REQUIRE_STREQ(ep->key, "b");
	ATF_REQUIRE_EQ((long)ep->data, 1);

	e.key = sa;
	ep = hsearch(e, FIND);

	e.key = sb;
	ep2 = hsearch(e, FIND);

	ATF_REQUIRE(ep != NULL);
	ATF_REQUIRE_STREQ(ep->key, "a");
	ATF_REQUIRE_EQ((long)ep->data, 0);

	ATF_REQUIRE(ep2 != NULL);
	ATF_REQUIRE_STREQ(ep2->key, "b");
	ATF_REQUIRE_EQ((long)ep2->data, 1);
	
	hdestroy();
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, basic);
	ATF_TP_ADD_TC(tp, duplicate);
	ATF_TP_ADD_TC(tp, nonexistent);
	ATF_TP_ADD_TC(tp, two);

	return atf_no_error();
}
