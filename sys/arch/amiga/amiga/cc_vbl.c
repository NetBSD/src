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
 *	$Id: cc_vbl.c,v 1.2 1994/01/29 06:58:54 chopps Exp $
 */

/* structure for a vbl_function. */
struct vbl_node;				  /* shut gcc up about no structure. */

#define CC_VBL_C
#include "types.h"
#include "cc_types.h"
#include "cc_vbl.h"

/*==================================================
 * Internal structures.
 *==================================================*/

/* Vertical Blank */
struct vbl_node {
    dll_node_t node;				  /* Private. */
    short   priority;				  /* Private. */
    short   flags;				  /* Private. */
    void  (*function)(register void *);			  /* put your function pointer here. */
    void   *data;				  /* functions data. */
};

enum vbl_node_bits {
    VBLNB_OFF,					  /* don't call me right now. */
    VBLNB_TURNOFF,				  /* turn function off. */
    VBLNB_REMOVE,				  /* remove me on next vbl round and clear flag. */
};

enum vlb_node_flags {
    VBLNF_OFF = 1 << VBLNB_OFF,
    VBLNF_TURNOFF = 1 << VBLNB_TURNOFF,
    VBLNF_REMOVE = 1 << VBLNB_REMOVE,		  
};    


/*==================================================
 * Global structures
 *==================================================*/

/* - vbl_list */
/* the actual vbl list only the vbl handler allowed to touch this list. */
static dll_list_t vbl_list;

/* - vbl_sync */
/* if this is set to 1 when the vbl handler is called, it resets it to zero when done. */
static int vbl_sync_flag;

/* - vbl enabled */
/* if this is set to 1 then the vbl handler will perform, otherwise will only reset the */
/* vbl_sync flag. */
static int vbl_disabled;

/*==================================================
 * INLINES
 *==================================================*/
/* this function will return as soon as the vertical blank has finished. */
void inline
vbl_sync (void)
{
    vbl_sync_flag = 1;
    while (vbl_sync_flag)
       ;
}

void inline
vbl_disable (void)
{
    vbl_disabled++;
}

void inline
vbl_enable (void)
{
    if (vbl_disabled) {
	vbl_disabled--;
    } else {
	/* probably should panic */
    }
}

/*==================================================
 * VERTICAL BLANK HANDLERS
 *==================================================*/

void vbl_handler (void);
/* structure for a vbl_function. */

struct vbl_node *add_node;			  /* function to add to the list. */


void
wait_for_vbl_function_removal (struct vbl_node *n)
{
    if (n->node.next) {				  
	while (n->flags & VBLNF_REMOVE)
            ;
    } else {
	/* probably should panic */
    }
}

void
turn_vbl_function_off (struct vbl_node *n)
{
    if (!(n->flags & VBLNF_OFF)) {
	n->flags |= VBLNF_TURNOFF;		  /* mark to turn off. */
	while (!(n->flags & VBLNF_OFF))		  /* wait for this to occur. */
            ;
    }
}

/* allow function to be called on next vbl interrupt. */
void
turn_vbl_function_on (struct vbl_node *n)
{
    n->flags &= (short) ~(VBLNF_OFF);
}

