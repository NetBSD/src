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
 *	$Id: cc.h,v 1.4 1994/03/25 16:30:04 chopps Exp $
 */

#if ! defined (_CC_H)
#define _CC_H

#include <sys/queue.h>
#include <amiga/amiga/cc_registers.h>

#if ! defined (HIADDR)
#define HIADDR(x) (u_short)((((unsigned long)(x))>>16)&0xffff)
#endif 
#if ! defined (LOADDR)
#define LOADDR(x) (u_short)(((unsigned long)(x))&0xffff)
#endif

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
	CIRCLEQ_ENTRY(mem_node) link; 	
	CIRCLEQ_ENTRY(mem_node) free_link;
	u_long size;		/* size of memory following node. */
};

#define CM_BLOCKSIZE 0x4
#define CM_BLOCKMASK (~(CM_BLOCKSIZE - 1))
#define MNODES_MEM(mn) ((u_char *)(&mn[1]))
#define PREP_DMA_MEM(mem) (void *)((caddr_t)mem - CHIPMEMADDR)
extern caddr_t CHIPMEMADDR;


/*
 * Prototypes.
 */

#if defined (__STDC__)
void custom_chips_init (void);
/* vertical blank server chain */
void cc_init_vbl (void);
void add_vbl_function (struct vbl_node *n, short priority, void *data);
void remove_vbl_function (struct vbl_node *n);
void turn_vbl_function_off (struct vbl_node *n);
void turn_vbl_function_on (struct vbl_node *n);
/* blitter */
void cc_init_blitter (void);
int is_blitter_busy (void);
void wait_blit (void);
void blitter_handler (void);
void do_blit (u_short size);
void set_blitter_control (u_short con0, u_short con1);
void set_blitter_mods (u_short a, u_short b, u_short c, u_short d);
void set_blitter_masks (u_short fm, u_short lm);
void set_blitter_data (u_short da, u_short db, u_short dc);
void set_blitter_pointers (void *a, void *b, void *c, void *d);
/* copper */
void install_copper_list (cop_t *l);
cop_t *find_copper_inst (cop_t *l, u_short inst);
void cc_init_copper (void);
void copper_handler (void);
/* audio */
void cc_init_audio (void);
void play_sample (u_short len, u_short *data, u_short period, u_short volume, u_short channels, u_long count);
void audio_handler (void);
/* chipmem */
void cc_init_chipmem (void);
void * alloc_chipmem (u_long size);
void free_chipmem (void *m);
u_long avail_chipmem (int largest);
u_long sizeof_chipmem (void *m);
#endif /* __STDC__ */

#endif /* _CC_H */

