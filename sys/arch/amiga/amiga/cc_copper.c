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
 *	$Id: cc_copper.c,v 1.2 1994/01/29 06:58:44 chopps Exp $
 */

#include "cc_copper.h"

/* Wait till end of frame. We should probably better use the
 * sleep/wakeup system newly introduced in the vbl manager (ch?) */
void
wait_tof (void)
{
    while (!(custom.vposr & 0x0007)) {	  /* wait until bottom of frame.
	;
    }
    while (custom.vposr & 0x0007) {	  /* wait until until top of frame. */
	;
    }
	
    if (!custom.vposr & 0x8000) {		  /* we are on short frame. */
	while (!(custom.vposr & 0x8000)) {	  /* wait for long frame bit set. */
	    ;
	}
    }
}

cop_t *
find_copper_inst (cop_t *l, u_word inst)
{
    cop_t *r = NULL;
    while ((l->cp.data & 0xff01ff01) != 0xff01ff00) {
	if (l->cp.inst.opcode == inst) {
	    r = l;
	    break;
	}
	l++;
    }
    return (r);
}

void
install_copper_list (struct copper_list *l)
{
    wait_tof ();
    wait_tof ();
    custom.cop1lc = l;
}


#if ! defined (AMIGA_TEST)
void
cc_init_copper (void)
{
    
}

#else /* AMIGA_TEST */
#define	HARDWARE_CUSTOM_H
#include <graphics/view.h>
#include <graphics/gfxbase.h>
#include <inline/graphics.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/intuition.h>
#include <inline/intuition.h>
#include <inline/exec.h>

struct View *old;
struct Task *t;
extern struct GfxBase *GfxBase;
void
cc_init_copper (void)
{

    old = GfxBase->ActiView;
    t = FindTask (NULL);
    SetTaskPri (t, 127);
    LoadView (NULL);
    WaitTOF ();
    WaitTOF ();
}

void
cc_deinit_copper (void)
{
    if (t) {
	if (old) {
	    LoadView (old);
	    WaitTOF ();
	    WaitTOF ();
	    custom.cop1lc = GfxBase->copinit;
	    RethinkDisplay ();
	}
	SetTaskPri (t, 0);
    }
}

#endif

/* level 3 interrupt */
void
copper_handler (void)
{
    custom.intreq = INTF_COPER;  
}
