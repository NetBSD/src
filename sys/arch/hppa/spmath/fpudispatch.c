/*	$NetBSD: fpudispatch.c,v 1.1.2.2 2002/06/23 17:37:14 jdolecek Exp $	 */

/*
 *  (c) Copyright 1991 HEWLETT-PACKARD COMPANY
 *
 *  To anyone who acknowledges that this file is provided "AS IS"
 *  without any express or implied warranty:
 *      permission to use, copy, modify, and distribute this file
 *  for any purpose is hereby granted without fee, provided that
 *  the above copyright notice and this notice appears in all
 *  copies, and that the name of Hewlett-Packard Company not be
 *  used in advertising or publicity pertaining to distribution
 *  of the software without specific, written prior permission.
 *  Hewlett-Packard Company makes no representations about the
 *  suitability of this software for any purpose.
 */

/* Source: /n/schirf/u/baford/CVS/mach4-parisc/kernel/parisc/fpudispatch.c,v 
 * Revision: 1.4 	Author: mike 
 * State: Exp    	Locker:  
 * Date: 1994/07/21 17:36:35 
 */

#include <sys/types.h>
#include <sys/systm.h>

#include "../spmath/float.h"
/*
 * XXX fredette - hack to glue the bulk of 
 * the spmath library to this dispatcher.
 */
#define	dbl_integer		unsigned
#define	sgl_floating_point	unsigned
#define	dbl_floating_point	unsigned
#include "../spmath/sgl_float.h"
#include "../spmath/dbl_float.h"
#include "../spmath/cnv_float.h"
#include "../spmath/md.h"
#include "../spmath/fpudispatch.h"

/*
 * version of EMULATION software for COPR,0,0 instruction
 */
#define EMULATION_VERSION 3
#define COPR_INST 0x30000000

/*
 * definition of extru macro.  If pos and len are constants, the compiler
 * will generate an extru instruction when optimized
 */
#define extru(r,pos,len)	(((r) >> (31-(pos))) & (( 1 << (len)) - 1))
/* definitions of bit field locations in the instruction */
#define fpmajorpos 5
#define fpr1pos	10
#define fpr2pos 15
#define fptpos	31
#define fpsubpos 18
#define fpclass1subpos 16
#define fpclasspos 22
#define fpfmtpos 20
#define fpdfpos 18
/*
 * the following are the extra bits for the 0E major op
 */
#define fpxr1pos 24
#define fpxr2pos 19
#define fpxtpos 25
#define fpxpos 23
#define fp0efmtpos 20
/*
 * the following are for the multi-ops
 */
#define fprm1pos 10
#define fprm2pos 15
#define fptmpos 31
#define fprapos 25
#define fptapos 20
#define fpmultifmt 26

/*
 * offset to constant zero in the FP emulation registers
 */
#define fpzeroreg (32*sizeof(double)/sizeof(unsigned))

/*
 * extract the major opcode from the instruction
 */
#define get_major(op) extru(op,fpmajorpos,6)
/*
 * extract the two bit class field from the FP instruction. The class is at bit
 * positions 21-22
 */
#define get_class(op) extru(op,fpclasspos,2)
/*
 * extract the 3 bit subop field.  For all but class 1 instructions, it is
 * located at bit positions 16-18
 */
#define get_subop(op) extru(op,fpsubpos,3)
/*
 * extract the 2 bit subop field from class 1 instructions.  It is located
 * at bit positions 15-16
 */
#define get_subop1(op) extru(op,fpclass1subpos,2)

/* definitions of unimplemented exceptions */
#define MAJOR_0C_EXCP	UNIMPLEMENTEDEXCEPTION
#define MAJOR_0E_EXCP	UNIMPLEMENTEDEXCEPTION
#define MAJOR_06_EXCP	UNIMPLEMENTEDEXCEPTION
#define MAJOR_26_EXCP	UNIMPLEMENTEDEXCEPTION
#define PA83_UNIMP_EXCP	UNIMPLEMENTEDEXCEPTION

