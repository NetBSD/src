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
 *	$Id: cc.c,v 1.3 1994/02/13 21:13:11 chopps Exp $
 */

#include <sys/types.h>
#include <sys/param.h>

#include <amiga/amiga/custom.h>
#include <amiga/amiga/dlists.h>
#include <amiga/amiga/cc.h>

#if defined (__GNUC__)
#define INLINE inline
#else
#define INLINE
#endif

/* init all the "custom chips" */
void
custom_chips_init ()
{
	cc_init_chipmem ();
	cc_init_vbl ();
	cc_init_audio ();
	cc_init_blitter ();
	cc_init_copper ();
}

/*
 * Vertical blank iterrupt sever chains.
 */

static dll_list_t vbl_list;

/* - vbl_sync */
/* if this is set to 1 when the vbl handler is called, it resets it to zero when done. */
static int vbl_sync_flag;

/* - vbl enabled */
/* if this is set to 1 then the vbl handler will perform, otherwise will only reset the */
/* vbl_sync flag. */
static int vbl_disabled;

struct vbl_node *add_node;			  /* function to add to the list. */

/* this function will return as soon as the vertical blank has finished. */
void INLINE
vbl_sync ()
{
	vbl_sync_flag = 1;
	while (vbl_sync_flag)
		;
}

void INLINE
vbl_disable ()
{
	vbl_disabled++;
}

void INLINE
vbl_enable ()
{
	if (vbl_disabled) 
		vbl_disabled--;
}

void
wait_for_vbl_function_removal (n)
	struct vbl_node *n;
{
	if (n->node.next)
		while (n->flags & VBLNF_REMOVE) 
			;
}

void 
turn_vbl_function_off (n)
	struct vbl_node *n;
{
	if (!(n->flags & VBLNF_OFF)) {
		n->flags |= VBLNF_TURNOFF;
		while (!(n->flags & VBLNF_OFF)) 
			;
	}
}

/* allow function to be called on next vbl interrupt. */
void
turn_vbl_function_on (n)
	struct vbl_node *n;
{
	n->flags &= (short) ~(VBLNF_OFF);
}

/* walk list search for correct spot to place node. */
/* WARNING: do not call this function from interrupts that have a */
/* higher priority than VBL (HL3) */
void                    
add_vbl_function (add, priority, data)
	struct vbl_node *add;
	short priority;
	void *data;
{
	/* we are executing in top half of kernel and so we are syncronous. */
	u_short ie = custom.intenar;
	struct vbl_node *n;

	add->data = data;
	if (ie & INTF_INTEN) {
		/* master interrupts enabled. */
		vbl_disable ();		/* disable the handler. */
		if (ie & INTF_VERTB) {
			/* vertical blank interrupts enabled */
			vbl_enable ();	/* enable the handler. */
			add_node = add;	/* set add node it will be added on next vbl. */

			while (add_node) /* wait for vbl it will clear add node after */
				;	 /* doing this. then exit. */
			
			return;
		}
	}
	/* NOTE: if master interrupts are enabled but VBL are not, our handler */
	/*       will be disabled so that no chance of a interupted list manipulation */
	/*       is possible. this could happen IF poor interrupt coding practices have */
	/*       been implemented and a interrupt occurs that upstages us and */
	/*       re-enables VBL interrupts. (ICK) I don't think this is the case but */
	/*       I might as well guard against it. (its only 3 inline C instructions.)*/
	
	for (n = (struct vbl_node *)vbl_list.head;
	     n->node.next;
	     n = (struct vbl_node *)n->node.next) {
		if (add->priority > n->priority) {
			/* insert add_node before. */
			dinsert (&n->node, &add->node);	
			add = NULL;
			break;
		}
	}
	if (add) 
		dadd_tail (&vbl_list, &add->node); /* add node to tail. */
	if (ie & INTF_VERTB) 
		vbl_enable ();		/* enable handler if nec. */
}

void
remove_vbl_function (n)
	struct vbl_node *n;
{
	n->flags |= VBLNF_REMOVE;
}


/* This is executing in bottom half of kernel. */
void
vbl_call_function (n)
	struct vbl_node *n;
{
	if (n && !(n->flags & VBLNF_OFF)) 
		n->function (n->data);
}

