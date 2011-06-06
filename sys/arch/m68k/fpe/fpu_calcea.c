/*	$NetBSD: fpu_calcea.c,v 1.22.2.1 2011/06/06 09:05:56 jruoho Exp $	*/

/*
 * Copyright (c) 1995 Gordon W. Ross
 * portion Copyright (c) 1995 Ken Nakata
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 4. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Gordon Ross
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

#include "opt_m68k_arch.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fpu_calcea.c,v 1.22.2.1 2011/06/06 09:05:56 jruoho Exp $");

#include <sys/param.h>
#include <sys/signal.h>
#include <sys/systm.h>
#include <machine/frame.h>
#include <m68k/m68k.h>

#include "fpu_emulate.h"

#ifdef DEBUG_FPE
#define DPRINTF(x)	printf x
#else
#define DPRINTF(x)	do {} while (/* CONSTCOND */ 0)
#endif

/*
 * Prototypes of static functions
 */
static int decode_ea6(struct frame *, struct instruction *,
		      struct insn_ea *, int);
static int fetch_immed(struct frame *, struct instruction *, int *);
static int fetch_disp(struct frame *, struct instruction *, int, int *);
static int calc_ea(struct insn_ea *, char *, char **);

/*
 * Helper routines for dealing with "effective address" values.
 */

/*
 * Decode an effective address into internal form.
 * Returns zero on success, else signal number.
 */
int
fpu_decode_ea(struct frame *frame, struct instruction *insn,
    struct insn_ea *ea, int modreg)
{
	int sig;

#ifdef DIAGNOSTIC
	if (insn->is_datasize < 0)
		panic("%s: called with uninitialized datasize", __func__);
#endif

	sig = 0;

	/* Set the most common value here. */
	ea->ea_regnum = 8 + (modreg & 7);

	if ((modreg & 060) == 0) {
		/* register direct */
		ea->ea_regnum = modreg & 0xf;
		ea->ea_flags = EA_DIRECT;
		DPRINTF(("%s: register direct reg=%d\n",
		    __func__, ea->ea_regnum));
	} else if ((modreg & 077) == 074) {
		/* immediate */
		ea->ea_flags = EA_IMMED;
		sig = fetch_immed(frame, insn, &ea->ea_immed[0]);
		DPRINTF(("%s: immediate size=%d\n",
		    __func__, insn->is_datasize));
	}
	/*
	 * rest of the address modes need to be separately
	 * handled for the LC040 and the others.
	 */
#if 0 /* XXX */
	else if (frame->f_format == 4 && frame->f_fmt4.f_fa) {
		/* LC040 */
		ea->ea_flags = EA_FRAME_EA;
		ea->ea_fea = frame->f_fmt4.f_fa;
		DPRINTF(("%s: 68LC040 - in-frame EA (%p) size %d\n",
		    __func__, (void *)ea->ea_fea, insn->is_datasize));
		if ((modreg & 070) == 030) {
			/* postincrement mode */
			ea->ea_flags |= EA_POSTINCR;
		} else if ((modreg & 070) == 040) {
			/* predecrement mode */
			ea->ea_flags |= EA_PREDECR;
#ifdef M68060
#if defined(M68020) || defined(M68030) || defined(M68040)
			if (cputype == CPU_68060)
#endif
				if (insn->is_datasize == 12)
					ea->ea_fea -= 8;
#endif
		}
	}
#endif /* XXX */
	else {
		/* 020/030 */
		switch (modreg & 070) {

		case 020:			/* (An) */
			ea->ea_flags = 0;
			DPRINTF(("%s: register indirect reg=%d\n",
			    __func__, ea->ea_regnum));
			break;

		case 030:			/* (An)+ */
			ea->ea_flags = EA_POSTINCR;
			DPRINTF(("%s: reg indirect postincrement reg=%d\n",
			    __func__, ea->ea_regnum));
			break;

		case 040:			/* -(An) */
			ea->ea_flags = EA_PREDECR;
			DPRINTF(("%s: reg indirect predecrement reg=%d\n",
			    __func__, ea->ea_regnum));
			break;

		case 050:			/* (d16,An) */
			ea->ea_flags = EA_OFFSET;
			sig = fetch_disp(frame, insn, 1, &ea->ea_offset);
			DPRINTF(("%s: reg indirect with displacement reg=%d\n",
			    __func__, ea->ea_regnum));
		break;

		case 060:			/* (d8,An,Xn) */
			ea->ea_flags = EA_INDEXED;
			sig = decode_ea6(frame, insn, ea, modreg);
			break;

		case 070:			/* misc. */
			ea->ea_regnum = (modreg & 7);
			switch (modreg & 7) {

			case 0:			/* (xxxx).W */
				ea->ea_flags = EA_ABS;
				sig = fetch_disp(frame, insn, 1,
				    &ea->ea_absaddr);
				DPRINTF(("%s: absolute address (word)\n",
				    __func__));
				break;

			case 1:			/* (xxxxxxxx).L */
				ea->ea_flags = EA_ABS;
				sig = fetch_disp(frame, insn, 2,
				    &ea->ea_absaddr);
				DPRINTF(("%s: absolute address (long)\n",
				    __func__));
				break;

			case 2:			/* (d16,PC) */
				ea->ea_flags = EA_PC_REL | EA_OFFSET;
				sig = fetch_disp(frame, insn, 1,
				    &ea->ea_absaddr);
				DPRINTF(("%s: pc relative word displacement\n",
				    __func__));
				break;

			case 3:			/* (d8,PC,Xn) */
				ea->ea_flags = EA_PC_REL | EA_INDEXED;
				sig = decode_ea6(frame, insn, ea, modreg);
				break;

			case 4:			/* #data */
				/* it should have been taken care of earlier */
			default:
				DPRINTF(("%s: invalid addr mode (7,%d)\n",
				    __func__, modreg & 7));
				return SIGILL;
			}
			break;
		}
	}
	ea->ea_moffs = 0;

