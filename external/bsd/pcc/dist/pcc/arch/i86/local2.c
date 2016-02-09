/*	Id: local2.c,v 1.7 2015/07/24 08:00:12 ragge Exp 	*/	
/*	$NetBSD: local2.c,v 1.1.1.1 2016/02/09 20:28:39 plunky Exp $	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
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

# include "pass2.h"
# include <ctype.h>
# include <string.h>

#define EXPREFIX	"_"

static int stkpos;

static int suppress_type;		/* Don't display a type on the
					   next expansion */

void
deflab(int label)
{
	printf(LABFMT ":\n", label);
}

static int regoff[7];
static TWORD ftype;

/*
 * Print out the prolog assembler.
 * addto and regoff are already calculated.
 */
static void
prtprolog(struct interpass_prolog *ipp, int addto)
{
	int i;

#if 1
	/* FIXME: can't use enter/leave if generating i8086 */
	if (addto == 0 || addto > 65535 || 1) {
		printf("	push bp\n\tmov bp,sp\n");
		if (addto)
			printf("	sub sp,#%d\n", addto);
	} else
		printf("	enter %d,0\n", addto);
#endif
	for (i = 0; i < MAXREGS; i++)
		if (TESTBIT(ipp->ipp_regs, i))
			printf("	mov -%d[%s],%s\n",
			     regoff[i], rnames[FPREG], rnames[i]);
}

/*
 * calculate stack size and offsets
 */
static int
offcalc(struct interpass_prolog *ipp)
{
	int i, addto;

	addto = p2maxautooff;
	if (addto >= AUTOINIT/SZCHAR)
		addto -= AUTOINIT/SZCHAR;
	for (i = 0; i < MAXREGS; i++)
		if (TESTBIT(ipp->ipp_regs, i)) {
			addto += SZINT/SZCHAR;
			regoff[i] = addto;
		}
	return addto;
}

void
prologue(struct interpass_prolog *ipp)
{
	int addto;

	ftype = ipp->ipp_type;

#ifdef LANG_F77
	if (ipp->ipp_vis)
		printf("	.globl %s\n", ipp->ipp_name);
	printf("	.align 4\n");
	printf("%s:\n", ipp->ipp_name);
#endif
	/*
	 * We here know what register to save and how much to 
	 * add to the stack.
	 */
	addto = offcalc(ipp);
	prtprolog(ipp, addto);
}

void
eoftn(struct interpass_prolog *ipp)
{
	int i;

	if (ipp->ipp_ip.ip_lbl == 0)
		return; /* no code needs to be generated */

	/* return from function code */
	for (i = 0; i < MAXREGS; i++)
		if (TESTBIT(ipp->ipp_regs, i))
			printf("	mov %s,-%d[%s]\n",
			    rnames[i],regoff[i], rnames[FPREG]);

	/* struct return needs special treatment */
	if (ftype == STRTY || ftype == UNIONTY) {
		printf("	mov ax, 8[bp]\n");
		printf("	leave\n");
		/* FIXME: ret n is not in 8086 */
		printf("	ret %d\n", 4 + ipp->ipp_argstacksize);
	} else {
		printf("	mov sp, bp\n");
		printf("	pop bp\n");
		if (ipp->ipp_argstacksize)
			/* CHECK ME */
			printf("	add sp, #%d\n", ipp->ipp_argstacksize);
		printf("	ret\n");
	}
}

/*
 * add/sub/...
 *
 * Param given:
 */
void
hopcode(int f, int o)
{
	char *str;

	switch (o) {
	case PLUS:
		str = "add";
		break;
	case MINUS:
		str = "sub";
		break;
	case AND:
		str = "and";
		break;
	case OR:
		str = "or";
		break;
	case ER:
		str = "xor";
		break;
	default:
		comperr("hopcode2: %d", o);
		str = 0; /* XXX gcc */
	}
	printf("%s", str);	/* don't need %c f as far as I can see */
}

/*
 * Return type size in bytes.  Used by R2REGS, arg 2 to offset().
 */
int
tlen(NODE *p)
{
	switch(p->n_type) {
		case CHAR:
		case UCHAR:
			return(1);

		case SHORT:
		case USHORT:
		case INT:
		case UNSIGNED:
			return(SZINT/SZCHAR);

		case DOUBLE:
			return(SZDOUBLE/SZCHAR);

		case LONG:
		case ULONG:
			return(SZLONG/SZCHAR);

		case LONGLONG:
		case ULONGLONG:
			return SZLONGLONG/SZCHAR;

		default:
			if (!ISPTR(p->n_type))
				comperr("tlen type %d not pointer");
			return SZPOINT(p->n_type)/SZCHAR;
		}
}

/*
 * Emit code to compare two long numbers.
 */
static void
twollcomp(NODE *p)
{
	int u;
	int s = getlab2();
	int e = p->n_label;
	int cb1, cb2;

	u = p->n_op;
	switch (p->n_op) {
	case NE:
		cb1 = 0;
		cb2 = NE;
		break;
	case EQ:
		cb1 = NE;
		cb2 = 0;
		break;
	case LE:
	case LT:
		u += (ULE-LE);
		/* FALLTHROUGH */
	case ULE:
	case ULT:
		cb1 = GT;
		cb2 = LT;
		break;
	case GE:
	case GT:
		u += (ULE-LE);
		/* FALLTHROUGH */
	case UGE:
	case UGT:
		cb1 = LT;
		cb2 = GT;
		break;
	
	default:
		cb1 = cb2 = 0; /* XXX gcc */
	}
	if (p->n_op >= ULE)
		cb1 += 4, cb2 += 4;
	expand(p, 0, "	cmp UL,UR\n");
	if (cb1) cbgen(cb1, s);
	if (cb2) cbgen(cb2, e);
	expand(p, 0, "	cmp AL,AR\n");
	cbgen(u, e);
	deflab(s);
}

