/*	Id: local2.c,v 1.6 2014/04/08 19:51:31 ragge Exp 	*/	
/*	$NetBSD: local2.c,v 1.1.1.1 2014/07/24 19:21:25 plunky Exp $	*/
/*
 * Copyright (c) 2014 Anders Magnusson (ragge@ludd.luth.se).
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

static int stkpos;

void
deflab(int label)
{
	printf(LABFMT ":\n", label);
}

static int regm, regf, fpsub, nfp;

void
prologue(struct interpass_prolog *ipp)
{
	int i;

	/*
	 * Subtract both space for automatics and permanent regs.
	 * XXX - no struct return yet.
	 */

	fpsub = p2maxautooff;
	if (fpsub >= AUTOINIT/SZCHAR)
		fpsub -= AUTOINIT/SZCHAR;
	regm = regf = nfp = 0;
	for (i = 0; i < MAXREGS; i++)
		if (TESTBIT(ipp->ipp_regs, i)) {
			if (i <= A7) {
				regm |= (1 << i);
				fpsub += 4;
			} else if (i >= FP0) {
				regf |= (1 << (i - FP0));
				fpsub += 12;
				nfp += 12;
			} else
				comperr("bad reg range");
		}
	printf("	link.%c %%fp,#%d\n", fpsub > 32768 ? 'l' : 'w', -fpsub);
	if (regm)
		printf("	movem.l #%d,%d(%%fp)\n", regm, -fpsub + nfp);
	if (regf)
		printf("	fmovem #%d,%d(%%fp)\n", regf, -fpsub);
}

void
eoftn(struct interpass_prolog *ipp)
{
	if (ipp->ipp_ip.ip_lbl == 0)
		return; /* no code needs to be generated */

	if (regm)
		printf("	movem.l %d(%%fp),#%d\n", -fpsub + nfp, regm);
	if (regf)
		printf("	fmovem %d(%%fp),#%d\n", -fpsub, regf);
	printf("	unlk %%fp\n	rts\n");
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
		str = "eor";
		break;
	default:
		comperr("hopcode2: %d", o);
		str = 0; /* XXX gcc */
	}
	printf("%s", str);
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
			return(SZSHORT/SZCHAR);

		case DOUBLE:
			return(SZDOUBLE/SZCHAR);

		case INT:
		case UNSIGNED:
			return(SZINT/SZCHAR);

		case LONG:
		case ULONG:
		case LONGLONG:
		case ULONGLONG:
			return SZLONGLONG/SZCHAR;

		default:
			if (!ISPTR(p->n_type))
				comperr("tlen type %d not pointer");
			return SZPOINT(p->n_type)/SZCHAR;
		}
}

int
fldexpand(NODE *p, int cookie, char **cp)
{
	comperr("fldexpand");
	return 0;
}

static void
starg(NODE *p)
{
	int sz = p->n_stsize;
	int subsz = (p->n_stsize + 3) & ~3;
	int fr, tr, cr;

	fr = regno(getlr(p, 'L')); /* from reg (struct pointer) */
	cr = regno(getlr(p, '1')); /* count reg (number of words) */
	tr = regno(getlr(p, '2')); /* to reg (stack) */

	/* Sub from stack and put in toreg */
	printf("	sub.l #%d,%%sp\n", subsz);
	printf("	move.l %%sp,%s\n", rnames[tr]);

	/* Gen an even copy start */
	if (sz & 1)
		expand(p, INBREG, "	move.b (A2)+,(AL)+\n");
	if (sz & 2)
		expand(p, INBREG, "	move.w (A2)+,(AL)+\n");
	sz -= (sz & ~3);
	
	/* if more than 4 words, use loop, otherwise output instructions */
	if (sz > 16) {
		printf("	move.l #%d,%s\n", sz/4, rnames[cr]);
		expand(p, INBREG, "1:	move.l (A2)+,(AL)+\n");
		expand(p, INBREG, "	dec.l A1\n");
		expand(p, INBREG, "	jne 1b\n");
	} else {
		if (sz > 12)
			expand(p, INBREG, "	move.l (A2)+,(AL)+\n"), sz -= 4;
		if (sz > 8)
			expand(p, INBREG, "	move.l (A2)+,(AL)+\n"), sz -= 4;
		if (sz > 4)
			expand(p, INBREG, "	move.l (A2)+,(AL)+\n"), sz -= 4;
		if (sz == 4)
			expand(p, INBREG, "	move.l (A2)+,(AL)+\n");
	}
}