	return sig;
}

/*
 * Decode Mode=6 address modes
 */
static int
decode_ea6(struct frame *frame, struct instruction *insn, struct insn_ea *ea,
    int modreg)
{
	int extword, idx;
	int basedisp, outerdisp;
	int bd_size, od_size;
	int sig;

	extword = fusword((void *)(insn->is_pc + insn->is_advance));
	if (extword < 0) {
		return SIGSEGV;
	}
	insn->is_advance += 2;

	/* get register index */
	ea->ea_idxreg = (extword >> 12) & 0xf;
	idx = frame->f_regs[ea->ea_idxreg];
	if ((extword & 0x0800) == 0) {
		/* if word sized index, sign-extend */
		idx &= 0xffff;
		if (idx & 0x8000) {
			idx |= 0xffff0000;
		}
	}
	/* scale register index */
	idx <<= ((extword >> 9) & 3);

	if ((extword & 0x100) == 0) {
		/* brief extension word - sign-extend the displacement */
		basedisp = (extword & 0xff);
		if (basedisp & 0x80) {
			basedisp |= 0xffffff00;
		}

		ea->ea_basedisp = idx + basedisp;
		ea->ea_outerdisp = 0;
		DPRINTF(("%s: brief ext word idxreg=%d, basedisp=%08x\n",
		    __func__, ea->ea_idxreg, ea->ea_basedisp));
	} else {
		/* full extension word */
		if (extword & 0x80) {
			ea->ea_flags |= EA_BASE_SUPPRSS;
		}
		bd_size = ((extword >> 4) & 3) - 1;
		od_size = (extword & 3) - 1;
		sig = fetch_disp(frame, insn, bd_size, &basedisp);
		if (sig)
			return sig;
		if (od_size >= 0)
			ea->ea_flags |= EA_MEM_INDIR;
		sig = fetch_disp(frame, insn, od_size, &outerdisp);
		if (sig)
			return sig;

		switch (extword & 0x44) {
		case 0:			/* preindexed */
			ea->ea_basedisp = basedisp + idx;
			ea->ea_outerdisp = outerdisp;
			break;
		case 4:			/* postindexed */
			ea->ea_basedisp = basedisp;
			ea->ea_outerdisp = outerdisp + idx;
			break;
		case 0x40:		/* no index */
			ea->ea_basedisp = basedisp;
			ea->ea_outerdisp = outerdisp;
			break;
		default:
			DPRINTF(("%s: invalid indirect mode: ext word %04x\n",
			    __func__, extword));
			return SIGILL;
			break;
		}
		DPRINTF(("%s: full ext idxreg=%d, basedisp=%x, outerdisp=%x\n",
		    __func__,
		    ea->ea_idxreg, ea->ea_basedisp, ea->ea_outerdisp));
	}
	DPRINTF(("%s: regnum=%d, flags=%x\n",
	    __func__, ea->ea_regnum, ea->ea_flags));
	return 0;
}

/*
 * Load a value from an effective address.
 * Returns zero on success, else signal number.
 */