/* This is executing in bottom half of kernel. */
/* Level 3 hardware interrupt */
void
vbl_handler ()
{
	struct vbl_node *n, *tmp;

	vbl_sync_flag = 0;		/* always written to. */

	/* if not enabled skip everything. */
	if (vbl_disabled) 
		goto ret_int;		/* NOTE: end of function. */
    
	/* handle all vbl functions */
	for (n = (struct vbl_node *)vbl_list.head; n->node.next; n = tmp) {
		if (add_node && add_node->priority > n->priority) {
			/* insert, process and signal done. */
			dinsert (&n->node, &add_node->node);
			n = add_node;
			add_node = NULL; 
		}
		tmp = (struct vbl_node *)n->node.next;
		if (n->flags & VBLNF_REMOVE) {
			dremove (&n->node);
			n->flags &= ~(VBLNF_REMOVE);
			n->node.next = NULL;
		} else if (n->flags & VBLNF_TURNOFF) {
			n->flags |= VBLNF_OFF;
			n->flags &= ~(VBLNF_TURNOFF);
		} else {
			/* execute the interrupt. */
			vbl_call_function (n); 
		}
	}
	if (add_node) {
		/* add node to tail. */ 
		dadd_tail (&vbl_list, &add_node->node);
		vbl_call_function (add_node);
		add_node = NULL;
	}
	ret_int:
	custom.intreq = INTF_VERTB;
}

void
cc_init_vbl ()
{
	dinit_list (&vbl_list);
	/* init vertical blank interrupts */
	custom.intena = INTF_SETCLR | INTF_VERTB; 
}


/*
 * Blitter stuff.
 */

void
cc_init_blitter ()
{
}

/* test twice to cover blitter bugs if BLTDONE (BUSY) is set it is not done. */
int
is_blitter_busy ()
{
	volatile u_short bb = (custom.dmaconr & DMAF_BLTDONE);
	if ((custom.dmaconr & DMAF_BLTDONE) || bb) 
		return (1);
	return (0);
}

void
wait_blit ()
{
	/* V40 state this covers all blitter bugs. */
	while (is_blitter_busy ()) 
		;
}

void
blitter_handler ()
{
	custom.intreq = INTF_BLIT;
}


void
do_blit (size)
	u_short size;
{
	custom.bltsize = size;
}

void
set_blitter_control (con0, con1)
	u_short con0, con1;
{
	custom.bltcon0 = con0;
	custom.bltcon1 = con1;
}

void
set_blitter_mods (a, b, c, d)
	u_short a, b, c, d;
{
	custom.bltamod = a;
	custom.bltbmod = b;
	custom.bltcmod = c;
	custom.bltdmod = d;
}

void
set_blitter_masks (fm, lm)
	u_short fm, lm;
{
	custom.bltafwm = fm;
	custom.bltalwm = lm;
}

void
set_blitter_data (da, db, dc)
	u_short da, db, dc;
{
	custom.bltadat = da;
	custom.bltbdat = db;
	custom.bltcdat = dc;
}

void
set_blitter_pointers (a, b, c, d)
	void *a, *b, *c, *d;
{
	custom.bltapt = a;
	custom.bltbpt = b;
	custom.bltcpt = c;
	custom.bltdpt = d;
}

/*
 * Copper Stuff.
 */


/* Wait till end of frame. We should probably better use the
 * sleep/wakeup system newly introduced in the vbl manager (ch?) */
void
wait_tof ()
{
	/* wait until bottom of frame. */
	while (!(custom.vposr & 0x0007))
		;
	
	/* wait until until top of frame. */
	while (custom.vposr & 0x0007) 
		;
	
	
	if (!custom.vposr & 0x8000) 
		while (!(custom.vposr & 0x8000)) /* we are on short frame. */	
			;			/* wait for long frame bit set. */
}

cop_t *
find_copper_inst (l, inst)
	cop_t *l;
	u_short inst;
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
install_copper_list (l)
	cop_t *l;
{
	wait_tof ();
	wait_tof ();
	custom.cop1lc = l;
}


void
cc_init_copper ()
{
}

/* level 3 interrupt */
void
copper_handler ()
{
	custom.intreq = INTF_COPER;  
}

/*
 * Audio stuff.
 */

struct audio_channel {
	u_short  play_count;
};

/* - channel[4] */
/* the data for each audio channel and what to do with it. */
static struct audio_channel channel[4];

/* audio vbl node for vbl function  */
struct vbl_node audio_vbl_node;    

