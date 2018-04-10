/*	$NetBSD: cc.c,v 1.26.16.1 2018/04/10 11:18:02 martin Exp $	*/

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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cc.c,v 1.26.16.1 2018/04/10 11:18:02 martin Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/queue.h>

#include <amiga/amiga/custom.h>
#include <amiga/amiga/cc.h>
#include "audio.h"

vaddr_t CUSTOMADDR, CUSTOMbase;

#if defined (__GNUC__)
#define INLINE inline
#else
#define INLINE
#endif

/* init all the "custom chips" */
void
custom_chips_init(void)
{
	cc_init_chipmem();
	cc_init_vbl();
	cc_init_audio();
	cc_init_blitter();
	cc_init_copper();
}

/*
 * Vertical blank iterrupt sever chains.
 */
LIST_HEAD(vbllist, vbl_node) vbl_list;

void
turn_vbl_function_off(struct vbl_node *n)
{
	if (n->flags & VBLNF_OFF)
		return;

	n->flags |= VBLNF_TURNOFF;
	while ((n->flags & VBLNF_OFF) == 0)
		;
}

/* allow function to be called on next vbl interrupt. */
void
turn_vbl_function_on(struct vbl_node *n)
{
	n->flags &= (short) ~(VBLNF_OFF);
}

void
add_vbl_function(struct vbl_node *add, short priority, void *data)
{
	int s;
	struct vbl_node *n, *prev;

	s = spl3();
	prev = NULL;
	LIST_FOREACH(n, &vbl_list, link) {
		if (add->priority > n->priority) {
			/* insert add_node before. */
			if (prev == NULL) {
				LIST_INSERT_HEAD(&vbl_list, add, link);
			} else {
				LIST_INSERT_AFTER(prev, add, link);
			}
			add = NULL;
			break;
		}
		prev = n;
	}
	if (add != NULL) {
		if (prev == NULL) {
			LIST_INSERT_HEAD(&vbl_list, add, link);
		} else {
			LIST_INSERT_AFTER(prev, add, link);
		}
	}
	splx(s);
}

void
remove_vbl_function(struct vbl_node *n)
{
	int s;

	s = spl3();
	LIST_REMOVE(n, link);
	splx(s);
}

/* Level 3 hardware interrupt */
void
vbl_handler(void)
{
	struct vbl_node *n;

	/* handle all vbl functions */
	LIST_FOREACH(n, &vbl_list, link) {
		if (n->flags & VBLNF_TURNOFF) {
			n->flags |= VBLNF_OFF;
			n->flags &= ~(VBLNF_TURNOFF);
		} else {
			if (n != NULL)
				n->function(n->data);
		}
	}
	custom.intreq = INTF_VERTB;
}

void
cc_init_vbl(void)
{
	LIST_INIT(&vbl_list);
	/*
	 * enable vertical blank interrupts
	 */
	custom.intena = INTF_SETCLR | INTF_VERTB;
}


/*
 * Blitter stuff.
 */

void
cc_init_blitter(void)
{
}

/* test twice to cover blitter bugs if BLTDONE (BUSY) is set it is not done. */
int
is_blitter_busy(void)
{
	u_short bb;

	bb = (custom.dmaconr & DMAF_BLTDONE);
	if ((custom.dmaconr & DMAF_BLTDONE) || bb)
		return (1);
	return (0);
}

void
wait_blit(void)
{
	/*
	 * V40 state this covers all blitter bugs.
	 */
	while (is_blitter_busy())
		;
}

void
blitter_handler(void)
{
	custom.intreq = INTF_BLIT;
}


void
do_blit(u_short size)
{
	custom.bltsize = size;
}

void
set_blitter_control(u_short con0, u_short con1)
{
	custom.bltcon0 = con0;
	custom.bltcon1 = con1;
}

void
set_blitter_mods(u_short a, u_short b, u_short c, u_short d)
{
	custom.bltamod = a;
	custom.bltbmod = b;
	custom.bltcmod = c;
	custom.bltdmod = d;
}

void
set_blitter_masks(u_short fm, u_short lm)
{
	custom.bltafwm = fm;
	custom.bltalwm = lm;
}

void
set_blitter_data(u_short da, u_short db, u_short dc)
{
	custom.bltadat = da;
	custom.bltbdat = db;
	custom.bltcdat = dc;
}

void
set_blitter_pointers(void *a, void *b, void *c, void *d)
{
	custom.bltapt = a;
	custom.bltbpt = b;
	custom.bltcpt = c;
	custom.bltdpt = d;
}

/*
 * Copper Stuff.
 */


/*
 * Wait till end of frame. We should probably better use the
 * sleep/wakeup system newly introduced in the vbl manager
 */
void
wait_tof(void)
{
	/*
	 * wait until bottom of frame.
	 */
	while ((custom.vposr & 0x0007) == 0)
		;

	/*
	 * wait until until top of frame.
	 */
	while (custom.vposr & 0x0007)
		;

	if (custom.vposr & 0x8000)
		return;
	/*
	 * we are on short frame.
	 * wait for long frame bit set
	 */
	while ((custom.vposr & 0x8000) == 0)
		;
}

