/*
 * Copyright (c) 1993 Christian E. Hopps
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Christian E. Hopps.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#if ! defined (_CC_COPPER_H)
#define _CC_COPPER_H

#include "cc.h"

typedef struct copper_list {
    union j {
	struct k {
	    u_word opcode;
	    u_word operand;
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

void install_copper_list (cop_t *l);
cop_t *find_copper_inst (cop_t *l, u_word inst);
void cc_init_copper (void);
void copper_handler (void);

#if defined (AMIGA_TEST)
void cc_deinit_copper (void);
#endif

#endif /* _CC_COPPER_H */