void
cc_init_audio ()
{
	int i;

	/* disable all audio interupts */
	custom.intena = INTF_AUD0|INTF_AUD1|INTF_AUD2|INTF_AUD3;

	/* initialize audio channels to off. */
	for (i=0; i < 4; i++) {
		channel[i].play_count = 0;
	};
}


/* Audio Interrupt Handler */
void
audio_handler ()
{
	u_short audio_dma = custom.dmaconr;
	u_short disable_dma = 0;
	int i;

	/* only check channels who have DMA enabled. */
	audio_dma &= (DMAF_AUD0|DMAF_AUD1|DMAF_AUD2|DMAF_AUD3);

	/* disable all audio interupts with DMA set */
	custom.intena = (audio_dma << 7);

	/* if no audio dma enabled then exit quick. */
	if (!audio_dma) {
		/* clear all interrupts. */
		custom.intreq = INTF_AUD0|INTF_AUD1|INTF_AUD2|INTF_AUD3; 
		goto end_int;		/* cleanup. */
	}

	for (i = 0; i < 4; i++) {
		u_short flag = (1 << i);
		u_short ir = custom.intreqr;
		/* check if this channel's interrupt is set */
		if (ir & (flag << 7)) {
			if (channel[i].play_count) {
				channel[i].play_count--;
			} else {
				/* disable DMA to this channel. */
				custom.dmacon = flag;
		
				/* disable interrupts to this channel. */
				custom.intena = (flag << 7);
			}
			 /* clear this channels interrupt. */
			custom.intreq = (flag << 7);
		}
	}

	end_int:
	/* enable audio interupts with dma still set. */
	audio_dma = custom.dmaconr;
	audio_dma &= (DMAF_AUD0|DMAF_AUD1|DMAF_AUD2|DMAF_AUD3);
	custom.intena = INTF_SETCLR| (audio_dma << 7);
}

void
play_sample (len, data, period, volume, channels, count)
	u_short len, *data, period, volume, channels;
	u_long count;
{
	u_short dmabits = channels & 0xf;
	u_short ch;

	custom.dmacon = dmabits;	/* turn off the correct channels */

	/* load the channels */
	for (ch = 0; ch < 4; ch++) {
		if (dmabits & (ch << ch)) {
			custom.aud[ch].len = len;
			custom.aud[ch].lc = data;
			custom.aud[ch].per = period;
			custom.aud[ch].vol = volume;
			channel[ch].play_count = count;
		}
	}
	/* turn on interrupts for channels */
	custom.intena = INTF_SETCLR | (dmabits << 7);
	 /* turn on the correct channels */
	custom.dmacon = DMAF_SETCLR | dmabits;
}

/*
 * Chipmem allocator.
 */

mem_list_t chip;

void
cc_init_chipmem ()
{
	int s = splhigh ();
	mem_node_t *mem;
	extern u_char *chipmem_end, *chipmem_start;

	chip.size = chipmem_end - (chipmem_start+NBPG);
	chip.memory = (u_char *)chipmem_steal (chip.size);

	chip.free_nodes = 1;
	chip.alloc_nodes = 0;
	chip.total = chip.size - sizeof (mem_node_t);
    
	mem = (mem_node_t *)chip.memory;
	mem->size = chip.total;

	dinit_list (&chip.node_list);
	dinit_list (&chip.free_list);
    
	dadd_head (&chip.node_list, &mem->node);
	dadd_head (&chip.free_list, &mem->free);
	splx (s);
}