int
fldexpand(NODE *p, int cookie, char **cp)
{
	comperr("fldexpand");
	return 0;
}

/*
 * Push a structure on stack as argument.
 * the scratch registers are already free here
 */
static void
starg(NODE *p)
{
	NODE *q = p->n_left;
	int s = (attr_find(p->n_ap, ATTR_P2STRUCT)->iarg(0) + 1) & ~1;

	if (s == 2)
		printf("	dec sp\n	dec sp\n");
	else
		printf("	sub sp,#%d\n", s);
	p->n_left = mklnode(OREG, 0, SP, INT);
	zzzcode(p, 'Q');
	tfree(p->n_left);
	p->n_left = q;
}

/*
 * Compare two floating point numbers.
 */
static void
fcomp(NODE *p)  
{
	static char *fpcb[] = { "jz", "jnz", "jbe", "jc", "jnc", "ja" };

	if ((p->n_su & DORIGHT) == 0)
		expand(p, 0, "\tfxch\n");
	expand(p, 0, "\tfucomip %st(1),%st\n");	/* emit compare insn  */
	expand(p, 0, "\tfstp %st(0)\n");	/* pop fromstack */

	if (p->n_op == NE || p->n_op == GT || p->n_op == GE)
		expand(p, 0, "\tjp LC\n");
	else if (p->n_op == EQ)
		printf("\tjp 1f\n");
	printf("	%s ", fpcb[p->n_op - EQ]);
	expand(p, 0, "LC\n");
	if (p->n_op == EQ)
		printf("1:\n");
}

/*
 * Convert an unsigned long long to floating point number.
 */
static void
ulltofp(NODE *p)
{
	int jmplab;

	jmplab = getlab2();
	expand(p, 0, "	push UL\n	push AL\n");
	expand(p, 0, "	fildq [sp]\n");
	expand(p, 0, "	add sp, #8\n");
	expand(p, 0, "	cmp UL, #0\n");
	printf("	jge " LABFMT "\n", jmplab);

	printf("	faddp %%st,%%st(1)\n");
	printf(LABFMT ":\n", jmplab);
}

static int
argsiz(NODE *p)
{
	TWORD t = p->n_type;
	if (t < LONG)
		return 2;
	if (t == LONG || t == ULONG || t == LONGLONG || t == ULONGLONG)
		return 4;
	if (t == FLOAT)
		return 4;
	if (t > BTMASK)
		return 2;
	if (t == DOUBLE)
		return 8;
	if (t == LDOUBLE)
		return 12;
	if (t == STRTY || t == UNIONTY)
		return (attr_find(p->n_ap, ATTR_P2STRUCT)->iarg(0)+1) & ~1;
	comperr("argsiz");
	return 0;
}

static void
fcast(NODE *p)
{
	TWORD t = p->n_type;
	int sz, c;

	if (t >= p->n_left->n_type)
		return; /* cast to more precision */
	if (t == FLOAT)
		sz = 4, c = 's';
	else
		sz = 8, c = 'l';

	printf("	sub sp, #%d\n", sz);
	printf("	fstp%c (%%sp)\n", c);
	printf("	fld%c (%%sp)\n", c);
	printf("	add sp, #%d\n", sz);
}


#if 0	

static void
llshft(NODE *p)
{
	NODE *r = p->n_right;
	/* FIXME: we have sal/shl/sar/shr but we are limited to
	   shift right or left 1
	   shift right or left by CL */
	char *d[3];
	/* We ought to shortcut this earlier but it's not obivous how
	   to do a match and say 'any old register pair' FIXME */
	if (r->n_op == ICON) {
		/* FIXME: we need different versions of this for signed right
		   shifting. Just test code - register setup needs checking
		   for ordering etc yet */
		if (r->n_lval == 16) {
			if (p->n_op == RS)
				printf("\tmov ax, dx\n\txor dx,dx\n");
			else
				printf("\tmov dx, ax\n\txor ax,ax\n");
			return;
		}
		if (r->n_lval == 8) {
			if (p->n_op == RS)
				printf("\tmov al, ah\n\tmov ah, dl\n\tmov dl, dh\n\txor dh,dh\n");
			else
				printf("\tmov ah, al\n\tmov dl, ah\n\tmov dh, dl\n\txor al, al\n");
			return;
		} 
		if (r->n_lval == 24) {
			if (p->n_op == RS)
				printf("\txor dx,dx\n\tmov al, dh\n\txor ax,ax\n");
			else
				printf("\txor ax,ax\n\tmov dh, al\n\txor dx,dx\n");
			return;
		}
	}
	if (p->n_op == LS) {
		d[0] = "l", d[1] = "ax", d[2] = "dx";
	} else
		d[0] = "r", d[1] = "dx", d[2] = "ax";

	printf("\tsh%sdl %s,%s\n",d[0], d[1], d[2]);
	printf("\ts%s%sl %%cl,%s\n", p->n_op == RS &&
	    (p->n_left->n_type == ULONG || p->n_left->n_type == ULONGLONG) ?
	    				"h" : "a", d[0], d[1]);
	printf("\ttestb $16,%%cl\n");
	printf("\tje 1f\n");
	printf("\tmov %s,%s\n", d[1], d[2]);
	if (p->n_op == RS && (p->n_left->n_type == LONGLONG|| p->n_left->n_type == LONG))
		printf("\tsarl $31,%%edx\n");
	else
		printf("\txor %s,%s\n",d[1],d[1]);
	printf("1:\n");
}
#endif