int
decode_0c(ir,class,subop,fpregs)
unsigned ir,class,subop;
unsigned fpregs[];
{
	unsigned r1,r2,t;	/* operand register offsets */ 
	unsigned fmt;		/* also sf for class 1 conversions */
	unsigned  df;		/* for class 1 conversions */
	unsigned *status;

	if (ir == COPR_INST) {
		fpregs[0] = EMULATION_VERSION << 11;
		return(NOEXCEPTION);
	}
	status = &fpregs[0];	/* fp status register */
	r1 = extru(ir,fpr1pos,5) * sizeof(double)/sizeof(unsigned);
	if (r1 == 0)		/* map fr0 source to constant zero */
		r1 = fpzeroreg;
	t = extru(ir,fptpos,5) * sizeof(double)/sizeof(unsigned);
	if (t == 0 && class != 2)	/* don't allow fr0 as a dest */
		return(MAJOR_0C_EXCP);
	fmt = extru(ir,fpfmtpos,2);	/* get fmt completer */

	switch (class) {
	    case 0:
		switch (subop) {
			case 0:	/* COPR 0,0 emulated above*/
			case 1:
			case 6:
			case 7:
				return(MAJOR_0C_EXCP);
			case 2:	/* FCPY */
				switch (fmt) {
				    case 2: /* illegal */
					return(MAJOR_0C_EXCP);
				    case 3: /* quad */
					fpregs[t+3] = fpregs[r1+3];
					fpregs[t+2] = fpregs[r1+2];
				    case 1: /* double */
					fpregs[t+1] = fpregs[r1+1];
				    case 0: /* single */
					fpregs[t] = fpregs[r1];
					return(NOEXCEPTION);
				}
			case 3: /* FABS */
				switch (fmt) {
				    case 2: /* illegal */
					return(MAJOR_0C_EXCP);
				    case 3: /* quad */
					fpregs[t+3] = fpregs[r1+3];
					fpregs[t+2] = fpregs[r1+2];
				    case 1: /* double */
					fpregs[t+1] = fpregs[r1+1];
				    case 0: /* single */
					/* copy and clear sign bit */
					fpregs[t] = fpregs[r1] & 0x7fffffff;
					return(NOEXCEPTION);
				}
			case 4: /* FSQRT */
				switch (fmt) {
				    case 0:
					return(sgl_fsqrt(&fpregs[r1],
						&fpregs[t],status));
				    case 1:
					return(dbl_fsqrt(&fpregs[r1],
						&fpregs[t],status));
				    case 2:
				    case 3: /* quad not implemented */
					return(MAJOR_0C_EXCP);
				}
			case 5: /* FRND */
				switch (fmt) {
				    case 0:
					return(sgl_frnd(&fpregs[r1],
						&fpregs[t],status));
				    case 1:
					return(dbl_frnd(&fpregs[r1],
						&fpregs[t],status));
				    case 2:
				    case 3: /* quad not implemented */
					return(MAJOR_0C_EXCP);
				}
		} /* end of switch (subop) */

	case 1: /* class 1 */
		df = extru(ir,fpdfpos,2); /* get dest format */
		if ((df & 2) || (fmt & 2)) {
			/*
			 * fmt's 2 and 3 are illegal of not implemented
			 * quad conversions
			 */
			return(MAJOR_0C_EXCP);
		}
		/*
		 * encode source and dest formats into 2 bits.
		 * high bit is source, low bit is dest.
		 * bit = 1 --> double precision
		 */
		fmt = (fmt << 1) | df;
		switch (subop) {
			case 0: /* FCNVFF */
				switch(fmt) {
				    case 0: /* sgl/sgl */
					return(MAJOR_0C_EXCP);
				    case 1: /* sgl/dbl */
					return(sgl_to_dbl_fcnvff(&fpregs[r1],
						&fpregs[t],status));
				    case 2: /* dbl/sgl */
					return(dbl_to_sgl_fcnvff(&fpregs[r1],
						&fpregs[t],status));
				    case 3: /* dbl/dbl */
					return(MAJOR_0C_EXCP);
				}
			case 1: /* FCNVXF */
				switch(fmt) {
				    case 0: /* sgl/sgl */
					return(sgl_to_sgl_fcnvxf(&fpregs[r1],
						&fpregs[t],status));
				    case 1: /* sgl/dbl */
					return(sgl_to_dbl_fcnvxf(&fpregs[r1],
						&fpregs[t],status));
				    case 2: /* dbl/sgl */
					return(dbl_to_sgl_fcnvxf(&fpregs[r1],
						&fpregs[t],status));
				    case 3: /* dbl/dbl */
					return(dbl_to_dbl_fcnvxf(&fpregs[r1],
						&fpregs[t],status));
				}
			case 2: /* FCNVFX */
				switch(fmt) {
				    case 0: /* sgl/sgl */
					return(sgl_to_sgl_fcnvfx(&fpregs[r1],
						&fpregs[t],status));
				    case 1: /* sgl/dbl */
					return(sgl_to_dbl_fcnvfx(&fpregs[r1],
						&fpregs[t],status));
				    case 2: /* dbl/sgl */
					return(dbl_to_sgl_fcnvfx(&fpregs[r1],
						&fpregs[t],status));
				    case 3: /* dbl/dbl */
					return(dbl_to_dbl_fcnvfx(&fpregs[r1],
						&fpregs[t],status));
				}
			case 3: /* FCNVFXT */
				switch(fmt) {
				    case 0: /* sgl/sgl */
					return(sgl_to_sgl_fcnvfxt(&fpregs[r1],
						&fpregs[t],status));
				    case 1: /* sgl/dbl */
					return(sgl_to_dbl_fcnvfxt(&fpregs[r1],
						&fpregs[t],status));
				    case 2: /* dbl/sgl */
					return(dbl_to_sgl_fcnvfxt(&fpregs[r1],
						&fpregs[t],status));
				    case 3: /* dbl/dbl */
					return(dbl_to_dbl_fcnvfxt(&fpregs[r1],
						&fpregs[t],status));
				}
		} /* end of switch subop */

	case 2: /* class 2 */
		r2 = extru(ir, fpr2pos, 5) * sizeof(double)/sizeof(unsigned);
		if (r2 == 0)
			r2 = fpzeroreg;
		switch (subop) {
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
				return(MAJOR_0C_EXCP);
			case 0: /* FCMP */
				switch (fmt) {
				    case 0:
					return(sgl_fcmp(&fpregs[r1],&fpregs[r2],
						extru(ir,fptpos,5),status));
				    case 1:
					return(dbl_fcmp(&fpregs[r1],&fpregs[r2],
						extru(ir,fptpos,5),status));
				    case 2: /* illegal */
				    case 3: /* quad not implemented */
					return(MAJOR_0C_EXCP);
				}
			case 1: /* FTEST */
				switch (fmt) {
				    case 0:
					/*
					 * arg0 is not used
					 * second param is the t field used for
					 * ftest,acc and ftest,rej
					 */
					/* XXX fredette - broken */
#if 0
					return(ftest(0,extru(ir,fptpos,5),
						&fpregs[0]));
#else
					panic("ftest");
#endif
				    case 1:
				    case 2:
				    case 3:
					return(MAJOR_0C_EXCP);
				}
		} /* end if switch for class 2*/
	case 3: /* class 3 */
		r2 = extru(ir,fpr2pos,5) * sizeof(double)/sizeof(unsigned);
		if (r2 == 0)
			r2 = fpzeroreg;
		switch (subop) {
			case 5:
			case 6:
			case 7:
				return(MAJOR_0C_EXCP);
			
			case 0: /* FADD */
				switch (fmt) {
				    case 0:
					return(sgl_fadd(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 1:
					return(dbl_fadd(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 2: /* illegal */
				    case 3: /* quad not implemented */
					return(MAJOR_0C_EXCP);
				}
			case 1: /* FSUB */
				switch (fmt) {
				    case 0:
					return(sgl_fsub(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 1:
					return(dbl_fsub(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 2: /* illegal */
				    case 3: /* quad not implemented */
					return(MAJOR_0C_EXCP);
				}
			case 2: /* FMPY */
				switch (fmt) {
				    case 0:
					return(sgl_fmpy(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 1:
					return(dbl_fmpy(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 2: /* illegal */
				    case 3: /* quad not implemented */
					return(MAJOR_0C_EXCP);
				}
			case 3: /* FDIV */
				switch (fmt) {
				    case 0:
					return(sgl_fdiv(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 1:
					return(dbl_fdiv(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 2: /* illegal */
				    case 3: /* quad not implemented */
					return(MAJOR_0C_EXCP);
				}
			case 4: /* FREM */
				switch (fmt) {
				    case 0:
					return(sgl_frem(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 1:
					return(dbl_frem(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 2: /* illegal */
				    case 3: /* quad not implemented */
					return(MAJOR_0C_EXCP);
				}
		} /* end of class 3 switch */
	} /* end of switch(class) */
	panic("decode_0c");
}

int
decode_0e(ir,class,subop,fpregs)
unsigned ir,class,subop;
unsigned fpregs[];
{
	unsigned r1,r2,t;	/* operand register offsets */
	unsigned fmt;		/* also sf for class 1 conversions */
	unsigned df;		/* dest format for class 1 conversions */
	unsigned *status;

	status = &fpregs[0];
	r1 = ((extru(ir,fpr1pos,5)<<1)|(extru(ir,fpxr1pos,1)));
	if (r1 == 0)
		r1 = fpzeroreg;
	t = ((extru(ir,fptpos,5)<<1)|(extru(ir,fpxtpos,1)));
	if (t == 0 && class != 2)
		return(MAJOR_0E_EXCP);
	if (class < 2)		/* class 0 or 1 has 2 bit fmt */
		fmt = extru(ir,fpfmtpos,2);
	else			/* class 2 and 3 have 1 bit fmt */
		fmt = extru(ir,fp0efmtpos,1);

	switch (class) {
	    case 0:
		switch (subop) {
			case 0: /* unimplemented */
			case 1:
			case 6:
			case 7:
				return(MAJOR_0E_EXCP);
			case 2: /* FCPY */
				switch (fmt) {
				    case 2:
				    case 3:
					return(MAJOR_0E_EXCP);
				    case 1: /* double */
					fpregs[t+1] = fpregs[r1+1];
				    case 0: /* single */
					fpregs[t] = fpregs[r1];
					return(NOEXCEPTION);
				}
			case 3: /* FABS */
				switch (fmt) {
				    case 2:
				    case 3:
					return(MAJOR_0E_EXCP);
				    case 1: /* double */
					fpregs[t+1] = fpregs[r1+1];
				    case 0: /* single */
					fpregs[t] = fpregs[r1] & 0x7fffffff;
					return(NOEXCEPTION);
				}
			case 4: /* FSQRT */
				switch (fmt) {
				    case 0:
					return(sgl_fsqrt(&fpregs[r1],
						&fpregs[t], status));
				    case 1:
					return(dbl_fsqrt(&fpregs[r1],
						&fpregs[t], status));
				    case 2:
				    case 3:
					return(MAJOR_0E_EXCP);
				}
			case 5: /* FRMD */
				switch (fmt) {
				    case 0:
					return(sgl_frnd(&fpregs[r1],
						&fpregs[t], status));
				    case 1:
					return(dbl_frnd(&fpregs[r1],
						&fpregs[t], status));
				    case 2:
				    case 3:
					return(MAJOR_0E_EXCP);
				}
		} /* end of switch (subop */
	
	case 1: /* class 1 */
		df = extru(ir,fpdfpos,2); /* get dest format */
		if ((df & 2) || (fmt & 2))
			return(MAJOR_0E_EXCP);
		
		fmt = (fmt << 1) | df;
		switch (subop) {
			case 0: /* FCNVFF */
				switch(fmt) {
				    case 0: /* sgl/sgl */
					return(MAJOR_0E_EXCP);
				    case 1: /* sgl/dbl */
					return(sgl_to_dbl_fcnvff(&fpregs[r1],
						&fpregs[t],status));
				    case 2: /* dbl/sgl */
					return(dbl_to_sgl_fcnvff(&fpregs[r1],
						&fpregs[t],status));
				    case 3: /* dbl/dbl */
					return(MAJOR_0E_EXCP);
				}
			case 1: /* FCNVXF */
				switch(fmt) {
				    case 0: /* sgl/sgl */
					return(sgl_to_sgl_fcnvxf(&fpregs[r1],
						&fpregs[t],status));
				    case 1: /* sgl/dbl */
					return(sgl_to_dbl_fcnvxf(&fpregs[r1],
						&fpregs[t],status));
				    case 2: /* dbl/sgl */
					return(dbl_to_sgl_fcnvxf(&fpregs[r1],
						&fpregs[t],status));
				    case 3: /* dbl/dbl */
					return(dbl_to_dbl_fcnvxf(&fpregs[r1],
						&fpregs[t],status));
				}
			case 2: /* FCNVFX */
				switch(fmt) {
				    case 0: /* sgl/sgl */
					return(sgl_to_sgl_fcnvfx(&fpregs[r1],
						&fpregs[t],status));
				    case 1: /* sgl/dbl */
					return(sgl_to_dbl_fcnvfx(&fpregs[r1],
						&fpregs[t],status));
				    case 2: /* dbl/sgl */
					return(dbl_to_sgl_fcnvfx(&fpregs[r1],
						&fpregs[t],status));
				    case 3: /* dbl/dbl */
					return(dbl_to_dbl_fcnvfx(&fpregs[r1],
						&fpregs[t],status));
				}
			case 3: /* FCNVFXT */
				switch(fmt) {
				    case 0: /* sgl/sgl */
					return(sgl_to_sgl_fcnvfxt(&fpregs[r1],
						&fpregs[t],status));
				    case 1: /* sgl/dbl */
					return(sgl_to_dbl_fcnvfxt(&fpregs[r1],
						&fpregs[t],status));
				    case 2: /* dbl/sgl */
					return(dbl_to_sgl_fcnvfxt(&fpregs[r1],
						&fpregs[t],status));
				    case 3: /* dbl/dbl */
					return(dbl_to_dbl_fcnvfxt(&fpregs[r1],
						&fpregs[t],status));
				}
		} /* end of switch subop */
	case 2: /* class 2 */
		r2 = ((extru(ir,fpr2pos,5)<<1)|(extru(ir,fpxr2pos,1)));
		if (r2 == 0)
			r2 = fpzeroreg;
		switch (subop) {
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
				return(MAJOR_0E_EXCP);
			case 0: /* FCMP */
				switch (fmt) {
				    /*
				     * fmt is only 1 bit long
				     */
				    case 0:
					return(sgl_fcmp(&fpregs[r1],&fpregs[r2],
						extru(ir,fptpos,5),status));
				    case 1:
					return(dbl_fcmp(&fpregs[r1],&fpregs[r2],
						extru(ir,fptpos,5),status));
				}
		} /* end of switch for class 2 */
	case 3: /* class 3 */
		r2 = ((extru(ir,fpr2pos,5)<<1)|(extru(ir,fpxr2pos,1)));
		if (r2 == 0)
			r2 = fpzeroreg;
		switch (subop) {
			case 5:
			case 6:
			case 7:
				return(MAJOR_0E_EXCP);
			
			/*
			 * Note that fmt is only 1 bit for class 3 */
			case 0: /* FADD */
				switch (fmt) {
				    case 0:
					return(sgl_fadd(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 1:
					return(dbl_fadd(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				}
			case 1: /* FSUB */
				switch (fmt) {
				    case 0:
					return(sgl_fsub(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 1:
					return(dbl_fsub(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				}
			case 2: /* FMPY or XMPYU */
				/*
				 * check for integer multiply (x bit set)
				 */
				if (extru(ir,fpxpos,1)) {
				    /*
				     * emulate XMPYU
				     */
				    switch (fmt) {
					case 0:
					    /*
					     * bad instruction if t specifies
					     * the right half of a register
					     */
					    if (t & 1)
						return(MAJOR_0E_EXCP);
					    /* XXX fredette - broken. */
#if 0
					    impyu(&fpregs[r1],&fpregs[r2],
						&fpregs[t]);
					    return(NOEXCEPTION);
#else
					    panic("impyu");
#endif
					case 1:
						return(MAJOR_0E_EXCP);
				    }
				}
				else { /* FMPY */
				    switch (fmt) {
				        case 0:
					    return(sgl_fmpy(&fpregs[r1],
					       &fpregs[r2],&fpregs[t],status));
				        case 1:
					    return(dbl_fmpy(&fpregs[r1],
					       &fpregs[r2],&fpregs[t],status));
				    }
				}
			case 3: /* FDIV */
				switch (fmt) {
				    case 0:
					return(sgl_fdiv(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 1:
					return(dbl_fdiv(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				}
			case 4: /* FREM */
				switch (fmt) {
				    case 0:
					return(sgl_frem(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				    case 1:
					return(dbl_frem(&fpregs[r1],&fpregs[r2],
						&fpregs[t],status));
				}
		} /* end of class 3 switch */
	} /* end of switch(class) */
	panic("decode_0e");
}


/*
 * routine to decode the 06 (FMPYADD and FMPYCFXT) instruction
 */
int
decode_06(ir,fpregs)
unsigned ir;
unsigned fpregs[];
{
	unsigned rm1, rm2, tm, ra, ta; /* operands */
	unsigned fmt;
	unsigned error = 0;
	unsigned status;
	union {
		double dbl;
		float flt;
		struct { unsigned i1; unsigned i2; } ints;
	} mtmp, atmp;


	status = fpregs[0];		/* use a local copy of status reg */
	fmt = extru(ir, fpmultifmt, 1);	/* get sgl/dbl flag */
	if (fmt == 0) { /* DBL */
		rm1 = extru(ir, fprm1pos, 5) * sizeof(double)/sizeof(unsigned);
		if (rm1 == 0)
			rm1 = fpzeroreg;
		rm2 = extru(ir, fprm2pos, 5) * sizeof(double)/sizeof(unsigned);
		if (rm2 == 0)
			rm2 = fpzeroreg;
		tm = extru(ir, fptmpos, 5) * sizeof(double)/sizeof(unsigned);
		if (tm == 0)
			return(MAJOR_06_EXCP);
		ra = extru(ir, fprapos, 5) * sizeof(double)/sizeof(unsigned);
		ta = extru(ir, fptapos, 5) * sizeof(double)/sizeof(unsigned);
		if (ta == 0)
			return(MAJOR_06_EXCP);
		
#ifdef TIMEX
		if (ra == 0) {
			 /* special case FMPYCFXT */
			if (dbl_fmpy(&fpregs[rm1],&fpregs[rm2],(unsigned *) &mtmp,
					&status))
				error = 1;
			if (dbl_to_sgl_fcnvfxt(&fpregs[ta],(unsigned *) &atmp,
				(unsigned *) &atmp,&status))
				error = 1;
		}
		else {
#else
		if (ra == 0)
			ra = fpzeroreg;
#endif
		
			if (dbl_fmpy(&fpregs[rm1],&fpregs[rm2],(unsigned *) &mtmp,
					&status))
				error = 1;
			if (dbl_fadd(&fpregs[ta], &fpregs[ra], (unsigned *) &atmp,
					&status))
				error = 1;
#ifdef TIMEX
		}
#endif
		if (error)
			return(MAJOR_06_EXCP);
		else {
			/* copy results */
			fpregs[tm] = mtmp.ints.i1;
			fpregs[tm+1] = mtmp.ints.i2;
			fpregs[ta] = atmp.ints.i1;
			fpregs[ta+1] = atmp.ints.i2;
			fpregs[0] = status;
			return(NOEXCEPTION);
		}
	}
	else { /* SGL */
		/*
		 * calculate offsets for single precision numbers
		 * See table 6-14 in PA-89 architecture for mapping
		 */
		rm1 = (extru(ir,fprm1pos,4) | 0x10 ) << 1;	/* get offset */
		rm1 |= extru(ir,fprm1pos-4,1);	/* add right word offset */

		rm2 = (extru(ir,fprm2pos,4) | 0x10 ) << 1;	/* get offset */
		rm2 |= extru(ir,fprm2pos-4,1);	/* add right word offset */

		tm = (extru(ir,fptmpos,4) | 0x10 ) << 1;	/* get offset */
		tm |= extru(ir,fptmpos-4,1);	/* add right word offset */

		ra = (extru(ir,fprapos,4) | 0x10 ) << 1;	/* get offset */
		ra |= extru(ir,fprapos-4,1);	/* add right word offset */

		ta = (extru(ir,fptapos,4) | 0x10 ) << 1;	/* get offset */
		ta |= extru(ir,fptapos-4,1);	/* add right word offset */
		
		if (ra == 0x20) { /* special case FMPYCFXT (really 0) */
			if (sgl_fmpy(&fpregs[rm1],&fpregs[rm2],(unsigned *) &mtmp,
					&status))
				error = 1;
			/* XXX fredette - this is broken */
#if 0
			if (sgl_to_sgl_fcnvfxt(&fpregs[ta],(unsigned *) &atmp,
				(unsigned *) &atmp,&status))
				error = 1;
#else	
				panic("FMPYADD");
#endif
		}
		else {
			if (sgl_fmpy(&fpregs[rm1],&fpregs[rm2],(unsigned *) &mtmp,
					&status))
				error = 1;
			if (sgl_fadd(&fpregs[ta], &fpregs[ra], (unsigned *) &atmp,
					&status))
				error = 1;
		}
		if (error)
			return(MAJOR_06_EXCP);
		else {
			/* copy results */
			fpregs[tm] = mtmp.ints.i1;
			fpregs[ta] = atmp.ints.i1;
			fpregs[0] = status;
			return(NOEXCEPTION);
		}
	}
}

/*
 * routine to decode the 26 (FMPYSUB) instruction
 */
int
decode_26(ir,fpregs)
unsigned ir;
unsigned fpregs[];
{
	unsigned rm1, rm2, tm, ra, ta; /* operands */
	unsigned fmt;
	unsigned error = 0;
	unsigned status;
	union {
		double dbl;
		float flt;
		struct { unsigned i1; unsigned i2; } ints;
	} mtmp, atmp;


	status = fpregs[0];
	fmt = extru(ir, fpmultifmt, 1);	/* get sgl/dbl flag */
	if (fmt == 0) { /* DBL */
		rm1 = extru(ir, fprm1pos, 5) * sizeof(double)/sizeof(unsigned);
		if (rm1 == 0)
			rm1 = fpzeroreg;
		rm2 = extru(ir, fprm2pos, 5) * sizeof(double)/sizeof(unsigned);
		if (rm2 == 0)
			rm2 = fpzeroreg;
		tm = extru(ir, fptmpos, 5) * sizeof(double)/sizeof(unsigned);
		if (tm == 0)
			return(MAJOR_26_EXCP);
		ra = extru(ir, fprapos, 5) * sizeof(double)/sizeof(unsigned);
		if (ra == 0)
			return(MAJOR_26_EXCP);
		ta = extru(ir, fptapos, 5) * sizeof(double)/sizeof(unsigned);
		if (ta == 0)
			return(MAJOR_26_EXCP);
		
		if (dbl_fmpy(&fpregs[rm1],&fpregs[rm2],(unsigned *) &mtmp,
				&status))
			error = 1;
		if (dbl_fsub(&fpregs[ta], &fpregs[ra], (unsigned *) &atmp,
				&status))
			error = 1;
		if (error)
			return(MAJOR_26_EXCP);
		else {
			/* copy results */
			fpregs[tm] = mtmp.ints.i1;
			fpregs[tm+1] = mtmp.ints.i2;
			fpregs[ta] = atmp.ints.i1;
			fpregs[ta+1] = atmp.ints.i2;
			fpregs[0] = status;
			return(NOEXCEPTION);
		}
	}
	else { /* SGL */
		/*
		 * calculate offsets for single precision numbers
		 * See table 6-14 in PA-89 architecture for mapping
		 */
		rm1 = (extru(ir,fprm1pos,4) | 0x10 ) << 1;	/* get offset */
		rm1 |= extru(ir,fprm1pos-4,1);	/* add right word offset */

		rm2 = (extru(ir,fprm2pos,4) | 0x10 ) << 1;	/* get offset */
		rm2 |= extru(ir,fprm2pos-4,1);	/* add right word offset */

		tm = (extru(ir,fptmpos,4) | 0x10 ) << 1;	/* get offset */
		tm |= extru(ir,fptmpos-4,1);	/* add right word offset */

		ra = (extru(ir,fprapos,4) | 0x10 ) << 1;	/* get offset */
		ra |= extru(ir,fprapos-4,1);	/* add right word offset */

		ta = (extru(ir,fptapos,4) | 0x10 ) << 1;	/* get offset */
		ta |= extru(ir,fptapos-4,1);	/* add right word offset */
		
		if (sgl_fmpy(&fpregs[rm1],&fpregs[rm2],(unsigned *) &mtmp,
				&status))
			error = 1;
		if (sgl_fsub(&fpregs[ta], &fpregs[ra], (unsigned *) &atmp,
				&status))
			error = 1;
		if (error)
			return(MAJOR_26_EXCP);
		else {
			/* copy results */
			fpregs[tm] = mtmp.ints.i1;
			fpregs[ta] = atmp.ints.i1;
			fpregs[0] = status;
			return(NOEXCEPTION);
		}
	}

}