static void *
allocate (m, size)
	mem_list_t *m;
	u_long size;
{
	int s = splhigh();		/* disable everything. */
	void *mem = NULL;
	
	if (size) {
		dll_node_t *n;

		if (size & ~(CM_BLOCKMASK)) 
			size = (size & CM_BLOCKMASK) + CM_BLOCKSIZE;
	
		/* walk list of available nodes. */
		for (n = chip.free_list.head; n->next; n = n->next) {
			mem_node_t *mn = MNODE_FROM_FREE (n);
			if (size == mn->size) {
				dremove (n); /* remove from avail list. */
				n->next = NULL;
				n->prev = NULL;
				m->free_nodes--;
				m->alloc_nodes++;
				m->total -= mn->size;
				mem = (void *)&mn[1];
				break;
			} else if (size < mn->size) {
				if ((mn->size - size) <= sizeof (mem_node_t)) {
					/* our allocation would not leave room */
					/* for a new node in between. */
					
					size = mn->size; /* increase size. */
					dremove (n); /* remove from avail list. */
					n->next = NULL;
					n->prev = NULL;
					m->free_nodes--;
					m->alloc_nodes++;
					m->total -= mn->size;
					mem = (void *)&mn[1];
					break;
				} else {
					/* split the node's memory. */
					
					mem_node_t *new = mn;
					new->size -= size + sizeof (mem_node_t);
					mn = (mem_node_t *)(MNODES_MEM(new) + new->size);
					mn->size = size; /* this node is now */
							 /* exactly size big. */
					n = &mn->free;

					/* add the new node to big list */
					dappend (&new->node, &mn->node);
					/* add the new node to free list */
					dappend (&new->free, &mn->free); 

					/* remove the old node from free list. */
					dremove (&mn->free); 
					n->next = NULL;
					n->prev = NULL;
		    
					m->alloc_nodes++;
					m->total -= (size + sizeof (mem_node_t));
					mem = (void *)&mn[1];
					break;
				}
			}
		}
	}
	splx (s);
	return (mem);
}

static void
deallocate (m, mem)
	mem_list_t *m;
	void *mem;
{
	int s = splhigh();		/* disable everything. */
	mem_node_t *mn = mem;		/* point to the memory. */
	mem_node_t *next, *prev;		
	int      added = 0;		/* flag */
	mn--;				/* now points to the node struct. */

	/* check ahead of us. */
	next = (mem_node_t *)mn->node.next;
	prev = (mem_node_t *)mn->node.prev;
	if (next->node.next && next->free.next) {
		/* if next is: a valid node and a free node. ==> merge */
		dinsert (&next->free, &mn->free); /* add onto free list */
		m->free_nodes++;
	
		dremove (&next->node);	/* remove next from main list. */
		dremove (&next->free);	/* remove next from free list. */
		m->free_nodes--;
		m->alloc_nodes--;
		m->total += mn->size + sizeof (mem_node_t);
		added = 1;
		mn->size += next->size + sizeof (mem_node_t);
	}
	if (prev->node.prev && prev->free.prev) {
		/* if prev is: a valid node and a free node. ==> merge */
	    
		if (mn->free.next) {	/* if we are on free list. */
			dremove (&mn->free); /* remove us from free list. */
			m->free_nodes--;
		}
		dremove (&mn->node);	/* remove us from main list. */
		m->alloc_nodes--;
		prev->size += mn->size + sizeof (mem_node_t);
		
		if (added) 
			m->total += sizeof (mem_node_t);
		else 
			m->total += mn->size + sizeof (mem_node_t);
	} else if (NULL == mn->free.next) {
		/* we still are not on free list and we need to be. */
		
		while (next->node.next && prev->node.prev) {
			if (next->free.next) {
				dinsert (&next->free, &mn->free);
				m->free_nodes++;
				break;
			}
			if (prev->free.prev) {
				dappend (&prev->free, &mn->free);
				m->free_nodes++;
				break;
			}
			prev = (mem_node_t *)prev->node.prev;
			next = (mem_node_t *)next->node.next;
		}
		if (NULL == mn->free.next) {
			if (NULL == next->node.next) 
				/* we are not on list so we can add */
				/* ourselves to the tail. (we walked to it.) */
				dadd_tail (&m->free_list, &mn->free);
			else 	
				dadd_head (&m->free_list, &mn->free);
			m->free_nodes++;
		}
		m->total += mn->size;	/* add our helpings to the pool. */
	}
	splx (s);
}

u_long
sizeof_chipmem (mem)
	void *mem;
{
	if (mem) {
		mem_node_t *mn = mem;
		mn--;
		return (mn->size);
	}
	return (0);
}

void *
alloc_chipmem (size)
	u_long size;
{
	return (allocate (&chip, size));
}

void
free_chipmem (mem)
	void *mem;
{
	if (mem)  
		deallocate (&chip, mem);
}

u_long
avail_chipmem (largest)
	int largest;
{
	u_long val = 0;
	
	if (largest) {
		int s = splhigh ();
		dll_node_t *n;

		for (n = chip.free_list.head; n->next; n = n->next) {
			mem_node_t *mn = MNODE_FROM_FREE (n);
			if (mn->size > val) 
				val = mn->size;
		}
		splx (s);
	} else 
		val = chip.total;
	return (val);
}	      