cop_t *
find_copper_inst(cop_t *l, u_short inst)
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
install_copper_list(cop_t *l)
{
	wait_tof();
	wait_tof();
	custom.cop1lc = l;
}


void
cc_init_copper(void)
{
}

/*
 * level 3 interrupt
 */
void
copper_handler(void)
{
	custom.intreq = INTF_COPER;
}

/*
 * Audio stuff.
 */


/* - channel[4] */
/* the data for each audio channel and what to do with it. */
struct audio_channel channel[4];

/* audio vbl node for vbl function  */
struct vbl_node audio_vbl_node;

void
cc_init_audio(void)
{
	int i;

	/*
	 * disable all audio interrupts
	 */
	custom.intena = INTF_AUD0|INTF_AUD1|INTF_AUD2|INTF_AUD3;

	/*
	 * initialize audio channels to off.
	 */
	for (i = 0; i < 4; i++) {
		channel[i].play_count = 0;
		channel[i].isaudio = 0;
		channel[i].handler = NULL;
	}
}


/*
 * Audio Interrupt Handler
 */
void
audio_handler(void)
{
	u_short audio_dma, flag, ir;
	int i;

	audio_dma = custom.dmaconr;

	/*
	 * only check channels who have DMA enabled.
	 */
	audio_dma &= (DMAF_AUD0|DMAF_AUD1|DMAF_AUD2|DMAF_AUD3);

	/*
	 * disable all audio interrupts with DMA set
	 */
	custom.intena = (audio_dma << INTB_AUD0) & AUCC_ALLINTF;

	/*
	 * if no audio DMA enabled then exit quick.
	 */
	if (!audio_dma) {
		/*
		 * clear all interrupts.
		 */
		custom.intreq = AUCC_ALLINTF;
		goto out;
	}
	for (i = 0; i < AUCC_MAXINT; i++) {
		flag = (1 << i);
		ir = custom.intreqr;
		/*
		 * is this channel's interrupt is set?
		 */
		if ((ir & (flag << INTB_AUD0)) == 0)
			continue;
#if NAUDIO>0
		custom.intreq=(flag<<INTB_AUD0);
		/* call audio handler with channel number */
		if (channel[i].isaudio==1)
			if (channel[i].handler)
				(*channel[i].handler)(i);
#endif

		if (channel[i].play_count)
			channel[i].play_count--;
		else {
			/*
			 * disable DMA to this channel and
			 * disable interrupts to this channel
			 */
			custom.dmacon = flag;
			custom.intena = (flag << INTB_AUD0);
			if (channel[i].isaudio==-1)
				channel[i].isaudio=0;
		}
		/*
		 * clear this channels interrupt.
		 */
		custom.intreq = (flag << INTB_AUD0);
	}

out:
	/*
	 * enable audio interrupts with DMA still set.
	 */
	audio_dma = custom.dmaconr;
	audio_dma &= (DMAF_AUD0|DMAF_AUD1|DMAF_AUD2|DMAF_AUD3);
	custom.intena = INTF_SETCLR | (audio_dma << INTB_AUD0);
}

void
play_sample(u_short len, u_short *data, u_short period, u_short volume, u_short channels, u_long count)
{
	u_short dmabits, ch;
	register int i;

	dmabits = channels & 0xf;

	/* check to see, whether all channels are free */
	for (i=0;i<4;i++) {
		if ((1<<i) & dmabits) {
			if (channel[i].isaudio)
				return; /* allocated */
			else
				channel[i].isaudio=-1; /* allocate */
		}
	}

	custom.dmacon = dmabits;	/* turn off the correct channels */

	/* load the channels */
	for (ch = 0; ch < 4; ch++) {
		if ((dmabits & (ch << ch)) == 0)
			continue;
		custom.aud[ch].len = len;
		custom.aud[ch].lc = data;
		custom.aud[ch].per = period;
		custom.aud[ch].vol = volume;
		channel[ch].play_count = count;
	}
	/*
	 * turn on interrupts and enable DMA for channels and
	 */
	custom.intena = INTF_SETCLR | (dmabits << INTB_AUD0);
	custom.dmacon = DMAF_SETCLR | DMAF_MASTER |dmabits;
}

/*
 * Chipmem allocator.
 */

static TAILQ_HEAD(chiplist, mem_node) chip_list;
static TAILQ_HEAD(freelist, mem_node) free_list;
static u_long   chip_total;		/* total free. */
static u_long   chip_size;		/* size of it all. */