int
fpu_load_ea(struct frame *frame, struct instruction *insn, struct insn_ea *ea,
    char *dst)
{
	int *reg;
	char *src;
	int len, step;
	int sig;

#ifdef DIAGNOSTIC
	if (ea->ea_regnum & ~0xF)
		panic("%s: bad regnum", __func__);
#endif

	DPRINTF(("%s: frame at %p\n", __func__, frame));
	/* dst is always int or larger. */
	len = insn->is_datasize;
	if (len < 4)
		dst += (4 - len);
	step = (len == 1 && ea->ea_regnum == 15 /* sp */) ? 2 : len;

#if 0
	if (ea->ea_flags & EA_FRAME_EA) {
		/* Using LC040 frame EA */
#ifdef DEBUG_FPE
		if (ea->ea_flags & (EA_PREDECR|EA_POSTINCR)) {
			printf("%s: frame ea %08x w/r%d\n",
			    __func__, ea->ea_fea, ea->ea_regnum);
		} else {
			printf("%s: frame ea %08x\n", __func__, ea->ea_fea);
		}
#endif
		src = (char *)ea->ea_fea;
		copyin(src + ea->ea_moffs, dst, len);
		if (ea->ea_flags & EA_PREDECR) {
			frame->f_regs[ea->ea_regnum] = ea->ea_fea;
			ea->ea_fea -= step;
			ea->ea_moffs = 0;
		} else if (ea->ea_flags & EA_POSTINCR) {
			ea->ea_fea += step;
			frame->f_regs[ea->ea_regnum] = ea->ea_fea;
			ea->ea_moffs = 0;
		} else {
			ea->ea_moffs += step;
		}
		/* That's it, folks */
	} else
#endif
	if (ea->ea_flags & EA_DIRECT) {
		if (len > 4) {
			DPRINTF(("%s: operand doesn't fit CPU reg\n",
			    __func__));
			return SIGILL;
		}
		if (ea->ea_moffs > 0) {
			DPRINTF(("%s: more than one move from CPU reg\n",
			    __func__));
			return SIGILL;
		}
		src = (char *)&frame->f_regs[ea->ea_regnum];
		/* The source is an int. */
		if (len < 4) {
			src += (4 - len);
			DPRINTF(("%s: short/byte opr - addr adjusted\n",
			    __func__));
		}
		DPRINTF(("%s: src %p\n", __func__, src));
		memcpy(dst, src, len);
	} else if (ea->ea_flags & EA_IMMED) {
		DPRINTF(("%s: immed %08x%08x%08x size %d\n", __func__,
		    ea->ea_immed[0], ea->ea_immed[1], ea->ea_immed[2], len));
		src = (char *)&ea->ea_immed[0];
		if (len < 4) {
			src += (4 - len);
			DPRINTF(("%s: short/byte immed opr - "
			    "addr adjusted\n", __func__));
		}
		memcpy(dst, src, len);
	} else if (ea->ea_flags & EA_ABS) {
		DPRINTF(("%s: abs addr %08x\n", __func__, ea->ea_absaddr));
		src = (char *)ea->ea_absaddr;
		copyin(src, dst, len);
	} else /* register indirect */ { 
		if (ea->ea_flags & EA_PC_REL) {
			DPRINTF(("%s: using PC\n", __func__));
			reg = NULL;
			/*
			 * Grab the register contents. 4 is offset to the first
			 * extension word from the opcode
			 */
			src = (char *)insn->is_pc + 4;
			DPRINTF(("%s: pc relative pc+4 = %p\n", __func__, src));
		} else /* not PC relative */ {
			DPRINTF(("%s: using register %c%d\n",
			    __func__,
			    (ea->ea_regnum >= 8) ? 'a' : 'd',
			    ea->ea_regnum & 7));
			/* point to the register */
			reg = &frame->f_regs[ea->ea_regnum];

			if (ea->ea_flags & EA_PREDECR) {
				DPRINTF(("%s: predecr mode - "
				    "reg decremented\n", __func__));
				*reg -= step;
				ea->ea_moffs = 0;
			}

			/* Grab the register contents. */
			src = (char *)*reg;
			DPRINTF(("%s: reg indirect reg = %p\n", __func__, src));
		}

		sig = calc_ea(ea, src, &src);
		if (sig)
			return sig;

		copyin(src + ea->ea_moffs, dst, len);

		/* do post-increment */
		if (ea->ea_flags & EA_POSTINCR) {
			if (ea->ea_flags & EA_PC_REL) {
				DPRINTF(("%s: tried to postincrement PC\n",
				    __func__));
				return SIGILL;
			}
			*reg += step;
			ea->ea_moffs = 0;
			DPRINTF(("%s: postinc mode - reg incremented\n",
			    __func__));
		} else {
			ea->ea_moffs += len;
		}
	}

	return 0;
}