/* walk list search for correct spot to place node. */
/* WARNING: do not call this function from interrupts that have a */
/* higher priority than VBL (HL3) */
void                    
add_vbl_function (struct vbl_node *add, short priority, void *data)
{
    /* we are executing in top half of kernel and so we are syncronous. */
    u_word ie = custom.intenar;
    struct vbl_node *n;

    add->data = data;
    if (ie & INTF_INTEN) {
	/* master interrupts enabled. */
	vbl_disable ();				  /* disable the handler. */
	if (ie & INTF_VERTB) {
	    /* vertical blank interrupts enabled */
	    vbl_enable ();			  /* enable the handler. */
	    add_node = add;			  /* set add node it will be added on next vbl. */

	    while (add_node) {			  /* wait for vbl it will clear add node after */
		;				  /* doing this. then exit. */
	    }
	    return;
	}
    }
    /* NOTE: if master interrupts are enabled but VBL are not, our handler */
    /*       will be disabled so that no chance of a interupted list manipulation */
    /*       is possible. this could happen IF poor interrupt coding practices have */
    /*       been implemented and a interrupt occurs that upstages us and */
    /*       re-enables VBL interrupts. (ICK) I don't think this is the case but */
    /*       I might as well guard against it. (its only 3 inline C instructions.)*/

    for (n = (struct vbl_node *)vbl_list.head; n->node.next; n = (struct vbl_node *)n->node.next) {
	if (add->priority > n->priority) {
	    dinsert (&n->node, &add->node);	  /* insert add_node before. */
	    add = NULL;
	    break;
	}
    }
    if (add) {
	dadd_tail (&vbl_list, &add->node);	  /* add node to tail. */
    }
    if (ie & INTF_VERTB) {
	vbl_enable ();				  /* enable handler if nec. */
    }
}

void
remove_vbl_function (struct vbl_node *n)
{
    n->flags |= VBLNF_REMOVE;
}


/* This is executing in bottom half of kernel. (read: anytime.) */
void inline
vbl_call_function (struct vbl_node *n)
{
    /* handle case of NULL */
    if (n && !(n->flags & VBLNF_OFF)) {
	n->function (n->data);
    }
}

/* This is executing in bottom half of kernel. (read: anytime.) */
/* Level 3 hardware interrupt */
void
vbl_handler (void)
{
    struct vbl_node *n, *tmp;

    vbl_sync_flag = 0;				  /* always written to. */

    /* if not enabled skip everything. */
    if (vbl_disabled) {
	goto ret_int;				  /* NOTE: end of function. */
    }
    
    /* handle all vbl functions */
    for (n = (struct vbl_node *)vbl_list.head; n->node.next; n = tmp) {
	if (add_node && add_node->priority > n->priority) {
	    dinsert (&n->node, &add_node->node); /* insert add_node before. */
	    n = add_node;			  /* process add_node first. */
	    add_node = NULL;			  /* and signal we are done with add_node. */
	}
	tmp = (struct vbl_node *)n->node.next;
	if (n->flags & VBLNF_REMOVE) {
	    dremove (&n->node);		  /* remove the node. */
	    n->flags &= ~(VBLNF_REMOVE);	  /* mark as removed. */
	    n->node.next = NULL;		  /* for sanity checks. */
	} else if (n->flags & VBLNF_TURNOFF) {
	    n->flags |= VBLNF_OFF;
	    n->flags &= ~(VBLNF_TURNOFF);
	} else {
	    vbl_call_function (n);		  /* execute the interrupt. */
	}
    }
    if (add_node) {
	dadd_tail (&vbl_list, &add_node->node); /* add node to tail. */
	vbl_call_function (add_node);		  /* call it. */
	add_node = NULL;			  /* signal we are done with it. */
    }
  ret_int:
    custom.intreq = INTF_VERTB;
}

#if ! defined (AMIGA_TEST)
void
cc_init_vbl (void)
{
    dinit_list (&vbl_list);
    custom.intena = INTF_SETCLR | INTF_VERTB; /* init vertical blank interrupts */
}

#else /* AMIGA_TEST */
#include <exec/interrupts.h>
#include <inline/exec.h>

void
test_vbl_handler (void)
{
    vbl_handler ();
}

struct Interrupt test_vbl;

void
cc_init_vbl (void)
{
    test_vbl.is_Code = vbl_handler;
    dinit_list (&vbl_list);
    AddIntServer (INTB_VERTB, &test_vbl);
}

void
cc_deinit_vbl (void)
{
    vbl_disable ();
    vbl_sync ();
    RemIntServer (INTB_VERTB, &test_vbl);
}
#endif /*AMIGA_TEST*/
