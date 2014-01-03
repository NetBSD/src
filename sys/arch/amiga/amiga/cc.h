/*	$NetBSD: cc.h,v 1.18 2014/01/03 07:14:20 mlelstv Exp $	*/

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

#if ! defined (_CC_H)
#define _CC_H

#include <sys/queue.h>
#include <amiga/amiga/cc_registers.h>

#ifndef __powerpc__
#include "opt_lev6_defer.h"
#endif /* __powerpc__ */

#if ! defined (HIADDR)
#define HIADDR(x) (u_short)((((unsigned long)(x))>>16)&0xffff)
#endif
#if ! defined (LOADDR)
#define LOADDR(x) (u_short)(((unsigned long)(x))&0xffff)
#endif


/*
 * Audio stuff
 */
struct audio_channel {
	u_short play_count;
	short	isaudio;
	void  (*handler)(int);
};

#ifdef LEV6_DEFER
#define AUCC_MAXINT 3
#define AUCC_ALLINTF (INTF_AUD0|INTF_AUD1|INTF_AUD2)
#else
#define AUCC_MAXINT 4
#define AUCC_ALLINTF (INTF_AUD0|INTF_AUD1|INTF_AUD2|INTF_AUD3)
#endif
/*
 * Define this one unconditionally; we may use AUD3 as slave channel
 * with LEV6_DEFER
 */
#define AUCC_ALLDMAF (DMAF_AUD0|DMAF_AUD1|DMAF_AUD2|DMAF_AUD3)

/*
 * Vertical blank iterrupt sever chains.
 */

struct vbl_node {
	LIST_ENTRY(vbl_node) link;
	short  priority;			/* Private. */
	short  flags;				/* Private. */
	void  (*function)(register void *);	/* put function pointer here */
	void   *data;				/* functions data. */
};

enum vbl_node_bits {
    VBLNB_OFF,		  /* don't call me right now. */
    VBLNB_TURNOFF,	  /* turn function off. */
};

enum vlb_node_flags {
    VBLNF_OFF = 1 << VBLNB_OFF,
    VBLNF_TURNOFF = 1 << VBLNB_TURNOFF,
};

/*
 * Blitter stuff.
 */

#define BLT_SHIFT_MASK(shift) (0xf&shift)

#define MAKEBOOL(val) (val ? 1 : 0)

#define DMAADDR(lng) (u_long)(((0x7 & lng) << 16)|(lng & 0xffff))

#define MAKE_BLTCON0(shift_a, use_a, use_b, use_c, use_d, minterm) \
        ((0x0000) | (BLT_SHIFT_MASK(shift_a) << 12) | \
	 (use_a << 11) |  (use_b << 10) |  (use_c << 9) |  (use_d << 8) | \
	 (minterm))

#define MAKE_BLTCON1(shift_b, efe, ife, fc, desc)  \
        ((0x0000) | (BLT_SHIFT_MASK(shift_b) << 12) | (efe << 4) | \
	 (ife << 3) | (fc << 2) | (desc << 1))

/*
 * Copper stuff.
 */

typedef struct copper_list {
    union j {
	struct k {
	    u_short opcode;
	    u_short operand;
	} inst;
	u_long data;
    } cp;
} cop_t;

#define CI_MOVE(x)   (0x7ffe & x)
#define CI_WAIT(h,v) (((0x7f&v)<<8)|(0xfe&h)|(0x0001))
#define CI_SKIP(x)   (((0x7f&v)<<8)|(0xfe&h)|(0x0001))

#define CD_MOVE(x) (x)
#define CD_WAIT(x) (x & 0xfffe)
#define CD_SKIP(x) (x|0x0001)

#define CBUMP(c) (c++)

#define CMOVE(c,r,v) do { \
			    c->cp.data=((CI_MOVE(r)<<16)|(CD_MOVE(v))); \
		            CBUMP (c); \
		        } while(0)
#define CWAIT(c,h,v) do { \
			    c->cp.data=((CI_WAIT(h,v) << 16)|CD_WAIT(0xfffe)); \
		            CBUMP (c); \
		        } while(0)
#define CSKIP(c,h,v) do { \
			    c->cp.data=((CI_SKIP(h,v)<<16)|CD_SKIP(0xffff)); \
		            CBUMP (c); \
		        } while(0)
#define CEND(c) do { \
			    c->cp.data=0xfffffffe; \
		            CBUMP (c); \
		        } while(0)

/*
 * Chipmem allocator stuff.
 */

struct mem_node {
	TAILQ_ENTRY(mem_node) link; 
	TAILQ_ENTRY(mem_node) free_link;
	u_long size;		/* size of memory following node. */
	u_char type;		/* free, used */
};
#define MNODE_FREE 0
#define MNODE_USED 1

#define CM_BLOCKSIZE 0x4
#define CM_BLOCKMASK (~(CM_BLOCKSIZE - 1))
#define MNODES_MEM(mn) ((u_char *)(&mn[1]))
#define PREP_DMA_MEM(mem) (void *)((char*)(mem) - CHIPMEMADDR)

extern vaddr_t CHIPMEMADDR;
extern vaddr_t chipmem_start;
extern vaddr_t chipmem_end;
#define CHIPMEMBASE	(0x00000000)
#define CHIPMEMTOP	(0x00200000)
#define NCHIPMEMPG	btoc(CHIPMEMTOP - CHIPMEMBASE)

/*
 * Prototypes.
 */
void custom_chips_init(void);
/* vertical blank server chain */
void cc_init_vbl(void);
void add_vbl_function(struct vbl_node *, short, void *);
void remove_vbl_function(struct vbl_node *);
void turn_vbl_function_off(struct vbl_node *);
void turn_vbl_function_on(struct vbl_node *);
/* blitter */
void cc_init_blitter(void);
int is_blitter_busy(void);
void wait_blit(void);
void blitter_handler(void);
void do_blit(u_short);
void set_blitter_control(u_short, u_short);
void set_blitter_mods(u_short, u_short, u_short, u_short);
void set_blitter_masks(u_short, u_short);
void set_blitter_data(u_short, u_short, u_short);
void set_blitter_pointers(void *, void *, void *, void *);
/* copper */
void install_copper_list(cop_t *);
cop_t *find_copper_inst(cop_t *, u_short);
void cc_init_copper(void);
void copper_handler(void);
/* audio */
void cc_init_audio(void);
void play_sample(u_short, u_short *, u_short, u_short, u_short, u_long);
void audio_handler(void);
/* chipmem */
void cc_init_chipmem(void);
void *alloc_chipmem(u_long);
void free_chipmem(void *);
u_long avail_chipmem(int);
u_long sizeof_chipmem(void *);

void wait_tof(void);
void vbl_handler(void);
void *chipmem_steal(long);

#endif /* _CC_H */