void
cc_init_chipmem(void)
{
	int s = splhigh ();
	struct mem_node *mem;

	chip_size = chipmem_end - (chipmem_start + PAGE_SIZE);
	chip_total = chip_size - sizeof(*mem);

	mem = (struct mem_node *)chipmem_steal(chip_size);
	mem->size = chip_total;

	TAILQ_INIT(&chip_list);
	TAILQ_INIT(&free_list);

	TAILQ_INSERT_HEAD(&chip_list, mem, link);
	TAILQ_INSERT_HEAD(&free_list, mem, free_link);
	splx(s);
}

void *
alloc_chipmem(u_long size)
{
	int s;
	struct mem_node *mn, *new;

	if (size == 0)
		return NULL;

	s = splhigh();

	if (size & ~(CM_BLOCKMASK))
		size = (size & CM_BLOCKMASK) + CM_BLOCKSIZE;

	/*
	 * walk list of available nodes.
	 */
	TAILQ_FOREACH(mn, &free_list, free_link)
		if (size <= mn->size)
			break;

	if (mn == NULL) {
		splx(s);
		return NULL;
	}

	if ((mn->size - size) <= sizeof (*mn)) {
		/*
		 * our allocation would not leave room
		 * for a new node in between.
		 */
		TAILQ_REMOVE(&free_list, mn, free_link);
		mn->type = MNODE_USED;
		size = mn->size;	 /* increase size. (or same) */
		chip_total -= mn->size;
		splx(s);
		return ((void *)&mn[1]);
	}

	/*
	 * split the node's memory.
	 */
	new = mn;
	new->size -= size + sizeof(struct mem_node);
	mn = (struct mem_node *)(MNODES_MEM(new) + new->size);
	mn->size = size;

	/*
	 * add split node to node list
	 * and mark as not on free list
	 */
	TAILQ_INSERT_AFTER(&chip_list, new, mn, link);
	mn->type = MNODE_USED;

	chip_total -= size + sizeof(struct mem_node);
	splx(s);
	return ((void *)&mn[1]);
}

void
free_chipmem(void *mem)
{
	struct mem_node *mn, *next, *prev;
	int s;

	if (mem == NULL)
		return;

	s = splhigh();
	mn = (struct mem_node *)mem - 1;
	next = TAILQ_NEXT(mn, link);
	prev = TAILQ_PREV(mn, chiplist, link);

	/*
	 * check ahead of us.
	 */
	if (next->type == MNODE_FREE) {
		/*
		 * if next is: a valid node and a free node. ==> merge
		 */
		TAILQ_INSERT_BEFORE(next, mn, free_link);
		mn->type = MNODE_FREE;
		TAILQ_REMOVE(&chip_list, next, link);
		TAILQ_REMOVE(&free_list, next, free_link);
		chip_total += mn->size + sizeof(struct mem_node);
		mn->size += next->size + sizeof(struct mem_node);
	}
	if (prev->type == MNODE_FREE) {
		/*
		 * if prev is: a valid node and a free node. ==> merge
		 */
		if (mn->type != MNODE_FREE)
			chip_total += mn->size + sizeof(struct mem_node);
		else {
			/* already on free list */
			TAILQ_REMOVE(&free_list, mn, free_link);
			mn->type = MNODE_USED;
			chip_total += sizeof(struct mem_node);
		}
		TAILQ_REMOVE(&chip_list, mn, link);
		prev->size += mn->size + sizeof(struct mem_node);
	} else if (mn->type != MNODE_FREE) {
		/*
		 * we still are not on free list and we need to be.
		 * <-- | -->
		 */
		while (next != NULL && prev != NULL) {
			if (next->type == MNODE_FREE) {
				TAILQ_INSERT_BEFORE(next, mn, free_link);
				mn->type = MNODE_FREE;
				break;
			}
			if (prev->type == MNODE_FREE) {
				TAILQ_INSERT_AFTER(&free_list, prev, mn,
				    free_link);
				mn->type = MNODE_FREE;
				break;
			}
			prev = TAILQ_PREV(prev, chiplist, link);
			next = TAILQ_NEXT(next, link);
		}
		if (mn->type != MNODE_FREE) {
			if (next == NULL) {
				/*
				 * we are not on list so we can add
				 * ourselves to the tail. (we walked to it.)
				 */
				TAILQ_INSERT_TAIL(&free_list,mn,free_link);
			} else {
				TAILQ_INSERT_HEAD(&free_list,mn,free_link);
			}
			mn->type = MNODE_FREE;
		}
		chip_total += mn->size;	/* add our helpings to the pool. */
	}
	splx(s);
}

u_long
sizeof_chipmem(void *mem)
{
	struct mem_node *mn;

	if (mem == NULL)
		return (0);
	mn = mem;
	mn--;
	return (mn->size);
}

u_long
avail_chipmem(int largest)
{
	struct mem_node *mn;
	u_long val;
	int s;

	val = 0;
	if (largest == 0)
		val = chip_total;
	else {
		s = splhigh();
		TAILQ_FOREACH(mn, &free_list, free_link) {
			if (mn->size > val)
				val = mn->size;
		}
		splx(s);
	}
	return (val);
}
