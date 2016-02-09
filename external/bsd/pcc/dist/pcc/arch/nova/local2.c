/*	Id: local2.c,v 1.13 2015/03/28 08:28:46 ragge Exp 	*/	
/*	$NetBSD: local2.c,v 1.1.1.5 2016/02/09 20:28:24 plunky Exp $	*/
/*
 * Copyright (c) 2006 Anders Magnusson (ragge@ludd.luth.se).
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

void acon(NODE *p);
int argsize(NODE *p);

void
deflab(int label)
{
	printf(LABFMT ":\n", label);
}

void
prologue(struct interpass_prolog *ipp)
{
	int totstk;

	totstk = p2maxautooff/(SZINT/SZCHAR);

	if (totstk)
		printf("	.word 0%o\n", totstk);
	printf("%s:\n", ipp->ipp_name);
	if (ipp->ipp_vis)
		printf("	.globl %s\n", ipp->ipp_name);
	printf("	mov 3,0\n");	/* put ret pc in ac0 */
	printf("	jsr @16\n");	/* jump to prolog */
}

void
eoftn(struct interpass_prolog *ipp)
{

	if (ipp->ipp_ip.ip_lbl == 0)
		return; /* no code needs to be generated */

	printf("	jmp @17\n");
}

/*
 * add/sub/...
 *
 * Param given:
 */
void
hopcode(int f, int o)
{
	char *str = 0;

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
		cerror("hopcode OR");
		break;
	case ER:
		cerror("hopcode xor");
		str = "xor";
		break;
	default:
		comperr("hopcode2: %d", o);
		str = 0; /* XXX gcc */
	}
	printf("%s%c", str, f);
}

#if 0
/*
 * Return type size in bytes.  Used by R2REGS, arg 2 to offset().
 */
int
tlen(p) NODE *p;
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
		case LONG:
		case ULONG:
			return(SZINT/SZCHAR);

		case LONGLONG:
		case ULONGLONG:
			return SZLONGLONG/SZCHAR;

		default:
			if (!ISPTR(p->n_type))
				comperr("tlen type %d not pointer");
			return SZPOINT(p->n_type)/SZCHAR;
		}
}
#endif

int
fldexpand(NODE *p, int cookie, char **cp)
{
	return 0;
}

#if 0
/*
 * Assign to a bitfield.
 * Clumsy at least, but what to do?
 */
static void
bfasg(NODE *p)
{
	NODE *fn = p->n_left;
	int shift = UPKFOFF(fn->n_rval);
	int fsz = UPKFSZ(fn->n_rval);
	int andval, tch = 0;

	/* get instruction size */
	switch (p->n_type) {
	case CHAR: case UCHAR: tch = 'b'; break;
	case SHORT: case USHORT: tch = 'w'; break;
	case INT: case UNSIGNED: tch = 'l'; break;
	default: comperr("bfasg");
	}

	/* put src into a temporary reg */
	printf("	mov%c ", tch);
	adrput(stdout, getlr(p, 'R'));
	printf(",");
	adrput(stdout, getlr(p, '1'));
	printf("\n");

	/* AND away the bits from dest */
	andval = ~(((1 << fsz) - 1) << shift);
	printf("	and%c $%d,", tch, andval);
	adrput(stdout, fn->n_left);
	printf("\n");

	/* AND away unwanted bits from src */
	andval = ((1 << fsz) - 1);
	printf("	and%c $%d,", tch, andval);
	adrput(stdout, getlr(p, '1'));
	printf("\n");

	/* SHIFT left src number of bits */
	if (shift) {
		printf("	sal%c $%d,", tch, shift);
		adrput(stdout, getlr(p, '1'));
		printf("\n");
	}

	/* OR in src to dest */
	printf("	or%c ", tch);
	adrput(stdout, getlr(p, '1'));
	printf(",");
	adrput(stdout, fn->n_left);
	printf("\n");
}
#endif

#if 0
/*
 * Push a structure on stack as argument.
 * the scratch registers are already free here
 */
static void
starg(NODE *p)
{
	int sz = attr_find(p->n_ap, ATTR_P2STRUCT)->iarg(0);
	printf("	subl $%d,%%esp\n", sz);
	printf("	pushl $%d\n", sz);
	expand(p, 0, "	pushl AL\n");
	expand(p, 0, "	leal 8(%esp),A1\n");
	expand(p, 0, "	pushl A1\n");
	printf("	call memcpy\n");
	printf("	addl $12,%%esp\n");
}
#endif