/*
 * Store a value at the effective address.
 * Returns zero on success, else signal number.
 */
int
fpu_store_ea(struct frame *frame, struct instruction *insn, struct insn_ea *ea,
    char *src)
{
	int *reg;
	char *dst;
	int len, step;
	int sig;

#ifdef DIAGNOSTIC
	if (ea->ea_regnum & ~0xf)
		panic("%s: bad regnum", __func__);
#endif

	if (ea->ea_flags & (EA_IMMED|EA_PC_REL)) {
		/* not alterable address mode */
		DPRINTF(("%s: not alterable address mode\n", __func__));
		return SIGILL;
	}

	/* src is always int or larger. */
	len = insn->is_datasize;
	if (len < 4)
		src += (4 - len);
	step = (len == 1 && ea->ea_regnum == 15 /* sp */) ? 2 : len;

	if (ea->ea_flags & EA_FRAME_EA) {
		/* Using LC040 frame EA */
#ifdef DEBUG_FPE
		if (ea->ea_flags & (EA_PREDECR|EA_POSTINCR)) {
			printf("%s: frame ea %08x w/r%d\n",
			    __func__, ea->ea_fea, ea->ea_regnum);
		} else {
			printf("%s: frame ea %08x\n", __func__, ea->ea_fea);
		}
#endif
		dst = (char *)ea->ea_fea;
		copyout(src, dst + ea->ea_moffs, len);
		if (ea->ea_flags & EA_PREDECR) {
			frame->f_regs[ea->ea_regnum] = ea->ea_fea;
			ea->ea_fea -= step;
			ea->ea_moffs = 0;
		} else if (ea->ea_flags & EA_POSTINCR) {
			ea->ea_fea += step;
			frame->f_regs[ea->ea_regnum] = ea->ea_fea;
			ea->ea_moffs = 0;
		} else {
			ea->ea_moffs += step;
		}
		/* That's it, folks */
	} else if (ea->ea_flags & EA_ABS) {
		DPRINTF(("%s: abs addr %08x\n", __func__, ea->ea_absaddr));
		dst = (char *)ea->ea_absaddr;
		copyout(src, dst + ea->ea_moffs, len);
		ea->ea_moffs += len;
	} else if (ea->ea_flags & EA_DIRECT) {
		if (len > 4) {
			DPRINTF(("%s: operand doesn't fit CPU reg\n",
			    __func__));
			return SIGILL;
		}
		if (ea->ea_moffs > 0) {
			DPRINTF(("%s: more than one move to CPU reg\n",
			    __func__));
			return SIGILL;
		}
		dst = (char *)&frame->f_regs[ea->ea_regnum];
		/* The destination is an int. */
		if (len < 4) {
			dst += (4 - len);
			DPRINTF(("%s: short/byte opr - dst addr adjusted\n",
			    __func__));
		}
		DPRINTF(("%s: dst %p\n", __func__, dst));
		memcpy(dst, src, len);
	} else /* One of MANY indirect forms... */ {
		DPRINTF(("%s: using register %c%d\n", __func__,
		    (ea->ea_regnum >= 8) ? 'a' : 'd', ea->ea_regnum & 7));
		/* point to the register */
		reg = &(frame->f_regs[ea->ea_regnum]);

		/* do pre-decrement */
		if (ea->ea_flags & EA_PREDECR) {
			DPRINTF(("%s: predecr mode - reg decremented\n",
			    __func__));
			*reg -= step;
			ea->ea_moffs = 0;
		}

		/* calculate the effective address */
		sig = calc_ea(ea, (char *)*reg, &dst);
		if (sig)
			return sig;

		DPRINTF(("%s: dst addr=%p+%d\n", __func__, dst, ea->ea_moffs));
		copyout(src, dst + ea->ea_moffs, len);

		/* do post-increment */
		if (ea->ea_flags & EA_POSTINCR) {
			*reg += step;
			ea->ea_moffs = 0;
			DPRINTF(("%s: postinc mode - reg incremented\n",
			    __func__));
		} else {
			ea->ea_moffs += len;
		}
	}

	return 0;
}

/*
 * fetch_immed: fetch immediate operand
 */
