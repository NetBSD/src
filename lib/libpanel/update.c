/*	$NetBSD: update.c,v 1.1 2015/10/26 23:09:49 uwe Exp $ */

/*
 * Copyright (c) 2015 Valery Ushakov
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
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: update.c,v 1.1 2015/10/26 23:09:49 uwe Exp $");

#include "panel_impl.h"


void
update_panels(void)
{
	PANEL *p;

	/*
	 * For each panel tell panels above it they need to refresh
	 * regions that overlap (are above) this panel.  This ensures
	 * that even if a panel below was touched, it's still
	 * overwritten by a panel above.
	 *
	 * Note that we also need to do this during "destructive"
	 * operations (hide, move, replace window - which see).
	 */
	FOREACH_PANEL (p) {
		PANEL *above = p;
		while ((above = PANEL_ABOVE(above)) != NULL) {
			touchoverlap(p->win, above->win);
		}
	}

	/*
	 * This is what effects Z-order: the window updated later
	 * overwrites contents of the windows below (before) it.
	 */
	FOREACH_PANEL (p) {
		wnoutrefresh(p->win);
	}
}