void
zzzcode(NODE *p, int c)
{
	TWORD t = p->n_type;
	char *s;

	switch (c) {
	case 'L':
		t = p->n_left->n_type;
		/* FALLTHROUGH */
	case 'A':
		s = (t == CHAR || t == UCHAR ? "b" :
		    t == SHORT || t == USHORT ? "w" : 
		    t == FLOAT ? "s" :
		    t == DOUBLE ? "d" :
		    t == LDOUBLE ? "x" : "l");
		printf("%s", s);
		break;

	case 'B':
		if (p->n_qual)
			printf("	add.l #%d,%%sp\n", (int)p->n_qual);
		break;

	case 'F': /* Emit float branches */
		switch (p->n_op) {
		case GT: s = "fjnle"; break;
		case GE: s = "fjnlt"; break;
		case LE: s = "fjngt"; break;
		case LT: s = "fjnge"; break;
		case NE: s = "fjne"; break;
		case EQ: s = "fjeq"; break;
		default: comperr("ZF"); s = 0;
		}
		printf("%s " LABFMT "\n", s, p->n_label);
		break;

	case 'P':
		printf("	lea -%d(%%fp),%%a0\n", stkpos);
		break;

	case 'Q': /* struct assign */
		printf("	move.l %d,-(%%sp)\n", p->n_stsize);
		expand(p, INAREG, "	move.l AR,-(%sp)\n");
		expand(p, INAREG, "	move.l AL,-(%sp)\n");
		printf("	jsr memcpy\n");
		printf("	add.l #12,%%sp\n");
		break;

	case 'S': /* struct arg */
		starg(p);
		break;

	case '2':
		if (regno(getlr(p, '2')) != regno(getlr(p, 'L')))
			expand(p, INAREG, "	fmove.x AL,A2\n");
		break;

	default:
		comperr("zzzcode %c", c);
	}
}

#if 0
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
#endif

/*
 * Does the bitfield shape match?
 */
int
flshape(NODE *p)
{
	comperr("flshape");
	return(0);
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
	printf("#" CONFMT, val);
}