void
zzzcode(NODE *p, int c)
{
	struct attr *ap;
	NODE *l;
	int pr, lr;
	char *ch;
	char sv;

	switch (c) {
	case 'A': /* swap st0 and st1 if right is evaluated second */
		if ((p->n_su & DORIGHT) == 0) {
			if (logop(p->n_op))
				printf("	fxch\n");
			else
				printf("r");
		}
		break;

	case 'C':  /* remove from stack after subroutine call */
#ifdef notdef
		if (p->n_left->n_flags & FSTDCALL)
			break;
#endif
		pr = p->n_qual;
		if (attr_find(p->n_ap, ATTR_I86_FPPOP))
			printf("	fstp	st(0)\n");
		if (p->n_op == UCALL)
			return; /* XXX remove ZC from UCALL */
		if (pr) {
			if (pr == 2)
				printf("	inc sp\n	inc sp\n");
			else
				printf("	add sp,#%d\n", pr);
		}
		break;

	case 'D': /* Long comparision */
		twollcomp(p);
		break;

	case 'F': /* Structure argument */
		starg(p);
		break;

	case 'G': /* Floating point compare */
		fcomp(p);
		break;

	case 'H': /* assign of long between regs */
		rmove(DECRA(p->n_right->n_reg, 0),
		    DECRA(p->n_left->n_reg, 0), LONGLONG);
		break;

	case 'I': /* float casts */
		fcast(p);
		break;

	case 'J': /* convert unsigned long long to floating point */
		ulltofp(p);
		break;

	case 'K': /* Load long reg into another reg */
		rmove(regno(p), DECRA(p->n_reg, 0), LONGLONG);
		break;

	case 'M': /* Output sconv move, if needed */
		l = getlr(p, 'L');
		/* XXX fixneed: regnum */
		pr = DECRA(p->n_reg, 0);
		lr = DECRA(l->n_reg, 0);
		if ((pr == AL && lr == AX) || (pr == BL && lr == BX) ||
		    (pr == CL && lr == CX) || (pr == DL && lr == DX))
			;
		else
			printf("	mov %s, %cL\n",
			    rnames[pr], rnames[lr][1]);
		l->n_rval = l->n_reg = p->n_reg; /* XXX - not pretty */
		break;

	case 'N': /* output extended reg name */
		printf("%s", rnames[getlr(p, '1')->n_rval]);
		break;

	case 'O': /* print out emulated ops */
		pr = 8;
		sv = 'l';
#if 0		
		if (p->n_op == RS || p->n_op == LS) {
			llshft(p);
			break;
		}
#endif
		if (p->n_op != RS && p->n_op != LS) {
			/* For 64bit we will need to push pointers
			   not u64 */
			expand(p, INCREG, "\tpush UR\n\tpush AR\n");
			expand(p, INCREG, "\tpush UL\n\tpush AL\n");
		} else {
			/* AR is a BREG so this goes wrong.. need an AREG
			   but putting an AREG in the table rule makes the
			   compiler shit itself - FIXME */
			expand(p, INAREG, "\tpush AR\n");
			expand(p, INCREG, "\tpush UL\n\tpush AL\n");
			pr = 6;
		}
		
		if (p->n_type == LONGLONG || p->n_type == ULONGLONG)
			sv = 'L';
		if (p->n_op == DIV && (p->n_type == ULONG || p->n_type == ULONGLONG))
			ch = "udiv";
		else if (p->n_op == MUL && (p->n_type == ULONG || p->n_type == ULONGLONG))
			ch = "umul";
		else if (p->n_op == DIV) ch = "div";
		else if (p->n_op == MUL) ch = "mul";
		else if (p->n_op == MOD && (p->n_type == ULONG || p->n_type == ULONGLONG))
			ch = "umod";
		else if (p->n_op == MOD) ch = "mod";
		else if (p->n_op == LS) ch = "ls";
		else if (p->n_op == RS && (p->n_type == ULONG || p->n_type == ULONGLONG))
			ch = "rs";
		else if (p->n_op == RS) ch = "urs";
		else ch = 0, comperr("ZO");
		printf("\tcall " EXPREFIX "__%c%sdi3\n\tadd %s,#%d\n",
			sv, ch, rnames[SP], pr);
                break;
	
	case 'P': /* typeless right hand */
		suppress_type = 1;
		break;

	case 'Q': /* emit struct assign */
		/*
		 * Put out some combination of movs{b,w}
		 * si/di/cx are available.
		 * FIXME: review es: and direction flag implications
		 */
		expand(p, INAREG, "	lea al,di\n");
		ap = attr_find(p->n_ap, ATTR_P2STRUCT);
		if (ap->iarg(0) < 32) {
			int i = ap->iarg(0) >> 1;
			while (i) {
				expand(p, INAREG, "	movsw\n");
				i--;
			}
		} else {
			printf("\tmov cx, #%d\n", ap->iarg(0) >> 1);
			printf("	rep movsw\n");
		}
		if (ap->iarg(0) & 2)
			printf("	movsw\n");
		if (ap->iarg(0) & 1)
			printf("	movsb\n");
		break;

	case 'S': /* emit eventual move after cast from long */
		pr = DECRA(p->n_reg, 0);
		lr = p->n_left->n_rval;
		switch (p->n_type) {
		case CHAR:
		case UCHAR:
			if (rnames[pr][1] == 'L' && rnames[lr][1] == 'X' &&
			    rnames[pr][0] == rnames[lr][0])
				break;
			if (rnames[lr][1] == 'X') {
				printf("\tmov %s, %cL\n",
				    rnames[pr], rnames[lr][0]);
				break;
			}
			/* Must go via stack */
			expand(p, INAREG, "\tmov A2,AL\n");
			expand(p, INBREG, "\tmov A1,A2\n");
#ifdef notdef
			/* cannot use freetemp() in instruction emission */
			s = freetemp(1);
			printf("\tmov %ci,%d(bp)\n", rnames[lr][0], s);
			printf("\tmov %s, %d(bp)\n", rnames[pr], s);
#endif
			break;

		case INT:
		case UNSIGNED:
			if (rnames[lr][0] == rnames[pr][1] &&
			    rnames[lr][1] == rnames[pr][2])
				break;
			printf("\tmov %s, %c%c\n",
				    rnames[pr], rnames[lr][0], rnames[lr][1]);
			break;

		default:
			if (rnames[lr][0] == rnames[pr][1] &&
			    rnames[lr][1] == rnames[pr][2])
				break;
			comperr("SCONV2 %s->%s", rnames[lr], rnames[pr]);
			break;
		}
		break;
	case 'T':
		/* Conversions from unsigned char to int/ptr */
		/* Cannot be in DI or SI */
		l = getlr(p, 'L');
		lr = regno(l);
		pr = regno(getlr(p, '1'));
		if (l->n_op != REG) { /* NAME or OREG */
			/* Need to force a mov into the low half, using
			   a movw might fail in protected mode or with
			   mmio spaces */
			printf("	mov %cl, ", rnames[pr][0]);
			expand(p, INAREG, "AL\n");
		} else {
		        if ((lr == AL && pr == AX) ||
		            (lr == BL && pr == BX) ||
			    (lr == CL && pr == CX) ||
			    (lr == DL && pr == DX))
				;
                         else
                         	printf("	mov %cl,%cl\n",
                         		rnames[pr][0],rnames[lr][0]);
                }
		printf("	xor %ch,%ch\n",
				rnames[pr][0], rnames[pr][0]);
		break;
		                                                                                                                                                                                                   break;
	default:
		comperr("zzzcode %c", c);
	}
}