void
zzzcode(NODE *p, int c)
{
	int pr;

	switch (c) {

	case 'C':  /* remove from stack after subroutine call */
		pr = p->n_qual;
		switch (pr) {
		case 1:
			printf("\tisz sp\n");
			break;
		case 2:
			printf("\tisz sp\n\tisz sp\n");
			break;
		case 3:
			printf("\tisz sp\n\tisz sp\n\tisz sp\n");
			break;
		case 4:
			printf("\tisz sp\n\tisz sp\n\tisz sp\n\tisz sp\n");
			break;
		default:
			printf("	lda 2,[0%o]\n", pr);
			printf("	lda 3,sp\n");
			printf("	add 2,3\n");
			printf("	sta 3,sp\n");
			break;
		}
		break;
#if 0
	case 'A': /* print out a skip ending if any numbers in queue */
		if (ldq == NULL)
			return;
		printf(",skp\n.LP%d:	.word 0%o", ldq->lab, ldq->val);
		if (ldq->name && *ldq->name)
			printf("+%s", ldq->name);
		printf("\n");
		ldq = ldq->next;
		break;

	case 'B': /* print a label for later load */
		ld = tmpalloc(sizeof(struct ldq));
		ld->val = p->n_lval;
		ld->name = p->n_name;
		ld->lab = prolnum++;
		ld->next = ldq;
		ldq = ld;
		printf(".LP%d-.", ld->lab);
		break;

	case 'C': /* fix reference to external variable via indirection */
		zzzcode(p->n_left, 'B');
		break;

	case 'D': /* fix reference to external variable via indirection */
		zzzcode(p, 'B');
		break;
#endif

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

cerror("flshape");
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
	printf("[" CONFMT "]", val);
}

/*
 * Conput prints out a constant.
 */
void
conput(FILE *fp, NODE *p)
{
	int val = p->n_lval;

	switch (p->n_op) {
	case ICON:
		if (p->n_name[0] != '\0') {
			fprintf(fp, "%s", p->n_name);
			if (val)
				fprintf(fp, "+0%o", val);
		} else
			fprintf(fp, "0%o", val);
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
comperr("upput");
#if 0
	size /= SZCHAR;
	switch (p->n_op) {
	case REG:
		printf("%%%s", &rnames[p->n_rval][3]);
		break;

	case NAME:
	case OREG:
		p->n_lval += size;
		adrput(stdout, p);
		p->n_lval -= size;
		break;
	case ICON:
		printf("$" CONFMT, p->n_lval >> 32);
		break;
	default:
		comperr("upput bad op %d size %d", p->n_op, size);
	}
#endif
}

void
adrput(FILE *io, NODE *p)
{
	int i;
	/* output an address, with offsets, from p */

static int looping = 7;
if (looping == 0) {
	looping = 1;
	printf("adrput %p\n", p);
	fwalk(p, e2print, 0);
	looping = 0;
}
	if (p->n_op == FLD)
		p = p->n_left;

	switch (p->n_op) {
	case ICON:
		/* addressable value of the constant */
		fputc('[', io);
		if (p->n_type == INCREF(CHAR) || p->n_type == INCREF(UCHAR))
			printf(".byteptr ");
		conput(io, p);
		fputc(']', io);
		break;

	case NAME:
		if (p->n_name[0] != '\0') {
			fputs(p->n_name, io);
			if (p->n_lval != 0)
				fprintf(io, "+" CONFMT, p->n_lval);
		} else
			fprintf(io, CONFMT, p->n_lval);
		break;;

	case OREG:
		if (p->n_name[0])
			comperr("name in OREG");
		i = (int)p->n_lval;
		if (i < 0) {
			putchar('-');
			i = -i;
		}
		printf("0%o,%s", i, rnames[regno(p)]);
		break;

	case REG:
		fprintf(io, "%s", rnames[p->n_rval]);
		break;

	default:
		comperr("illegal address, op %d, node %p", p->n_op, p);
		break;

	}
}

/*   printf conditional and unconditional branches */
void
cbgen(int o, int lab)
{
	comperr("cbgen");
}

void
myreader(struct interpass *ipole)
{
	if (x2debug)
		printip(ipole);
}

void
mycanon(NODE *p)
{
}

void
myoptim(struct interpass *ip)
{
}

void
rmove(int s, int d, TWORD t)
{
	comperr("rmove");
}

/*
 * For class c, find worst-case displacement of the number of
 * registers in the array r[] indexed by class.
 * Return true if we always can find a color.
 */
int
COLORMAP(int c, int *r)
{
	int num = 0;

	switch (c) {
	case CLASSA:
		num = (r[CLASSA]+r[CLASSB]) < AREGCNT;
		break;
	case CLASSB:
		num = (r[CLASSB]+r[CLASSA]) < BREGCNT;
		break;
	case CLASSC:
		num = r[CLASSC] < CREGCNT;
		break;
	case CLASSD:
		num = r[CLASSD] < DREGCNT;
		break;
	case CLASSE:
		num = r[CLASSE] < EREGCNT;
		break;
	}
	return num;
}

char *rnames[] = {
	"0", "1", "2", "3", "2", "3", "fp", "sp"
};

/*
 * Return a class suitable for a specific type.
 */
int
gclass(TWORD t)
{
	return ISPTR(t) ? CLASSB : CLASSA;
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
			size += szty(p->n_right->n_type);
	}
	if (p->n_op != ASSIGN)
		size += szty(p->n_type);

        op->n_qual = size; /* XXX */
}

/*
 * Special shapes.
 */
int
special(NODE *p, int shape)
{
	switch (shape) {
	case SLDFPSP:
		return regno(p) == FPREG || regno(p) == STKREG;
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
 * Supposed to find target-specific constraints and rewrite them.
 */
int
myxasm(struct interpass *ip, NODE *p)
{
	return 0;
}

void
storemod(NODE *q, int off, int reg)
{
	NODE *l, *r, *p;

	if (off < MAXZP*2) {
		q->n_op = NAME;
		q->n_name = "";
		q->n_lval = -off/2 + ZPOFF;
	} else {
		l = mklnode(REG, 0, reg, INCREF(q->n_type));
		r = mklnode(ICON, off, 0, INT);
		p = mkbinode(PLUS, l, r, INCREF(q->n_type));
		q->n_op = UMUL;
		q->n_left = p;
	}
	q->n_rval = q->n_su = 0;
}