void
conput(FILE *fp, NODE *p)
{
	long val = p->n_lval;

	if (p->n_type <= UCHAR)
		val &= 255;
	else if (p->n_type <= USHORT)
		val &= 65535;

	switch (p->n_op) {
	case ICON:
		if (p->n_name[0] != '\0') {
			fprintf(fp, "%s", p->n_name);
			if (val)
				fprintf(fp, "+%ld", val);
		} else
			fprintf(fp, "%ld", val);
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
	switch (p->n_op) {
	case REG:
		printf("%%%s", &rnames[p->n_rval][2]);
		break;
	case NAME:
	case OREG:
		p->n_lval += 4;
		adrput(stdout, p);
		p->n_lval -= 4;
		break;

	case ICON:
		printf("#%d", (int)p->n_lval);
		break;

	default:
		comperr("upput bad op %d size %d", p->n_op, size);
	}
}

void
adrput(FILE *io, NODE *p)
{
	int r;

	/* output an address, with offsets, from p */
	switch (p->n_op) {
	case NAME:
		if (p->n_name[0] != '\0') {
			if (p->n_lval != 0)
				fprintf(io, CONFMT "+", p->n_lval);
			fprintf(io, "%s", p->n_name);
		} else {
			comperr("adrput");
			fprintf(io, CONFMT, p->n_lval);
		}
		return;

	case OREG:
		r = p->n_rval;
		if (p->n_name[0])
			printf("%s%s", p->n_name, p->n_lval ? "+" : "");
		if (p->n_lval)
			fprintf(io, CONFMT, p->n_lval);
		if (R2TEST(r)) {
			int r1 = R2UPK1(r);
			int r2 = R2UPK2(r);
			int sh = R2UPK3(r);

			fprintf(io, "(%s,%s,%d)", 
			    r1 == MAXREGS ? "" : rnames[r1],
			    r2 == MAXREGS ? "" : rnames[r2], sh);
		} else
			fprintf(io, "(%s)", rnames[p->n_rval]);
		return;
	case ICON:
		/* addressable value of the constant */
		if (p->n_type == LONGLONG || p->n_type == ULONGLONG) {
			fprintf(io, "#" CONFMT, p->n_lval >> 32);
		} else {
			fputc('#', io);
			conput(io, p);
		}
		return;

	case REG:
		if ((p->n_type == LONGLONG || p->n_type == ULONGLONG) &&
			/* XXX allocated reg may get wrong type here */
		    (p->n_rval > A7 && p->n_rval < FP0)) {
			fprintf(io, "%%%c%c", rnames[p->n_rval][0],
			    rnames[p->n_rval][1]);
		} else
			fprintf(io, "%s", rnames[p->n_rval]);
		return;

	default:
		comperr("illegal address, op %d, node %p", p->n_op, p);
		return;

	}
}

static char *
ccbranches[] = {
	"jeq",		/* jumpe */
	"jne",		/* jumpn */
	"jle",		/* jumple */
	"jlt",		/* jumpl */
	"jge",		/* jumpge */
	"jgt",		/* jumpg */
	"jls",		/* jumple (jlequ) */
	"jcs",		/* jumpl (jlssu) */
	"jcc",		/* jumpge (jgequ) */
	"jhi",		/* jumpg (jgtru) */
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
mkcall(NODE *p, char *name)
{
	p->n_op = CALL;
	p->n_right = mkunode(FUNARG, p->n_left, 0, p->n_left->n_type);
	p->n_left = mklnode(ICON, 0, 0, FTN|p->n_type);
	p->n_left->n_name = name;
}

static void
mkcall2(NODE *p, char *name)
{
	p->n_op = CALL;
	p->n_right = mkunode(FUNARG, p->n_right, 0, p->n_right->n_type);
	p->n_left = mkunode(FUNARG, p->n_left, 0, p->n_left->n_type);
	p->n_right = mkbinode(CM, p->n_left, p->n_right, INT);
	p->n_left = mklnode(ICON, 0, 0, FTN|p->n_type);
	p->n_left->n_name = name;
}


static void
fixcalls(NODE *p, void *arg)
{
	TWORD lt;

	switch (p->n_op) {
	case STCALL:
	case USTCALL:
		if (p->n_stsize+p2autooff > stkpos)
			stkpos = p->n_stsize+p2autooff;
		break;

	case DIV:
		if (p->n_type == LONGLONG)
			mkcall2(p, "__divdi3");
		else if (p->n_type == ULONGLONG)
			mkcall2(p, "__udivdi3");
		break;

	case MOD:
		if (p->n_type == LONGLONG)
			mkcall2(p, "__moddi3");
		else if (p->n_type == ULONGLONG)
			mkcall2(p, "__umoddi3");
		break;

	case MUL:
		if (p->n_type == LONGLONG || p->n_type == ULONGLONG)
			mkcall2(p, "__muldi3");
		break;

	case LS:
		if (p->n_type == LONGLONG || p->n_type == ULONGLONG)
			mkcall2(p, "__ashldi3");
		break;

	case RS:
		if (p->n_type == LONGLONG)
			mkcall2(p, "__ashrdi3");
		else if (p->n_type == ULONGLONG)
			mkcall2(p, "__lshrdi3");
		break;

	case SCONV:
		lt = p->n_left->n_type;
		switch (p->n_type) {
		case LONGLONG:
			if (lt == FLOAT)
				mkcall(p, "__fixsfdi");
			else if (lt == DOUBLE)
				mkcall(p, "__fixdfdi");
			else if (lt == LDOUBLE)
				mkcall(p, "__fixxfdi");
			break;
		case ULONGLONG:
			if (lt == FLOAT)
				mkcall(p, "__fixunssfdi");
			else if (lt == DOUBLE)
				mkcall(p, "__fixunsdfdi");
			else if (lt == LDOUBLE)
				mkcall(p, "__fixunsxfdi");
			break;
		case FLOAT:
			if (lt == LONGLONG)
				mkcall(p, "__floatdisf");
			else if (lt == ULONGLONG)
				mkcall(p, "__floatundisf");
			break;
		case DOUBLE:
			if (lt == LONGLONG)
				mkcall(p, "__floatdidf");
			else if (lt == ULONGLONG)
				mkcall(p, "__floatundidf");
			break;
		case LDOUBLE:
			if (lt == LONGLONG)
				mkcall(p, "__floatdixf");
			else if (lt == ULONGLONG)
				mkcall(p, "__floatundixf");
			break;
		}
		break;
#if 0
	case XASM:
		p->n_name = adjustname(p->n_name);
		break;
#endif
	}
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

void
rmove(int s, int d, TWORD t)
{

	if (t == LONGLONG || t == ULONGLONG) {
		printf("	move.l %s,%s\n",
		    rnames[s-D0D1], rnames[d-D0D1]);
		printf("	move.l %s,%s\n",
		    rnames[s+1-D0D1], rnames[d+1-D0D1]);
	} else if (t >= FLOAT && t <= TDOUBLE)
		printf("	fmove.x %s,%s\n", rnames[s], rnames[d]);
	else
		printf("	move.l %s,%s\n", rnames[s], rnames[d]);
}

/*
 * For class cc, find worst-case displacement of the number of
 * registers in the array r[] indexed by class.
 */
int
COLORMAP(int cc, int *r)
{
	int a,c;

	a = r[CLASSA];
	c = r[CLASSC];

	switch (cc) {
	case CLASSA:
		if (c * 2 + a < 8)
			return 1;
		break;
	case CLASSB:
		return r[CLASSB] < 6;
	case CLASSC:
		if (c > 2)
			return 0;
		if (c == 2 && a > 0)
			return 0;
		if (c == 1 && a > 1)
			return 0;
		if (c == 0 && a > 3)
			return 0;
		return 1;
	}
	return 0;
}

char *rnames[] = {
	"%d0", "%d1", "%d2", "%d3", "%d4", "%d5", "%d6", "%d7",
	"%a0", "%a1", "%a2", "%a3", "%a4", "%a5", "%a6", "%a7",
	"d0d1", "d1d2", "d2d3", "d3d4", "d4d5", "d5d6", "d6d7",
	"%fp0", "%fp1", "%fp2", "%fp3", "%fp4", "%fp5", "%fp6", "%fp7", 
};

/*
 * Return a class suitable for a specific type.
 */
int
gclass(TWORD t)
{
	if (t > BTMASK)
		return CLASSB;
	if (t == LONGLONG || t == ULONGLONG)
		return CLASSC;
	if (t == FLOAT || t == DOUBLE || t == LDOUBLE)
		return CLASSD;
	return CLASSA;
}

static int
argsiz(NODE *p)
{
	TWORD t = p->n_type;

	if (t < LONGLONG || t == FLOAT || t > BTMASK)
		return 4;
	if (t == LONGLONG || t == ULONGLONG || t == DOUBLE)
		return 8;
	if (t == LDOUBLE)
		return 12;
	if (t == STRTY || t == UNIONTY)
		return (p->n_stsize+3) & ~3;
	comperr("argsiz");
	return 0;
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
	for (p = p->n_right; p->n_op == CM; p = p->n_left)
		size += argsiz(p->n_right);
	size += argsiz(p);
	op->n_qual = size; /* XXX */
}

/*
 * Special shapes.
 */
int
special(NODE *p, int shape)
{
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
	int cw = xasmcode(p->n_name);
	int ww;
	char *w;

	switch (ww = XASMVAL(cw)) {
	case 'd': /* Just convert to reg */
	case 'a':
		p->n_name = tmpstrdup(p->n_name);
		w = strchr(p->n_name, XASMVAL(cw));
		*w = 'r'; /* now reg */
		break;
	}
	return 0;
}