int canaddr(NODE *);
int
canaddr(NODE *p)
{
	int o = p->n_op;

	if (o==NAME || o==REG || o==ICON || o==OREG ||
	    (o==UMUL && shumul(p->n_left, SOREG)))
		return(1);
	return(0);
}

/*
 * Does the bitfield shape match?
 */
int
flshape(NODE *p)
{
	comperr("flshape");
	return 0;
}

/* INTEMP shapes must not contain any temporary registers */
/* XXX should this go away now? */
int
shtemp(NODE *p)
{
	return 0;
#if 0
	int r;

	if (p->n_op == STARG )
		p = p->n_left;

	switch (p->n_op) {
	case REG:
		return (!istreg(p->n_rval));

	case OREG:
		r = p->n_rval;
		if (R2TEST(r)) {
			if (istreg(R2UPK1(r)))
				return(0);
			r = R2UPK2(r);
		}
		return (!istreg(r));

	case UMUL:
		p = p->n_left;
		return (p->n_op != UMUL && shtemp(p));
	}

	if (optype(p->n_op) != LTYPE)
		return(0);
	return(1);
#endif
}

void
adrcon(CONSZ val)
{
	printf("$" CONFMT, val);
}

void
conput(FILE *fp, NODE *p)
{
	int val = (int)p->n_lval;

	switch (p->n_op) {
	case ICON:
		if (p->n_name[0] != '\0') {
			fprintf(fp, "#%s", p->n_name);
			if (val)
				fprintf(fp, "+%d", val);
		} else
			fprintf(fp, "#%d", val);
		return;

	default:
		comperr("illegal conput, p %p", p);
	}
}

/*ARGSUSED*/
void
insput(NODE *p)
{
	comperr("insput");
}

/*
 * Write out the upper address, like the upper register of a 2-register
 * reference, or the next memory location.
 */
void
upput(NODE *p, int size)
{

	size /= SZCHAR;
	switch (p->n_op) {
	case REG:
		printf("%s", &rnames[p->n_rval][2]);
		break;

	case NAME:
	case OREG:
		p->n_lval += size;
		adrput(stdout, p);
		p->n_lval -= size;
		break;
	case ICON:
		printf("#"CONFMT, p->n_lval >> 16);
		break;
	default:
		comperr("upput bad op %d size %d", p->n_op, size);
	}
}