static int
fetch_immed(struct frame *frame, struct instruction *insn, int *dst)
{
	int data, ext_bytes;

	ext_bytes = insn->is_datasize;

	if (0 < ext_bytes) {
		data = fusword((void *)(insn->is_pc + insn->is_advance));
		if (data < 0)
			return SIGSEGV;
		if (ext_bytes == 1) {
			/* sign-extend byte to long */
			data &= 0xff;
			if (data & 0x80)
				data |= 0xffffff00;
		} else if (ext_bytes == 2) {
			/* sign-extend word to long */
			data &= 0xffff;
			if (data & 0x8000)
			data |= 0xffff0000;
		}
		insn->is_advance += 2;
		dst[0] = data;
	}
	if (2 < ext_bytes) {
		data = fusword((void *)(insn->is_pc + insn->is_advance));
		if (data < 0)
			return SIGSEGV;
		insn->is_advance += 2;
		dst[0] <<= 16;
		dst[0] |= data;
	}
	if (4 < ext_bytes) {
		data = fusword((void *)(insn->is_pc + insn->is_advance));
		if (data < 0)
			return SIGSEGV;
		dst[1] = data << 16;
		data = fusword((void *)(insn->is_pc + insn->is_advance + 2));
		if (data < 0)
			return SIGSEGV;
		insn->is_advance += 4;
		dst[1] |= data;
	}
	if (8 < ext_bytes) {
		data = fusword((void *)(insn->is_pc + insn->is_advance));
		if (data < 0)
			return SIGSEGV;
		dst[2] = data << 16;
		data = fusword((void *)(insn->is_pc + insn->is_advance + 2));
		if (data < 0)
			return SIGSEGV;
		insn->is_advance += 4;
		dst[2] |= data;
	}

	return 0;
}

/*
 * fetch_disp: fetch displacement in full extension words
 */
static int
fetch_disp(struct frame *frame, struct instruction *insn, int size, int *res)
{
	int disp, word;

	if (size == 1) {
		word = fusword((void *)(insn->is_pc + insn->is_advance));
		if (word < 0)
			return SIGSEGV;
		disp = word & 0xffff;
		if (disp & 0x8000) {
			/* sign-extend */
			disp |= 0xffff0000;
		}
		insn->is_advance += 2;
	} else if (size == 2) {
		word = fusword((void *)(insn->is_pc + insn->is_advance));
		if (word < 0)
			return SIGSEGV;
		disp = word << 16;
		word = fusword((void *)(insn->is_pc + insn->is_advance + 2));
		if (word < 0)
			return SIGSEGV;
		disp |= (word & 0xffff);
		insn->is_advance += 4;
	} else {
		disp = 0;
	}
	*res = disp;
	return 0;
}

/*
 * Calculates an effective address for all address modes except for
 * register direct, absolute, and immediate modes.  However, it does
 * not take care of predecrement/postincrement of register content.
 * Returns a signal value (0 == no error).
 */
static int
calc_ea(struct insn_ea *ea, char *ptr, char **eaddr)
	/* ptr:		 base address (usually a register content) */
	/* eaddr:	 pointer to result pointer */
{
	int data, word;

	DPRINTF(("%s: reg indirect (reg) = %p\n", __func__, ptr));

	if (ea->ea_flags & EA_OFFSET) {
		/* apply the signed offset */
		DPRINTF(("%s: offset %d\n", __func__, ea->ea_offset));
		ptr += ea->ea_offset;
	} else if (ea->ea_flags & EA_INDEXED) {
		DPRINTF(("%s: indexed mode\n", __func__));

		if (ea->ea_flags & EA_BASE_SUPPRSS) {
			/* base register is suppressed */
			ptr = (char *)ea->ea_basedisp;
		} else {
			ptr += ea->ea_basedisp;
		}

		if (ea->ea_flags & EA_MEM_INDIR) {
			DPRINTF(("%s: mem indir mode: basedisp=%08x, "
			    "outerdisp=%08x\n",
			    __func__, ea->ea_basedisp, ea->ea_outerdisp));
			DPRINTF(("%s: addr fetched from %p\n", __func__, ptr));
			/* memory indirect modes */
			word = fusword(ptr);
			if (word < 0)
				return SIGSEGV;
			word <<= 16;
			data = fusword(ptr + 2);
			if (data < 0)
				return SIGSEGV;
			word |= data;
			DPRINTF(("%s: fetched ptr 0x%08x\n", __func__, word));
			ptr = (char *)word + ea->ea_outerdisp;
		}
	}

	*eaddr = ptr;

	return 0;
}
