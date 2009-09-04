/*	$Id: local2.c,v 1.1.1.1 2009/09/04 00:27:30 gmcgarry Exp $	*/
/*
 * Copyright (c) 2008 Michael Shalayeff
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

static int stkpos;

void
deflab(int label)
{
	printf(LABFMT ":\n", label);
}

static int regoff[MAXREGS];
static TWORD ftype;
char *rbyte[], *rshort[], *rlong[];

/*
 * Print out the prolog assembler.
 * addto and regoff are already calculated.
 */
static void
prtprolog(struct interpass_prolog *ipp, int addto)
{
	int i;

	/* XXX should look if there is any need to emit this */
	printf("\tpushq %%rbp\n");
	printf("\tmovq %%rsp,%%rbp\n");
	addto = (addto+15) & ~15; /* 16-byte aligned */
	if (addto)
		printf("\tsubq $%d,%%rsp\n", addto);

	/* save permanent registers */
	for (i = 0; i < MAXREGS; i++)
		if (TESTBIT(ipp->ipp_regs, i))
			fprintf(stdout, "\tmovq %s,-%d(%s)\n",
			    rnames[i], regoff[i], rnames[FPREG]);
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
			addto += SZLONG/SZCHAR;
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
	printf("	.align 16\n");
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
			fprintf(stdout, "	movq -%d(%s),%s\n",
			    regoff[i], rnames[FPREG], rnames[i]);

	/* struct return needs special treatment */
	if (ftype == STRTY || ftype == UNIONTY) {
		printf("	movl 8(%%ebp),%%eax\n");
		printf("	leave\n");
		printf("	ret $%d\n", 4);
	} else {
		printf("	leave\n");
		printf("	ret\n");
	}
	printf("\t.size %s,.-%s\n", ipp->ipp_name, ipp->ipp_name);
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
	printf("%s%c", str, f);
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

#if 0
/*
 * Emit code to compare two longlong numbers.
 */
static void
twollcomp(NODE *p)
{
	int o = p->n_op;
	int s = getlab2();
	int e = p->n_label;
	int cb1, cb2;

	if (o >= ULE)
		o -= (ULE-LE);
	switch (o) {
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
		cb1 = GT;
		cb2 = LT;
		break;
	case GE:
	case GT:
		cb1 = LT;
		cb2 = GT;
		break;
	
	default:
		cb1 = cb2 = 0; /* XXX gcc */
	}
	if (p->n_op >= ULE)
		cb1 += 4, cb2 += 4;
	expand(p, 0, "	cmpl UR,UL\n");
	if (cb1) cbgen(cb1, s);
	if (cb2) cbgen(cb2, e);
	expand(p, 0, "	cmpl AR,AL\n");
	cbgen(p->n_op, e);
	deflab(s);
}
#endif

int
fldexpand(NODE *p, int cookie, char **cp)
{
	CONSZ val;

	if (p->n_op == ASSIGN)
		p = p->n_left;
	switch (**cp) {
	case 'S':
		printf("%d", UPKFSZ(p->n_rval));
		break;
	case 'H':
		printf("%d", UPKFOFF(p->n_rval));
		break;
	case 'M':
	case 'N':
		val = (CONSZ)1 << UPKFSZ(p->n_rval);
		--val;
		val <<= UPKFOFF(p->n_rval);
		if (p->n_type > UNSIGNED)
			printf("0x%llx", (**cp == 'M' ? val : ~val));
		else
			printf("0x%llx", (**cp == 'M' ? val : ~val)&0xffffffff);
		break;
	default:
		comperr("fldexpand");
	}
	return 1;
}

static void
bfext(NODE *p)
{
	int ch = 0, sz = 0;

	if (ISUNSIGNED(p->n_right->n_type))
		return;
	switch (p->n_right->n_type) {
	case CHAR:
		ch = 'b';
		sz = 8;
		break;
	case SHORT:
		ch = 'w';
		sz = 16;
		break;
	case INT:
		ch = 'l';
		sz = 32;
		break;
	case LONG:
		ch = 'q';
		sz = 64;
		break;
	default:
		comperr("bfext");
	}

	sz -= UPKFSZ(p->n_left->n_rval);
	printf("\tshl%c $%d,", ch, sz);
	adrput(stdout, getlr(p, 'D'));
	printf("\n\tsar%c $%d,", ch, sz);
	adrput(stdout, getlr(p, 'D'));
	printf("\n");
}
#if 0

/*
 * Push a structure on stack as argument.
 * the scratch registers are already free here
 */
static void
starg(NODE *p)
{
	FILE *fp = stdout;

	fprintf(fp, "	subl $%d,%%esp\n", p->n_stsize);
	fprintf(fp, "	pushl $%d\n", p->n_stsize);
	expand(p, 0, "	pushl AL\n");
	expand(p, 0, "	leal 8(%esp),A1\n");
	expand(p, 0, "	pushl A1\n");
	fprintf(fp, "	call memcpy\n");
	fprintf(fp, "	addl $12,%%esp\n");
}

#endif
/*
 * Generate code to convert an unsigned long to xmm float/double.
 */
static void
ultofd(NODE *p)
{

#define	E(x)	expand(p, 0, x)
	E("	movq AL,A1\n");
	E("	testq A1,A1\n");
	E("	js 2f\n");
	E("	cvtsi2sZfq A1,A3\n");
	E("	jmp 3f\n");
	E("2:\n");
	E("	movq A1,A2\n");
	E("	shrq A2\n");
	E("	andq $1,A1\n");
	E("	orq A1,A2\n");
	E("	cvtsi2sZfq A2,A3\n");
	E("	addsZf A3,A3\n");
	E("3:\n");
#undef E
}

static int
argsiz(NODE *p)
{
	TWORD t = p->n_type;

	if (p->n_left->n_op == REG)
		return 0; /* not on stack */
	if (t == LDOUBLE)
		return 16;
	if (t == STRTY || t == UNIONTY)
		return p->n_stsize;
	return 8;
}

void
zzzcode(NODE *p, int c)
{
	NODE *l;
	int pr, lr, s;
	char **rt;

	switch (c) {
#if 0
	case 'A': /* swap st0 and st1 if right is evaluated second */
		if ((p->n_su & DORIGHT) == 0) {
			if (logop(p->n_op))
				printf("	fxch\n");
			else
				printf("r");
		}
		break;
#endif

	case 'C':  /* remove from stack after subroutine call */
		pr = p->n_qual;
		if (p->n_op == STCALL || p->n_op == USTCALL)
			pr += 4; /* XXX */
		if (p->n_op == UCALL)
			return; /* XXX remove ZC from UCALL */
		if (pr)
			printf("	addq $%d, %s\n", pr, rnames[RSP]);
		break;

	case 'E': /* Perform bitfield sign-extension */
		bfext(p);
		break;

#if 0
	case 'F': /* Structure argument */
		if (p->n_stalign != 0) /* already on stack */
			starg(p);
		break;
#endif
	case 'j': /* convert unsigned long to f/d */
		ultofd(p);
		break;

	case 'M': /* Output sconv move, if needed */
		l = getlr(p, 'L');
		/* XXX fixneed: regnum */
		pr = DECRA(p->n_reg, 0);
		lr = DECRA(l->n_reg, 0);
		if (pr == lr)
			break;
		printf("	movb %s,%s\n", rbyte[lr], rbyte[pr]);
		l->n_rval = l->n_reg = p->n_reg; /* XXX - not pretty */
		break;

#if 0
	case 'N': /* output extended reg name */
		printf("%s", rnames[getlr(p, '1')->n_rval]);
		break;

	case 'S': /* emit eventual move after cast from longlong */
		pr = DECRA(p->n_reg, 0);
		lr = p->n_left->n_rval;
		switch (p->n_type) {
		case CHAR:
		case UCHAR:
			if (rnames[pr][2] == 'l' && rnames[lr][2] == 'x' &&
			    rnames[pr][1] == rnames[lr][1])
				break;
			if (rnames[lr][2] == 'x') {
				printf("\tmovb %%%cl,%s\n",
				    rnames[lr][1], rnames[pr]);
				break;
			}
			/* Must go via stack */
			s = BITOOR(freetemp(1));
			printf("\tmovl %%e%ci,%d(%%rbp)\n", rnames[lr][1], s);
			printf("\tmovb %d(%%rbp),%s\n", s, rnames[pr]);
			comperr("SCONV1 %s->%s", rnames[lr], rnames[pr]);
			break;

		case SHORT:
		case USHORT:
			if (rnames[lr][1] == rnames[pr][2] &&
			    rnames[lr][2] == rnames[pr][3])
				break;
			printf("\tmovw %%%c%c,%%%s\n",
			    rnames[lr][1], rnames[lr][2], rnames[pr]+2);
			comperr("SCONV2 %s->%s", rnames[lr], rnames[pr]);
			break;
		case INT:
		case UNSIGNED:
			if (rnames[lr][1] == rnames[pr][2] &&
			    rnames[lr][2] == rnames[pr][3])
				break;
			printf("\tmovl %%e%c%c,%s\n",
				    rnames[lr][1], rnames[lr][2], rnames[pr]);
			comperr("SCONV3 %s->%s", rnames[lr], rnames[pr]);
			break;

		default:
			if (rnames[lr][1] == rnames[pr][2] &&
			    rnames[lr][2] == rnames[pr][3])
				break;
			comperr("SCONV4 %s->%s", rnames[lr], rnames[pr]);
			break;
		}
		break;
#endif
        case 'Q': /* emit struct assign */
		/* XXX - optimize for small structs */
		printf("\tmovq $%d,%%rdx\n", p->n_stsize);
		expand(p, INAREG, "\tmovq AR,%rsi\n");
		expand(p, INAREG, "\tleaq AL,%rdi\n\n");
		printf("\tcall memcpy\n");
		break;

	case 'R': /* print opname based on right type */
	case 'L': /* print opname based on left type */
		switch (getlr(p, c)->n_type) {
		case CHAR: case UCHAR: s = 'b'; break;
		case SHORT: case USHORT: s = 'w'; break;
		case INT: case UNSIGNED: s = 'l'; break;
		default: s = 'q'; break;
		printf("%c", s);
		}
		break;

	case '8': /* special reg name printout (64-bit) */
	case '1': /* special reg name printout (32-bit) */
		l = getlr(p, '1');
		rt = c == '8' ? rnames : rlong;
		printf("%s", rt[l->n_rval]);
		break;

	case 'g':
		p = p->n_left;
		/* FALLTHROUGH */
	case 'f': /* float or double */
		printf("%c", p->n_type == FLOAT ? 's' : 'd');
		break;

	case 'q': /* int or long */
		printf("%c", p->n_left->n_type == LONG ? 'q' : ' ');
		break;

	default:
		comperr("zzzcode %c", c);
	}
}

/*ARGSUSED*/
int
rewfld(NODE *p)
{
	return(1);
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
	int o = p->n_op;

	if (o == OREG || o == REG || o == NAME)
		return SRDIR; /* Direct match */
	if (o == UMUL && shumul(p->n_left, SOREG))
		return SROREG; /* Convert into oreg */
	return SRREG; /* put it into a register */
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
	int val = p->n_lval;

	switch (p->n_op) {
	case ICON:
		if (p->n_name[0] != '\0') {
			fprintf(fp, "%s", p->n_name);
			if (val)
				fprintf(fp, "+%d", val);
		} else
			fprintf(fp, "%d", val);
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
		fprintf(stdout, "%%%s", &rnames[p->n_rval][3]);
		break;

	case NAME:
	case OREG:
		p->n_lval += size;
		adrput(stdout, p);
		p->n_lval -= size;
		break;
	case ICON:
		fprintf(stdout, "$" CONFMT, p->n_lval >> 32);
		break;
	default:
		comperr("upput bad op %d size %d", p->n_op, size);
	}
}

void
adrput(FILE *io, NODE *p)
{
	int r;
	char **rc;
	/* output an address, with offsets, from p */

	if (p->n_op == FLD)
		p = p->n_left;

	switch (p->n_op) {

	case NAME:
		if (p->n_name[0] != '\0') {
			fputs(p->n_name, io);
			if (p->n_lval != 0)
				fprintf(io, "+" CONFMT, p->n_lval);
		} else
			fprintf(io, CONFMT, p->n_lval);
		return;

	case OREG:
		r = p->n_rval;
		if (p->n_name[0])
			printf("%s%s", p->n_name, p->n_lval ? "+" : "");
		if (p->n_lval)
			fprintf(io, "%d", (int)p->n_lval);
		if (R2TEST(r)) {
			fprintf(io, "(%s,%s,8)", rnames[R2UPK1(r)],
			    rnames[R2UPK2(r)]);
		} else
			fprintf(io, "(%s)", rnames[p->n_rval]);
		return;
	case ICON:
#ifdef PCC_DEBUG
		/* Sanitycheck for PIC, to catch adressable constants */
		if (kflag && p->n_name[0]) {
			static int foo;

			if (foo++ == 0) {
				printf("\nfailing...\n");
				fwalk(p, e2print, 0);
				comperr("pass2 conput");
			}
		}
#endif
		/* addressable value of the constant */
		fputc('$', io);
		conput(io, p);
		return;

	case REG:
		switch (p->n_type) {
		case CHAR:
		case UCHAR:
			rc = rbyte;
			break;
		case SHORT:
		case USHORT:
			rc = rshort;
			break;
		case INT:
		case UNSIGNED:
			rc = rlong;
			break;
		default:
			rc = rnames;
			break;
		}
		fprintf(io, "%s", rc[p->n_rval]);
		return;

	default:
		comperr("illegal address, op %d, node %p", p->n_op, p);
		return;

	}
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
		if (p->n_stsize+p2autooff > stkpos)
			stkpos = p->n_stsize+p2autooff;
		break;
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

	switch (t) {
	case INT:
	case UNSIGNED:
		printf("	movl %s,%s\n", rlong[s], rlong[d]);
		break;
	case CHAR:
	case UCHAR:
		printf("	movb %s,%s\n", rbyte[s], rbyte[d]);
		break;
	case SHORT:
	case USHORT:
		printf("	movw %s,%s\n", rshort[s], rshort[d]);
		break;
	case FLOAT:
		printf("	movss %s,%s\n", rnames[s], rnames[d]);
		break;
	case DOUBLE:
		printf("	movsd %s,%s\n", rnames[s], rnames[d]);
		break;
	case LDOUBLE:
		/* a=b()*c(); will generate this */
		comperr("bad float rmove: %d %d", s, d);
		break;
	default:
		printf("	movq %s,%s\n", rnames[s], rnames[d]);
		break;
	}
}

/*
 * For class c, find worst-case displacement of the number of
 * registers in the array r[] indexed by class.
 */
int
COLORMAP(int c, int *r)
{

	switch (c) {
	case CLASSA:
		return r[CLASSA] < 14;
	case CLASSB:
		return r[CLASSB] < 16;
	}
	return 0; /* XXX gcc */
}

char *rnames[] = {
	"%rax", "%rdx", "%rcx", "%rbx", "%rsi", "%rdi", "%rbp", "%rsp",
	"%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15",
	"%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7",
	"%xmm8", "%xmm9", "%xmm10", "%xmm11", "%xmm12", "%xmm13", "%xmm14",
	"%xmm15",
};

/* register names for shorter sizes */
char *rbyte[] = {
	"%al", "%dl", "%cl", "%bl", "%sil", "%dil", "%bpl", "%spl",
	"%r8b", "%r9b", "%r10b", "%r11b", "%r12b", "%r13b", "%r14b", "%r15b", 
};
char *rshort[] = {
	"%ax", "%dx", "%cx", "%bx", "%si", "%di", "%bp", "%sp",
	"%r8w", "%r9w", "%r10w", "%r11w", "%r12w", "%r13w", "%r14w", "%r15w", 
};
char *rlong[] = {
	"%eax", "%edx", "%ecx", "%ebx", "%esi", "%edi", "%ebp", "%esp",
	"%r8d", "%r9d", "%r10d", "%r11d", "%r12d", "%r13d", "%r14d", "%r15d", 
};


/*
 * Return a class suitable for a specific type.
 */
int
gclass(TWORD t)
{
	if (t == FLOAT || t == DOUBLE || t == LDOUBLE)
		return CLASSB;
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
	for (p = p->n_right; p->n_op == CM; p = p->n_left)
		size += argsiz(p->n_right);
	size += argsiz(p);
	size = (size+15) & ~15;
	if (size)
		printf("	subq $%d,%s\n", size, rnames[RSP]);
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
	case SCON32:
		if (o != ICON || p->n_name[0])
			break;
		if (p->n_lval < MIN_INT || p->n_lval > MAX_INT)
			break;
		return SRDIR;
	default:
		cerror("special: %x\n", shape);
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
	return 0;
#if 0
	struct interpass *ip2;
	NODE *in = 0, *ut = 0;
	TWORD t;
	char *w;
	int reg;
	int cw;

	cw = xasmcode(p->n_name);
	if (cw & (XASMASG|XASMINOUT))
		ut = p->n_left;
	if ((cw & XASMASG) == 0)
		in = p->n_left;

	switch (XASMVAL(cw)) {
	case 'D': reg = EDI; break;
	case 'S': reg = ESI; break;
	case 'a': reg = EAX; break;
	case 'b': reg = EBX; break;
	case 'c': reg = ECX; break;
	case 'd': reg = EDX; break;
	case 't': reg = 0; break;
	case 'u': reg = 1; break;
	case 'A': reg = EAXEDX; break;
	case 'q': /* XXX let it be CLASSA as for now */
		p->n_name = tmpstrdup(p->n_name);
		w = strchr(p->n_name, 'q');
		*w = 'r';
		return 0;
	default:
		return 0;
	}
	p->n_name = tmpstrdup(p->n_name);
	for (w = p->n_name; *w; w++)
		;
	w[-1] = 'r'; /* now reg */
	t = p->n_left->n_type;
	if (reg == EAXEDX) {
		p->n_label = CLASSC;
	} else {
		p->n_label = CLASSA;
		if (t == CHAR || t == UCHAR) {
			p->n_label = CLASSB;
			reg = reg * 2 + 8;
		}
	}
	if (t == FLOAT || t == DOUBLE || t == LDOUBLE) {
		p->n_label = CLASSD;
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
#endif
	return 1;
}

void
targarg(char *w, void *arg)
{
cerror("targarg");
#if 0
	NODE **ary = arg;
	NODE *p, *q;

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
#endif
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
		p->n_name[0] = XASMVAL(cw);
		return 1;
	default:
		return 0;
	}
}