static void do_adrput(FILE *io, NODE *p, int ut)
{
	int r;
	/* output an address, with offsets, from p */

	switch (p->n_op) {

	case NAME:
		if (!ut) {
			switch (p->n_type) {
				case SHORT:
				case USHORT:
				case INT:
				case UNSIGNED:
					printf("word ptr ");
					break;
				case CHAR:
				case UCHAR:
					printf("byte ptr ");
			}
		}
		if (p->n_name[0] != '\0') {
			fputs(p->n_name, io);
			if (p->n_lval != 0)
				fprintf(io, "+" CONFMT, p->n_lval);
		} else
			fprintf(io, "#"CONFMT, p->n_lval);
		return;

	case OREG:
		r = p->n_rval;
		if (p->n_name[0])
			printf("%s%s", p->n_name, p->n_lval ? "+" : "");
		if (p->n_lval)
			fprintf(io, "%d", (int)p->n_lval);
		printf("[");
		if (R2TEST(r)) {
			fprintf(io, "%s,%s,4", rnames[R2UPK1(r)],
			    rnames[R2UPK2(r)]);
		} else
			fprintf(io, "%s", rnames[p->n_rval]);
		printf("]");
		return;
	case ICON:
		/* addressable value of the constant */
		if (!ut) {
			switch (p->n_type) {
				case SHORT:
				case USHORT:
				case INT:
				case UNSIGNED:
					printf("word ");
					break;
					case CHAR:
				case UCHAR:
					printf("byte ");
			}
		}
		conput(io, p);
		return;

	case REG:
		switch (p->n_type) {
		case LONGLONG:
		case ULONGLONG:
		case LONG:
		case ULONG:
			fprintf(io, "%c%c", rnames[p->n_rval][0],
			    rnames[p->n_rval][1]);
			break;
		default:
			fprintf(io, "%s", rnames[p->n_rval]);
		}
		return;

	default:
		comperr("illegal address, op %d, node %p", p->n_op, p);
		return;

	}
}

void adrput(FILE *io, NODE *p)
{
	do_adrput(io, p, suppress_type);
	suppress_type = 0;
}

static char *
ccbranches[] = {
	"je",		/* jumpe */
	"jne",		/* jumpn */
	"jle",		/* jumple */
	"jl",		/* jumpl */
	"jge",		/* jumpge */
	"jg",		/* jumpg */
	"jbe",		/* jumple (jlequ) */
	"jb",		/* jumpl (jlssu) */
	"jae",		/* jumpge (jgequ) */
	"ja",		/* jumpg (jgtru) */
};


/*   printf conditional and unconditional branches */
void
cbgen(int o, int lab)
{
	if (o < EQ || o > UGT)
		comperr("bad conditional branch: %s", opst[o]);
	printf("	%s " LABFMT "\n", ccbranches[o-EQ], lab);
}

static void
fixcalls(NODE *p, void *arg)
{
	/* Prepare for struct return by allocating bounce space on stack */
	switch (p->n_op) {
	case STCALL:
	case USTCALL:
		if (attr_find(p->n_ap, ATTR_P2STRUCT)->iarg(0)+p2autooff > stkpos)
			stkpos = attr_find(p->n_ap, ATTR_P2STRUCT)->iarg(0)+p2autooff;
		break;
	case LS:
	case RS:
		if (p->n_type != LONGLONG && p->n_type != ULONGLONG
		    && p->n_type != LONG && p->n_type != ULONG)
			break;
		if (p->n_right->n_op == ICON) /* constants must be char */
			p->n_right->n_type = CHAR;
		break;
	}
}

/*
 * Must store floats in memory if there are two function calls involved.
 */
static int
storefloat(struct interpass *ip, NODE *p)
{
	int l, r;

	switch (optype(p->n_op)) {
	case BITYPE:
		l = storefloat(ip, p->n_left);
		r = storefloat(ip, p->n_right);
		if (p->n_op == CM)
			return 0; /* arguments, don't care */
		if (callop(p->n_op))
			return 1; /* found one */
#define ISF(p) ((p)->n_type == FLOAT || (p)->n_type == DOUBLE || \
	(p)->n_type == LDOUBLE)
		if (ISF(p->n_left) && ISF(p->n_right) && l && r) {
			/* must store one. store left */
			struct interpass *nip;
			TWORD t = p->n_left->n_type;
			NODE *ll;
			int off;

                	off = freetemp(szty(t));
                	ll = mklnode(OREG, off, FPREG, t);
			nip = ipnode(mkbinode(ASSIGN, ll, p->n_left, t));
			p->n_left = mklnode(OREG, off, FPREG, t);
                	DLIST_INSERT_BEFORE(ip, nip, qelem);
		}
		return l|r;

	case UTYPE:
		l = storefloat(ip, p->n_left);
		if (callop(p->n_op))
			l = 1;
		return l;
	default:
		return 0;
	}
}

static void
outfargs(struct interpass *ip, NODE **ary, int num, int *cwp, int c)
{
	struct interpass *ip2;
	NODE *q, *r;
	int i;

	for (i = 0; i < num; i++)
		if (XASMVAL(cwp[i]) == c && (cwp[i] & (XASMASG|XASMINOUT)))
			break;
	if (i == num)
		return;
	q = ary[i]->n_left;
	r = mklnode(REG, 0, c == 'u' ? 040 : 037, q->n_type);
	ary[i]->n_left = tcopy(r);
	ip2 = ipnode(mkbinode(ASSIGN, q, r, q->n_type));
	DLIST_INSERT_AFTER(ip, ip2, qelem);
}

