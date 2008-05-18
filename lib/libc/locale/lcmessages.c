/*	$NetBSD: lcmessages.c,v 1.1.2.2 2008/05/18 12:30:17 yamt Exp $	*/
/*
 * Copyright (c) 2008, The NetBSD Foundation, Inc.
 * All rights reserved.
 * 
 * This code is derived from software contributed to The NetBSD Foundation
 * by Brian Ginsbach.
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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: lcmessages.c,v 1.1.2.2 2008/05/18 12:30:17 yamt Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/localedef.h>

#include <assert.h>

#include "localeio.h"
#include "lcmessages.h"

#define NSTRINGS (sizeof(_MessagesLocale)/sizeof(const char **))

int
__loadmessages(const char *filename)
{

	_DIAGASSERT(filename != NULL);

	return __loadlocale(filename, NSTRINGS, 0, sizeof(_MessagesLocale),
			    &_CurrentMessagesLocale, &_DefaultMessagesLocale);
}

#ifdef LOCALE_DEBUG

#include "namespace.h"

#include <stdio.h>

void
__dumptimelocale()
{

	(void)printf("yesexpr = \"%s\"\n", _CurrentMessagesLocale->yesexpr);
	(void)printf("noexpr = \"%s\"\n", _CurrentMessagesLocale->noexpr);
	(void)printf("yesstr = \"%s\"\n", _CurrentMessagesLocale->yesstr);
	(void)printf("nostr = \"%s\"\n", _CurrentMessagesLocale->nostr);
}
#endif	/* LOCALE_DEBUG */
