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
 *	$Id: grf_mode.h,v 1.2 1994/01/29 06:59:55 chopps Exp $
 */

#if ! defined (_GRF_MODE_H)
#define _GRF_MODE_H

/* mode functions */

/* allocates a view for use with this mode. */
typedef view_t *alloc_view_func (dmode_t *mode, dimen_t *dim, u_byte depth);

/* returns the currently active view for this mode 
 * NOTE: this need not be the view that is displaying on
 *       the monitor. */
typedef view_t *get_current_view_func (dmode_t *);

/* returns the monitor that owns this mode */
typedef monitor_t  *get_monitor_func (dmode_t *);
    
struct display_mode {
    dll_node_t node;				  /* a link into a monitor's mode list. */
    u_byte    *name;				  /* logical name for mode. */
    dimen_t    nominal_size;			  /* best fit. */
    void      *data;				  /* mode specific flags. */

    /* mode functions */
    alloc_view_func       *alloc_view;		  /* allocate a view for this mode. */
    get_current_view_func *get_current_view;	  /* get active view. */
    get_monitor_func      *get_monitor;		  /* get monitor that mode belongs to */
};

view_t *grf_get_current_view (dmode_t *d);
monitor_t *grf_get_monitor (dmode_t *d);

#endif /* _GRF_MODE_H */