static void
infargs(struct interpass *ip, NODE **ary, int num, int *cwp, int c)
{
	struct interpass *ip2;
	NODE *q, *r;
	int i;

	for (i = 0; i < num; i++)
		if (XASMVAL(cwp[i]) == c && (cwp[i] & XASMASG) == 0)
			break;
	if (i == num)
		return;
	q = ary[i]->n_left;
	q = (cwp[i] & XASMINOUT) ? tcopy(q) : q;
	r = mklnode(REG, 0, c == 'u' ? 040 : 037, q->n_type);
	if ((cwp[i] & XASMINOUT) == 0)
		ary[i]->n_left = tcopy(r);
	ip2 = ipnode(mkbinode(ASSIGN, r, q, q->n_type));
	DLIST_INSERT_BEFORE(ip, ip2, qelem);
}

/*
 * Extract float args to XASM and ensure that they are put on the stack
 * in correct order.
 * This should be done sow other way.
 */
static void
fixxfloat(struct interpass *ip, NODE *p)
{
	NODE *w, **ary;
	int nn, i, c, *cwp;

	nn = 1;
	w = p->n_left;
	if (w->n_op == ICON && w->n_type == STRTY)
		return;
	/* index all xasm args first */
	for (; w->n_op == CM; w = w->n_left)
		nn++;
	ary = tmpcalloc(nn * sizeof(NODE *));
	cwp = tmpcalloc(nn * sizeof(int));
	for (i = 0, w = p->n_left; w->n_op == CM; w = w->n_left) {
		ary[i] = w->n_right;
		cwp[i] = xasmcode(ary[i]->n_name);
		i++;
	}
	ary[i] = w;
	cwp[i] = xasmcode(ary[i]->n_name);
	for (i = 0; i < nn; i++)
		if (XASMVAL(cwp[i]) == 't' || XASMVAL(cwp[i]) == 'u')
			break;
	if (i == nn)
		return;

	for (i = 0; i < nn; i++) {
		c = XASMVAL(cwp[i]);
		if (c >= '0' && c <= '9')
			cwp[i] = (cwp[i] & ~0377) | XASMVAL(cwp[c-'0']);
	}
	infargs(ip, ary, nn, cwp, 'u');
	infargs(ip, ary, nn, cwp, 't');
	outfargs(ip, ary, nn, cwp, 't');
	outfargs(ip, ary, nn, cwp, 'u');
}

static NODE *
lptr(NODE *p)
{
	if (p->n_op == ASSIGN && p->n_right->n_op == REG &&
	    regno(p->n_right) == BP)
		return p->n_right;
	if (p->n_op == FUNARG && p->n_left->n_op == REG &&
	    regno(p->n_left) == BP)
		return p->n_left;
	return NIL;
}

/*
 * Find arg reg that should be struct reference instead.
 */
static void
updatereg(NODE *p, void *arg)
{
	NODE *q;

	if (p->n_op != STCALL)
		return;
	if (p->n_right->n_op != CM)
		p = p->n_right;
	else for (p = p->n_right;
	    p->n_op == CM && p->n_left->n_op == CM; p = p->n_left)
		;
	if (p->n_op == CM) {
		if ((q = lptr(p->n_left)))
			;
		else
			q = lptr(p->n_right);
	} else
		q = lptr(p);
	if (q == NIL)
		comperr("bad STCALL hidden reg");

	/* q is now the hidden arg */
	q->n_op = MINUS;
	q->n_type = INCREF(CHAR);
	q->n_left = mklnode(REG, 0, BP, INCREF(CHAR));
	q->n_right = mklnode(ICON, stkpos, 0, INT);
}

void
myreader(struct interpass *ipole)
{
	struct interpass *ip;

	stkpos = p2autooff;
	DLIST_FOREACH(ip, ipole, qelem) {
		if (ip->type != IP_NODE)
			continue;
		walkf(ip->ip_node, fixcalls, 0);
		storefloat(ip, ip->ip_node);
		if (ip->ip_node->n_op == XASM)
			fixxfloat(ip, ip->ip_node);
	}
	if (stkpos != p2autooff) {
		DLIST_FOREACH(ip, ipole, qelem) {
			if (ip->type != IP_NODE)
				continue;
			walkf(ip->ip_node, updatereg, 0);
		}
	}
	if (stkpos > p2autooff)
		p2autooff = stkpos;
	if (stkpos > p2maxautooff)
		p2maxautooff = stkpos;
	if (x2debug)
		printip(ipole);
}

/*
 * Remove some PCONVs after OREGs are created.
 */
static void
pconv2(NODE *p, void *arg)
{
	NODE *q;
	if (p->n_op == PLUS) {
		if (p->n_type == (PTR|SHORT) || p->n_type == (PTR|USHORT)) {
			if (p->n_right->n_op != ICON)
				return;
			if (p->n_left->n_op != PCONV)
				return;
			if (p->n_left->n_left->n_op != OREG)
				return;
			q = p->n_left->n_left;
			nfree(p->n_left);
			p->n_left = q;
			/*
			 * This will be converted to another OREG later.
			 */
		}
	}
}

void
mycanon(NODE *p)
{
	walkf(p, pconv2, 0);
}

void
myoptim(struct interpass *ip)
{
}

static char rl[] =
  { AX, AX, AX, AX, AX, DX, DX, DX, DX, CX, CX, CX, BX, BX, SI };
static char rh[] =
  { DX, CX, BX, SI, DI, CX, BX, SI, DI, BX, SI, DI, SI, DI, DI };

