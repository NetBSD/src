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
 *	$Id: grf_monitor.h,v 1.2 1994/01/29 06:59:58 chopps Exp $
 */

#if ! defined (_GRF_MONITOR_H)
#define _GRF_MONITOR_H

/* monitor functions */

/* this function will be called for every (maybe) VB if the mode is 
 * the active one */
typedef void     vbl_handler_func (void *);

/* returns the next mode given a mode or the first if NULL */
typedef dmode_t *get_next_mode_func (dmode_t *);

/* returns the currently active mode this can be NULL if none are active */
typedef dmode_t *get_current_mode_func (void);

/* returns the mode that best fits the parameters, or NULL if none fit */
typedef dmode_t *get_best_mode_func (dimen_t *size, u_byte depth);
    
/* allocate a bitmap for use with this monitor see notes on restrictions */
typedef bmap_t *alloc_bitmap_func (u_word w, u_word h, u_word d, u_word f);

/* free a bitmap that was allocated with this monitors alloc_bitmap() 
 * function. */
typedef void    free_bitmap_func (bmap_t *bm);

/* monitor: support different graphic boards and viewmodes. */
struct monitor {
    dll_node_t  node;				  /* a link into the database. */
    u_byte     *name;				  /* a logical name for this monitor. */
    void       *data;				  /* monitor specific data. */

    /* funciton interface to this monitor. */
    vbl_handler_func       *vbl_handler;	  /* gets called on every vbl if not NULL */
    get_next_mode_func     *get_next_mode;	  /* return next mode in list */
    get_current_mode_func  *get_current_mode;	  /* return active mode or NULL */
    get_best_mode_func     *get_best_mode;	  /* return mode that best fits */
    
    alloc_bitmap_func      *alloc_bitmap;	  /* allocate a bitmap for use with this monitor */
    free_bitmap_func       *free_bitmap;	  /* free a bitmap */
};

dmode_t * grf_get_next_mode (monitor_t *m, dmode_t *d);
dmode_t * grf_get_current_mode (monitor_t *);
dmode_t * grf_get_best_mode (monitor_t *m, dimen_t *size, u_byte depth);
bmap_t  * grf_alloc_bitmap (monitor_t *m, u_word w, u_word h, u_word d, u_word f);
void grf_free_bitmap (monitor_t *m, bmap_t *bm);

#endif /* _GRF_MONITOR_H */

