/*
 * Copyright (c) 1994 Christian E. Hopps
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
 *      This product includes software developed by Christian E. Hopps.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 *	$Id: grf_view.c,v 1.2 1994/01/29 07:00:02 chopps Exp $
 */
#include "grf_types.h"
#include "grf_view.h"
#include "grf_mode.h"

/* defined in grf.c */
dmode_t *get_best_display_mode (u_long width, u_long height, u_byte depth);

void
grf_display_view (view_t *v)
{
    v->display_view (v);
}

view_t *
grf_alloc_view (dmode_t *d, dimen_t *dim, u_byte depth)
{
    if (!d) {
	d = get_best_display_mode (dim->width, dim->height, depth);
    }
    if (d) {
	return (d->alloc_view (d, dim, depth));
    }
    return (NULL);
}

void
grf_remove_view (view_t *v)
{
    v->remove_view (v);
}

void
grf_free_view (view_t *v)
{
    v->free_view (v);
}

dmode_t *
grf_get_display_mode (view_t *v)
{
    return (v->get_display_mode (v));
}

int
grf_get_colormap (view_t *v, colormap_t *cm)
{
    return (v->get_colormap (v, cm));
}

int
grf_use_colormap (view_t *v, colormap_t *cm)
{
    return (v->use_colormap (v, cm));
}