void
rmove(int s, int d, TWORD t)
{
	int sl, sh, dl, dh;

	switch (t) {
	case LONGLONG:
	case ULONGLONG:
	case LONG:
	case ULONG:		/* ?? FIXME check */
#if 1
		sl = rl[s-AXDX];
		sh = rh[s-AXDX];
		dl = rl[d-AXDX];
		dh = rh[d-AXDX];

		/* sanity checks, remove when satisfied */
		if (memcmp(rnames[s], rnames[sl], 2) != 0 ||
		    memcmp(rnames[s]+2, rnames[sh], 2) != 0)
			comperr("rmove source error");
		if (memcmp(rnames[d], rnames[dl], 2) != 0 ||
		    memcmp(rnames[d]+2, rnames[dh], 2) != 0)
			comperr("rmove dest error");
#define	SW(x,y) { int i = x; x = y; y = i; }
		if (sh == dl) {
			/* Swap if overwriting */
			SW(sl, sh);
			SW(dl, dh);
		}
		if (sl != dl)
			printf("	mov %s,%s\n", rnames[dl], rnames[sl]);
		if (sh != dh)
			printf("	mov %s,%s\n", rnames[dh], rnames[sh]);
#else
		if (memcmp(rnames[s], rnames[d], 2) != 0)
			printf("	mov %c%c,%c%c\n",
			    rnames[d][0],rnames[d][1],
			    rnames[s][0],rnames[s][1]);
		if (memcmp(&rnames[s][2], &rnames[d][2], 2) != 0)
			printf("	mov %c%c,%c%c\n",
			    rnames[d][2],rnames[d][3]
			    rnames[s][2],rnames[s][3]);
#endif
		break;
	case CHAR:
	case UCHAR:
		printf("	mov %s,%s\n", rnames[d], rnames[s]);
		break;
	case FLOAT:
	case DOUBLE:
	case LDOUBLE:
#ifdef notdef
		/* a=b()*c(); will generate this */
		comperr("bad float rmove: %d %d", s, d);
#endif
		break;
	default:
		printf("	mov %s,%s\n", rnames[d], rnames[s]);
	}
}

/*
 * For class c, find worst-case displacement of the number of
 * registers in the array r[] indexed by class.
 */
int
COLORMAP(int c, int *r)
{
	int num;

	switch (c) {
	case CLASSA:
		num = r[CLASSB] > 4 ? 4 : r[CLASSB];
		num += 2*r[CLASSC];
		num += r[CLASSA];
		return num < 6;
	case CLASSB:
		num = r[CLASSA];
		num += 2*r[CLASSC];
		num += r[CLASSB];
		return num < 4;
	case CLASSC:
		num = r[CLASSA];
		num += r[CLASSB] > 4 ? 4 : r[CLASSB];
		num += 2*r[CLASSC];
		return num < 5;
	case CLASSD:
		return r[CLASSD] < DREGCNT;
	}
	return 0; /* XXX gcc */
}

char *rnames[] = {
	"ax", "dx", "cx", "bx", "si", "di", "bp", "sp",
	"al", "ah", "dl", "dh", "cl", "ch", "bl", "bh",
	"axdx", "axcx", "axbx", "axsi", "axdi", "dxcx",
	"dxbx", "dxsi", "dxdi", "cxbx", "cxsi", "cxdi",
	"bxsi", "bxdi", "sidi",
	"st0", "st1", "st2", "st3", "st4", "st5", "st6", "st7",
};

/*
 * Return a class suitable for a specific type.
 */
int
gclass(TWORD t)
{
	if (t == CHAR || t == UCHAR)
		return CLASSB;
	if (t == LONGLONG || t == ULONGLONG || t == LONG || t == ULONG)
		return CLASSC;
	if (t == FLOAT || t == DOUBLE || t == LDOUBLE)
		return CLASSD;
	return CLASSA;
}

/*
 * Calculate argument sizes.
 */
void
lastcall(NODE *p)
{
	NODE *op = p;
	int size = 0;

	p->n_qual = 0;
	if (p->n_op != CALL && p->n_op != FORTCALL && p->n_op != STCALL)
		return;
	for (p = p->n_right; p->n_op == CM; p = p->n_left) { 
		if (p->n_right->n_op != ASSIGN)
			size += argsiz(p->n_right);
	}
	if (p->n_op != ASSIGN)
		size += argsiz(p);

	op->n_qual = size; /* XXX */
}

/*
 * Special shapes.
 */
int
special(NODE *p, int shape)
{
	int o = p->n_op;

	switch (shape) {
	case SFUNCALL:
		if (o == STCALL || o == USTCALL)
			return SRREG;
		break;
	case SPCON:
		if (o != ICON || p->n_name[0] ||
		    p->n_lval < 0 || p->n_lval > 0x7fffffff)
			break;
		return SRDIR;
	case SMIXOR:
		return tshape(p, SZERO);
	case SMILWXOR:
		if (o != ICON || p->n_name[0] ||
		    p->n_lval == 0 || p->n_lval & 0xffffffff)
			break;
		return SRDIR;
	case SMIHWXOR:
		if (o != ICON || p->n_name[0] ||
		     p->n_lval == 0 || (p->n_lval >> 32) != 0)
			break;
		return SRDIR;
	case STWO:
		if (o == ICON && p->n_name[0] == 0 && p->n_lval == 2)
			return SRDIR;
		break;
	case SMTWO:
		if (o == ICON && p->n_name[0] == 0 && p->n_lval == -2)
			return SRDIR;
		break;
	}
	return SRNOPE;
}

/*
 * Target-dependent command-line options.
 */
void
mflags(char *str)
{
}

/*
 * Do something target-dependent for xasm arguments.
 */
