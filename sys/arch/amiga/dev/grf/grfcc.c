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
 *	$Id: grfcc.c,v 1.2 1994/01/29 07:00:08 chopps Exp $
 */

#include <stdlib.h>

#include "../../amiga/cc_vbl.h"
#include "grf_types.h"
#include "grf_mode.h"
#include "grf_monitor.h"

/* add your monitor here. */
monitor_t *cc_init_monitor (void);
/* and here. */
monitor_t *(*init_monitor[])(void) = {
    cc_init_monitor, 
    NULL
};

dll_list_t instance_monitors;
dll_list_t *monitors;

struct vbl_node grf_vbl_node;

void
grf_vbl_function (void * data)
{
    dll_node_t *n = monitors->head;

    while (n->next) {
	monitor_t *m = (monitor_t *)n;
	if (m->vbl_handler) {
	    m->vbl_handler (m);
	}
	n = n->next;
    }
}

/*
 * XXX: called from ite console init routine.
 * Does just what configure will do later but without printing anything.
 */


int
grfcc_probe ()
{
    int i = 0;

    grf_vbl_node.function = grf_vbl_function;
    
    if (NULL == monitors) {
	dinit_list (&instance_monitors);
	monitors = &instance_monitors;
    
	while (init_monitor[i]) {
	    init_monitor[i] ();
	    i++;
	}
	if (i) {
	    add_vbl_function (&grf_vbl_node, 1, 0);
	    return (1);
	}
        return (0);
    }
    return (1);
}

void
grfcc_config (void)
{
    grfprobe ();
}

dmode_t *
get_best_display_mode (u_long width, u_long height, u_byte depth)
{
    dmode_t *save = NULL;
    monitor_t *m = (monitor_t *)monitors->head;
    long dt;
    
    while (m->node.next) {
	dimen_t dim;
	dmode_t *d;
	long dx, dy, ct;
	
	dim.width = width;
	dim.height = height;
	d = m->get_best_mode (&dim, depth);
	if (d) {
	    dx = abs (d->nominal_size.width - width);
	    dy = abs (d->nominal_size.height - height);
	    ct = dx + dy;

	    if (ct < dt || save == NULL) {
		save = d;
		dt = ct;
	    }
	}	
	m = (monitor_t *)m->node.next;
    }
    return (save);
}