int
myxasm(struct interpass *ip, NODE *p)
{
	struct interpass *ip2;
	int Cmax[] = { 31, 63, 127, 0xffff, 3, 255 };
	NODE *in = 0, *ut = 0;
	TWORD t;
	char *w;
	int reg;
	int c, cw;
	CONSZ v;

	cw = xasmcode(p->n_name);
	if (cw & (XASMASG|XASMINOUT))
		ut = p->n_left;
	if ((cw & XASMASG) == 0)
		in = p->n_left;

	c = XASMVAL(cw);
	switch (c) {
	case 'D': reg = DI; break;
	case 'S': reg = SI; break;
	case 'a': reg = AX; break;
	case 'b': reg = BX; break;
	case 'c': reg = CX; break;
	case 'd': reg = DX; break;

	case 't':
	case 'u':
		p->n_name = tmpstrdup(p->n_name);
		w = strchr(p->n_name, XASMVAL(cw));
		*w = 'r'; /* now reg */
		return 1;

	case 'A': reg = AXDX; break;
	case 'q': {
		/* Set edges in MYSETXARG */
		if (p->n_left->n_op == REG || p->n_left->n_op == TEMP)
			return 1;
		t = p->n_left->n_type;
		if (in && ut)
			in = tcopy(in);
		p->n_left = mklnode(TEMP, 0, p2env.epp->ip_tmpnum++, t);
		if (ut) {
			ip2 = ipnode(mkbinode(ASSIGN, ut, tcopy(p->n_left), t));
			DLIST_INSERT_AFTER(ip, ip2, qelem);
		}
		if (in) {
			ip2 = ipnode(mkbinode(ASSIGN, tcopy(p->n_left), in, t));
			DLIST_INSERT_BEFORE(ip, ip2, qelem);
		}
		return 1;
	}

	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
		if (p->n_left->n_op != ICON) {
			if ((c = XASMVAL1(cw)) != 0) {
				p->n_name++;
				return 0; /* Try again */
			}
			uerror("xasm arg not constant");
		}
		v = p->n_left->n_lval;
		if ((c == 'K' && v < -128) ||
		    (c == 'L' && v != 0xff && v != 0xffff) ||
		    (c != 'K' && v < 0) ||
		    (v > Cmax[c-'I']))
			uerror("xasm val out of range");
		p->n_name = "i";
		return 1;

	default:
		return 0;
	}
	/* If there are requested either memory or register, delete memory */
	w = p->n_name = tmpstrdup(p->n_name);
	if (*w == '=')
		w++;
	*w++ = 'r';
	*w = 0;

	t = p->n_left->n_type;
	if (reg == AXDX) {
		;
	} else {
		if (t == CHAR || t == UCHAR) {
			reg = reg * 2 + 8;
		}
	}
	if (t == FLOAT || t == DOUBLE || t == LDOUBLE) {
		reg += 037;
	}

	if (in && ut)
		in = tcopy(in);
	p->n_left = mklnode(REG, 0, reg, t);
	if (ut) {
		ip2 = ipnode(mkbinode(ASSIGN, ut, tcopy(p->n_left), t));
		DLIST_INSERT_AFTER(ip, ip2, qelem);
	}
	if (in) {
		ip2 = ipnode(mkbinode(ASSIGN, tcopy(p->n_left), in, t));
		DLIST_INSERT_BEFORE(ip, ip2, qelem);
	}
	return 1;
}

void
targarg(char *w, void *arg)
{
	NODE **ary = arg;
	NODE *p, *q;

	if (ary[(int)w[1]-'0'] == 0)
		p = ary[(int)w[1]-'0'-1]->n_left; /* XXX */
	else
		p = ary[(int)w[1]-'0']->n_left;
	if (optype(p->n_op) != LTYPE)
		comperr("bad xarg op %d", p->n_op);
	q = tcopy(p);
	if (q->n_op == REG) {
		if (*w == 'k') {
			q->n_type = INT;
		} else if (*w != 'w') {
			if (q->n_type > UCHAR) {
				regno(q) = regno(q)*2+8;
				if (*w == 'h')
					regno(q)++;
			}
			q->n_type = INT;
		} else
			q->n_type = SHORT;
	}
	adrput(stdout, q);
	tfree(q);
}

/*
 * target-specific conversion of numeric arguments.
 */
int
numconv(void *ip, void *p1, void *q1)
{
	NODE *p = p1, *q = q1;
	int cw = xasmcode(q->n_name);

	switch (XASMVAL(cw)) {
	case 'a':
	case 'b':
	case 'c':
	case 'd':
		p->n_name = tmpcalloc(2);
		p->n_name[0] = (char)XASMVAL(cw);
		return 1;
	default:
		return 0;
	}
}

static struct {
	char *name; int num;
} xcr[] = {
	{ "ax", AX },
	{ "bx", BX },
	{ "cx", CX },
	{ "dx", DX },
	{ "si", SI },
	{ "di", DI },
	{ NULL, 0 },
};

/*
 * Check for other names of the xasm constraints registers.
 */

/*
 * Check for other names of the xasm constraints registers.
 */
int xasmconstregs(char *s)
{
	int i;

	if (strncmp(s, "st", 2) == 0) {
		int off =0;
		if (s[2] == '(' && s[4] == ')')
			off = s[3] - '0';
		return SIDI + 1 + off;
	}

	for (i = 0; xcr[i].name; i++)
		if (strcmp(xcr[i].name, s) == 0)
			return xcr[i].num;
	return -1;
}

