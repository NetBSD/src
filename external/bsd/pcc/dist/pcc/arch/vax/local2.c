/*	Id: local2.c,v 1.37 2012/12/21 21:44:27 ragge Exp 	*/	
/*	$NetBSD: local2.c,v 1.1.1.6 2014/07/24 19:21:48 plunky Exp $	*/
/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code and documentation must retain the above
 * copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditionsand the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 * 	This product includes software developed or owned by Caldera
 *	International, Inc.
 * Neither the name of Caldera International, Inc. nor the names of other
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OFLIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

# include "pass2.h"
# include "ctype.h"
/* a lot of the machine dependent parts of the second pass */

static void prtype(NODE *n);
static void acon(NODE *p);

/*
 * Print out the prolog assembler.
 * addto and regoff are already calculated.
 */
void
prologue(struct interpass_prolog *ipp)
{
	printf("	.word 0x%llx\n", (unsigned long long)ipp->ipp_regs[0]);
	if (p2maxautooff)
		printf("	subl2 $%d,%%sp\n", p2maxautooff);
	if (pflag) {
		int i = getlab2();
		printf("\tmovab\t" LABFMT ",%%r0\n", i);
		printf("\tjsb\t__mcount\n");
		printf("\t.data\n");
		printf("\t.align  2\n");
		printf(LABFMT ":\t.long\t0\n", i);
		printf("\t.text\n");
	}
}

/*
 * Called after all instructions in a function are emitted.
 * Generates code for epilog here.
 */
void
eoftn(struct interpass_prolog *ipp)
{
	if (ipp->ipp_ip.ip_lbl == 0)
		return; /* no code needs to be generated */
	printf("	ret\n");
}

struct hoptab { int opmask; char * opstring; } ioptab[] = {

	{ PLUS,	"add", },
	{ MINUS,	"sub", },
	{ MUL,	"mul", },
	{ DIV,	"div", },
	{ OR,	"bis", },
	{ ER,	"xor", },
	{ AND,	"bic", },
	{ -1, ""     },
};

void
hopcode( f, o ){
	/* output the appropriate string from the above table */

	register struct hoptab *q;

	for( q = ioptab;  q->opmask>=0; ++q ){
		if( q->opmask == o ){
			printf( "%s", q->opstring );
/* tbl
			if( f == 'F' ) printf( "e" );
			else if( f == 'D' ) printf( "d" );
   tbl */
/* tbl */
			switch( f ) {
				case 'L':
				case 'W':
				case 'B':
				case 'D':
				case 'F':
					printf("%c", tolower(f));
					break;

				}
/* tbl */
			return;
			}
		}
	cerror( "no hoptab for %s", opst[o] );
	}

char *
rnames[] = {  /* keyed to register number tokens */

	"%r0", "%r1", "%r2", "%r3", "%r4", "%r5",
	"%r6", "%r7", "%r8", "%r9", "%r10", "%r11",
	"%ap", "%fp", "%sp", "%pc",
	/* The concatenated regs has the name of the lowest */
	"%r0", "%r1", "%r2", "%r3", "%r4", "%r5",
	"%r6", "%r7", "%r8", "%r9", "%r10"
	};

int
tlen(NODE *p)
{
	switch(p->n_type) {
	case CHAR:
	case UCHAR:
		return(1);

	case SHORT:
	case USHORT:
		return(2);

	case DOUBLE:
	case LONGLONG:
	case ULONGLONG:
		return(8);

	default:
		return(4);
	}
}

void
prtype(NODE *n)
{
	static char pt[] = { 0, 0, 'b', 'b', 'w', 'w', 'l', 'l', 0, 0,
	    'q', 'q', 'f', 'd' };
	TWORD t = n->n_type;

	if (ISPTR(t))
		t = UNSIGNED;

	if (t > DOUBLE || pt[t] == 0)
		comperr("prtype: bad type");
	putchar(pt[t]);
}

/*
 * Emit conversions as given by the following table. Dest is always reg,
 *   if it should be something else let peephole optimizer deal with it.
 *   This code ensures type correctness in 32-bit registers.
 *   XXX is that necessary?
 *
 * From				To
 *	 char   uchar  short  ushort int    uint   ll    ull   float double
 * char  movb   movb   cvtbw  cvtbw  cvtbl  cvtbl  A     A     cvtbf cvtbd
 * uchar movb   movb   movzbw movzbw movzbl movzbl B     B     G     G
 * short movb   movb   movw   movw   cvtwl  cvtwl  C(A)  C(A)  cvtwf cvtwd
 * ushrt movb   movb   movw   movw   movzwl movzwl D(B)  D(B)  H     H
 * int   movb   movb   movw   movw   movl   movl   E     E     cvtlf cvtld
 * uint  movb   movb   movw   movw   movl   movl   F     F     I     I
 * ll    movb   movb   movw   movw   movl   movl   movq  movq  J     K
 * ull   movb   movb   movw   movw   movl   movl   movq  movq  L     M
 * float cvtfb  cvtfb  cvtfw  cvtfw  cvtfl  cvtfl  N     O     movf  cvtfd
 * doubl cvtdb  cvtdb  cvtdw  cvtdw  cvtdl  cvtdl  P     Q     cvtdf movd
 *
 *  A: cvtbl + sign extend
 *  B: movzbl + zero extend
 *  G: movzbw + cvtwX
 *  H: movzwl + cvtwX
 *  I: cvtld + addX
 *  J: call __floatdisf
 *  K: call __floatdidf
 *  L: xxx + call __floatdisf
 *  M: xxx + call __floatdidf
 *  N: call __fixsfdi
 *  O: call __fixunssfdi
 *  P: call __fixdfdi
 *  Q: call __fixunsdfdi
 */

#define	MVD	1 /* mov + dest type */
#define	CVT	2 /* cvt + src type + dst type */
#define	MVZ	3 /* movz + src type + dst type */
#define	CSE	4 /* cvt + src type + l + sign extend upper */
#define	MZE	5 /* movz + src type + l + zero extend upper */
#define	MLE	6 /* movl + sign extend upper */
#define	MLZ	7 /* movl + zero extend upper */
#define	MZC	8 /* movz + cvt */

static char scary[][10] = {
	{ MVD, MVD, CVT, CVT, CVT, CVT, CSE, CSE, CVT, CVT },
	{ MVD, MVD, MVZ, MVZ, MVZ, MVZ, MZE, MZE, MZC, MZC },
	{ MVD, MVD, MVD, MVD, CVT, CVT, CSE, CSE, CVT, CVT },
	{ MVD, MVD, MVD, MVD, MVZ, MVZ, MZE, MZE, MZC, MZC },
	{ MVD, MVD, MVD, MVD, MVD, MVD, MLE, MLE, CVT, CVT },
	{ MVD, MVD, MVD, MVD, MVD, MVD, MLZ, MLZ, 'I', 'I' },
	{ MVD, MVD, MVD, MVD, MVD, MVD, MVD, MVD, 'J', 'K' },
	{ MVD, MVD, MVD, MVD, MVD, MVD, MVD, MVD, 'L', 'M' },
	{ CVT, CVT, CVT, CVT, CVT, CVT, 'N', 'O', MVD, CVT },
	{ CVT, CVT, CVT, CVT, CVT, CVT, 'P', 'Q', CVT, MVD },
};

static void
sconv(NODE *p)
{
	NODE *l = p->n_left;
	TWORD ts, td;
	int o;

	/*
	 * Source node may be in register or memory.
	 * Result is always in register.
	 */
	ts = l->n_type;
	if (ISPTR(ts))
		ts = UNSIGNED;
	td = p->n_type;
	ts = ts < LONG ? ts-2 : ts-4;
	td = td < LONG ? td-2 : td-4;

	o = scary[ts][td];
	switch (o) {
	case MLE:
	case MLZ:
		expand(p, INAREG|INBREG, "\tmovl\tAL,A1\n");
		break;

	case MVD:
		if (l->n_op == REG && regno(l) == regno(getlr(p, '1')))
			break; /* unneccessary move */
		expand(p, INAREG|INBREG, "\tmovZR\tAL,A1\n");
		break;

	case CSE:
		expand(p, INAREG|INBREG, "\tcvtZLl\tAL,A1\n");
		break;

	case CVT:
		expand(p, INAREG|INBREG, "\tcvtZLZR\tAL,A1\n");
		break;

	case MZE:
		expand(p, INAREG|INBREG, "\tmovzZLl\tAL,A1\n");
		break;

	case MVZ:
		expand(p, INAREG|INBREG, "\tmovzZLZR\tAL,A1\n");
		break;

	case MZC:
		expand(p, INAREG|INBREG, "\tmovzZLl\tAL,A1\n");
		expand(p, INAREG|INBREG, "\tcvtlZR\tA1,A1\n");
		break;

	case 'I': /* unsigned to double */
		expand(p, INAREG|INBREG, "\tcvtld\tAL,A1\n");
		printf("\tjgeq\t1f\n");
		expand(p, INAREG|INBREG, "\taddd2\t$0d4.294967296e+9,A1\n");
		printf("1:\n");
		break;
	default:
		comperr("unsupported conversion %d", o);
	}
	switch (o) {
	case MLE:
	case CSE:
		expand(p, INBREG, "\tashl\t$-31,A1,U1\n");
		break;
	case MLZ:
	case MZE:
		expand(p, INAREG|INBREG, "\tclrl\tU1\n");
		break;
	}
}

/*
 * Assign a constant from p to q.  Both are expected to be leaves by now.
 * This is for 64-bit integers.
 */
static void
casg64(NODE *p)
{
	NODE *l, *r;
	char *str;
	int mneg = 1;
	
	l = p->n_left;
	r = p->n_right;

#ifdef PCC_DEBUG
	if (r->n_op != ICON)
		comperr("casg");
#endif
	if (r->n_name[0] != '\0') {
		/* named constant, nothing to do */
		str = "movq\tAR,AL";
		mneg = 0;
	} else if (r->n_lval == 0) {
		str = "clrq\tAL";
		mneg = 0;
	} else if (r->n_lval < 0) {
		if (r->n_lval >= -63) {
			r->n_lval = -r->n_lval;
			str = "mnegl\tAR,AL";
		} else if (r->n_lval >= -128) {
			str = "cvtbl\tAR,AL";
		} else if (r->n_lval >= -32768) {
			str = "cvtwl\tAR,AL";
		} else if (r->n_lval >= -4294967296LL) {
			str = "movl\tAR,AL";
		} else {
			str = "movq\tAR,AL";
			mneg = 0;
		}
	} else {
		mneg = 0;
		if (r->n_lval <= 63 || r->n_lval > 4294967295LL) {
			str = "movq\tAR,AL";
		} else if (r->n_lval <= 255) {
			str = "movzbl\tAR,AL\n\tclrl\tUL";
		} else if (r->n_lval <= 65535) {
			str = "movzwl\tAR,AL\n\tclrl\tUL";
		} else /* if (r->n_lval <= 4294967295) */ {
			str = "movl\tAR,AL\n\tclrl\tUL";
		}
	}
	expand(p, FOREFF, str);
	if (mneg)
		expand(p, FOREFF, "\n\tmnegl $1,UL");
}

/*
 * Assign a constant from p to q.  Both are expected to be leaves by now.
 * This is only for 32-bit integer types.
 */
static void
casg(NODE *p)
{
	NODE *l, *r;
	char *str;
	
	l = p->n_left;
	r = p->n_right;

#ifdef PCC_DEBUG
	if (r->n_op != ICON)
		comperr("casg");
#endif
	if (r->n_name[0] != '\0') {
		/* named constant, nothing to do */
		str = "movZL\tAR,AL";
	} else if (r->n_lval == 0) {
		str = "clrZL\tAL";
	} else if (r->n_lval < 0) {
		if (r->n_lval >= -63) {
			r->n_lval = -r->n_lval;
			str = "mnegZL\tAR,AL";
		} else if (r->n_lval >= -128) {
			if (l->n_type == CHAR)
				str = "movb\tAR,AL";
			else
				str = "cvtbZL\tAR,AL";
		} else if (r->n_lval >= -32768) {
			if (l->n_type == SHORT)
				str = "movw\tAR,AL";
			else
				str = "cvtwZL\tAR,AL";
		} else
			str = "movZL\tAR,AL";
	} else {
		if (r->n_lval <= 63 || r->n_lval > 65535) {
			str = "movZL\tAR,AL";
		} else if (r->n_lval <= 255) {
			str = l->n_type < SHORT ?
			    "movb\tAR,AL" : "movzbZL\tAR,AL";
		} else /* if (r->n_lval <= 65535) */ {
			str = l->n_type < INT ?
			    "movw\tAR,AL" : "movzwZL\tAR,AL";
		}
	}
	expand(p, FOREFF, str);
}

/*
 * Emit code to compare two longlong numbers.
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
	expand(p, 0, "	cmpl UL,UR\n");
	if (cb1) cbgen(cb1, s);
	if (cb2) cbgen(cb2, e);
	expand(p, 0, "	cmpl AL,AR\n");
	cbgen(u, e);
	deflab(s);
}


void
zzzcode(NODE *p, int c)
{
	NODE *l, *r;
	TWORD t;
	int m;
	char *ch;

	switch (c) {
	case 'N':  /* logical ops, turned into 0-1 */
		/* use register given by register 1 */
		cbgen( 0, m=getlab2());
		deflab( p->n_label );
		printf( "	clrl	%s\n", rnames[getlr( p, '1' )->n_rval] );
		deflab( m );
		return;

	case 'A': /* Assign a constant directly to a memory position */
		printf("\t");
		if (p->n_type < LONG || ISPTR(p->n_type))
			casg(p);
		else
			casg64(p);
		printf("\n");
		break;

	case 'B': /* long long compare */
		twollcomp(p);
		break;

	case 'C':	/* num words pushed on arg stack */
		printf("$%d", p->n_qual);
		break;

	case 'D':	/* INCR and DECR */
		zzzcode(p->n_left, 'A');
		printf("\n	");

#if 0
	case 'E':	/* INCR and DECR, FOREFF */
		if (p->n_right->n_lval == 1)
			{
			printf("%s", (p->n_op == INCR ? "inc" : "dec") );
			prtype(p->n_left);
			printf("	");
			adrput(stdout, p->n_left);
			return;
			}
		printf("%s", (p->n_op == INCR ? "add" : "sub") );
		prtype(p->n_left);
		printf("2	");
		adrput(stdout, p->n_right);
		printf(",");
		adrput(p->n_left);
		return;
#endif

	case 'F':	/* register type of right operand */
		{
		register NODE *n;
		register int ty;

		n = getlr( p, 'R' );
		ty = n->n_type;

		if (x2debug) printf("->%d<-", ty);

		if ( ty==DOUBLE) printf("d");
		else if ( ty==FLOAT ) printf("f");
		else printf("l");
		return;
		}

	case 'G': /* emit conversion instructions */
		sconv(p);
		break;

	case 'J': /* jump or ret? */
		{
			struct interpass *ip =
			    DLIST_PREV((struct interpass *)p2env.epp, qelem);
			if (ip->type != IP_DEFLAB ||
			    ip->ip_lbl != getlr(p, 'L')->n_lval)
				expand(p, FOREFF, "jbr	LL");
			else
				printf("ret");
		}
		break;

	case 'L':	/* type of left operand */
	case 'R':	/* type of right operand */
		{
		register NODE *n;

		n = getlr ( p, c);
		if (x2debug) printf("->%d<-", n->n_type);

		prtype(n);
		return;
		}

	case 'O': /* print out emulated ops */
		expand(p, FOREFF, "\tmovq	AR,-(%sp)\n");
		expand(p, FOREFF, "\tmovq	AL,-(%sp)\n");
		if (p->n_op == DIV && p->n_type == ULONGLONG) ch = "udiv";
		else if (p->n_op == DIV) ch = "div";
		else if (p->n_op == MOD && p->n_type == ULONGLONG) ch = "umod";
		else if (p->n_op == MOD) ch = "mod";
		else if (p->n_op == MUL) ch = "mul";
		else ch = 0, comperr("ZO %d", p->n_op);
		printf("\tcalls	$4,__%sdi3\n", ch);
		break;


	case 'Z':	/* complement mask for bit instr */
		printf("$%Ld", ~p->n_right->n_lval);
		return;

	case 'U':	/* 32 - n, for unsigned right shifts */
		t = DEUNSIGN(p->n_left->n_type);
		m = t == CHAR ? 8 : t == SHORT ? 16 : 32;
		printf("$" CONFMT, m - p->n_right->n_lval);
		return;

	case 'T':	/* rounded structure length for arguments */
		{
		int size;

		size = p->n_stsize;
		SETOFF( size, 4);
		printf("$%d", size);
		return;
		}

	case 'S':  /* structure assignment */
		{
			register int size;

			size = p->n_stsize;
			l = r = NULL; /* XXX gcc */
			if( p->n_op == STASG ){
				l = p->n_left;
				r = p->n_right;

				}
			else if( p->n_op == STARG ){
				/* store an arg into a temporary */
				printf("\tsubl2 $%d,%%sp\n",
				    size < 4 ? 4 : size);
				l = mklnode(OREG, 0, SP, INT);
				r = p->n_left;
				}
			else cerror( "STASG bad" );

			if( r->n_op == ICON ) r->n_op = NAME;
			else if( r->n_op == REG ) r->n_op = OREG;
			else if( r->n_op != OREG ) cerror( "STASG-r" );

		if (size != 0) {
			if( size <= 0 || size > 65535 )
				cerror("structure size <0=0 or >65535");

			switch(size) {
				case 1:
					printf("	movb	");
					break;
				case 2:
					printf("	movw	");
					break;
				case 4:
					printf("	movl	");
					break;
				case 8:
					printf("	movq	");
					break;
				default:
					printf("	movc3	$%d,", size);
					break;
			}
			adrput(stdout, r);
			printf(",");
			adrput(stdout, l);
			printf("\n");
		}

			if( r->n_op == NAME ) r->n_op = ICON;
			else if( r->n_op == OREG ) r->n_op = REG;
			if (p->n_op == STARG)
				tfree(l);

			}
		break;

	default:
		comperr("illegal zzzcode '%c'", c);
	}
}

void
rmove(int rt, int rs, TWORD t)
{
	printf( "	%s	%s,%s\n",
		(t==FLOAT ? "movf" : (t==DOUBLE ? "movd" : "movl")),
		rnames[rt], rnames[rs] );
}

int
rewfld(NODE *p)
{
	return(1);
}

#if 0
int
callreg(NODE *p)
{
	return( R0 );
}

int
base(register NODE *p)
{
	register int o = p->op;

	if( (o==ICON && p->name[0] != '\0')) return( 100 ); /* ie no base reg */
	if( o==REG ) return( p->rval );
    if( (o==PLUS || o==MINUS) && p->left->op == REG && p->right->op==ICON)
		return( p->left->rval );
    if( o==OREG && !R2TEST(p->rval) && (p->type==INT || p->type==UNSIGNED || ISPTR(p->type)) )
		return( p->rval + 0200*1 );
	if( o==INCR && p->left->op==REG ) return( p->left->rval + 0200*2 );
	if( o==ASG MINUS && p->left->op==REG) return( p->left->rval + 0200*4 );
	if( o==UNARY MUL && p->left->op==INCR && p->left->left->op==REG
	  && (p->type==INT || p->type==UNSIGNED || ISPTR(p->type)) )
		return( p->left->left->rval + 0200*(1+2) );
	return( -1 );
}

int
offset(register NODE *p, int tyl)
{

	if( tyl==1 && p->op==REG && (p->type==INT || p->type==UNSIGNED) ) return( p->rval );
	if( (p->op==LS && p->left->op==REG && (p->left->type==INT || p->left->type==UNSIGNED) &&
	      (p->right->op==ICON && p->right->name[0]=='\0')
	      && (1<<p->right->lval)==tyl))
		return( p->left->rval );
	return( -1 );
}
#endif

#if 0
void
makeor2( p, q, b, o) register NODE *p, *q; register int b, o; {
	register NODE *t;
	NODE *f;

	p->n_op = OREG;
	f = p->n_left; 	/* have to free this subtree later */

	/* init base */
	switch (q->n_op) {
		case ICON:
		case REG:
		case OREG:
			t = q;
			break;

		case MINUS:
			q->n_right->n_lval = -q->n_right->n_lval;
		case PLUS:
			t = q->n_right;
			break;

		case UMUL:
			t = q->n_left->n_left;
			break;

		default:
			cerror("illegal makeor2");
			t = NULL; /* XXX gcc */
	}

	p->n_lval = t->n_lval;
	p->n_name = t->n_name;

	/* init offset */
	p->n_rval = R2PACK( (b & 0177), o, (b>>7) );

	tfree(f);
	return;
	}

int
canaddr( p ) NODE *p; {
	register int o = p->n_op;

	if( o==NAME || o==REG || o==ICON || o==OREG || (o==UMUL && shumul(p->n_left, STARNM|SOREG)) ) return(1);
	return(0);
	}

shltype( o, p ) register NODE *p; {
	return( o== REG || o == NAME || o == ICON || o == OREG || ( o==UMUL && shumul(p->n_left, STARNM|SOREG)) );
	}
#endif

int
fldexpand(NODE *p, int cookie, char **cp)
{
	return 0;
}

int
flshape(register NODE *p)
{
	return( p->n_op == REG || p->n_op == NAME || p->n_op == ICON ||
		(p->n_op == OREG && (!R2TEST(p->n_rval) || tlen(p) == 1)) );
}

int
shtemp(register NODE *p)
{
	if( p->n_op == STARG ) p = p->n_left;
	return( p->n_op==NAME || p->n_op ==ICON || p->n_op == OREG || (p->n_op==UMUL && shumul(p->n_left, STARNM|SOREG)) );
}

/*
 * Shape matches for UMUL.  Cooperates with offstar().
 */
int
shumul(NODE *p, int shape)
{

	if (x2debug)
		printf("shumul(%p)\n", p);

	/* Turns currently anything into OREG on vax */
	if (shape & SOREG)
		return SROREG;
	return SRNOPE;
}


#ifdef notdef
int
shumul( p, shape ) register NODE *p; int shape; {
	register int o;

	if (x2debug) {
		 printf("\nshumul:op=%d,lop=%d,rop=%d", p->n_op, p->n_left->n_op, p->n_right->n_op);
		printf(" prname=%s,plty=%d, prlval=%lld\n", p->n_right->n_name, p->n_left->n_type, p->n_right->n_lval);
		}


	o = p->n_op;
	if( o == NAME || (o == OREG && !R2TEST(p->n_rval)) || o == ICON )
		if (shape & STARNM)
			return SRDIR;

	if( ( o == INCR || o == ASG MINUS ) &&
	    ( p->n_left->n_op == REG && p->n_right->n_op == ICON ) &&
	    p->n_right->n_name[0] == '\0' )
		{
		switch (p->n_left->n_type)
			{
			case CHAR|PTR:
			case UCHAR|PTR:
				o = 1;
				break;

			case SHORT|PTR:
			case USHORT|PTR:
				o = 2;
				break;

			case INT|PTR:
			case UNSIGNED|PTR:
			case LONG|PTR:
			case ULONG|PTR:
			case FLOAT|PTR:
				o = 4;
				break;

			case DOUBLE|PTR:
				o = 8;
				break;

			default:
				if ( ISPTR(p->n_left->n_type) ) {
					o = 4;
					break;
					}
				else return(0);
			}
		return( p->n_right->n_lval == o ? STARREG : 0);
		}

	return( SRNOPE );
	}
#endif

void
adrcon(CONSZ val)
{
	printf( "$" );
	printf( CONFMT, val );
}

void
conput(FILE *fp, NODE *p)
{
	switch( p->n_op ){

	case ICON:
		acon( p );
		return;

	case REG:
		printf( "%s", rnames[p->n_rval] );
		return;

	default:
		cerror( "illegal conput" );
		}
	}

void
insput(register NODE *p)
{
	cerror( "insput" );
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
		printf("%s", rnames[regno(p)-16+1]);
		break;

	case NAME:
		if (kflag)
			comperr("upput NAME");
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
}

void
adrput(FILE *fp, NODE *p)
{
	register int r;
	/* output an address, with offsets, from p */

	if( p->n_op == FLD ){
		p = p->n_left;
		}
	switch( p->n_op ){

	case NAME:
		acon( p );
		return;

	case ICON:
		/* addressable value of the constant */
		if (p->n_name[0] == '\0') /* uses xxxab */
			printf("$");
		acon(p);
		return;

	case REG:
		printf( "%s", rnames[p->n_rval] );
		return;

	case OREG:
		r = p->n_rval;
		if( R2TEST(r) ){ /* double indexing */
			register int flags;

			flags = R2UPK3(r);
			if( flags & 1 ) printf("*");
			if( flags & 4 ) printf("-");
			if( p->n_lval != 0 || p->n_name[0] != '\0' ) acon(p);
			if( R2UPK1(r) != 100) printf( "(%s)", rnames[R2UPK1(r)] );
			if( flags & 2 ) printf("+");
			printf( "[%s]", rnames[R2UPK2(r)] );
			return;
			}
		if( r == AP ){  /* in the argument region */
			if( p->n_lval <= 0 || p->n_name[0] != '\0' )
				werror( "bad arg temp" );
			printf( CONFMT, p->n_lval );
			printf( "(%%ap)" );
			return;
			}
		if( p->n_lval != 0 || p->n_name[0] != '\0') acon( p );
		printf( "(%s)", rnames[p->n_rval] );
		return;

	case UMUL:
		/* STARNM or STARREG found */
		if( tshape(p, STARNM) ) {
			printf( "*" );
			adrput(0,  p->n_left);
			}
		else {	/* STARREG - really auto inc or dec */
			register NODE *q;

/* tbl
			p = p->n_left;
			p->n_left->n_op = OREG;
			if( p->n_op == INCR ) {
				adrput( p->n_left );
				printf( "+" );
				}
			else {
				printf( "-" );
				adrput( p->n_left );
				}
   tbl */
#ifdef notyet
			printf("%c(%s)%c", (p->n_left->n_op==INCR ? '\0' : '-'),
				rnames[p->n_left->n_left->n_rval], 
				(p->n_left->n_op==INCR ? '+' : '\0') );
#else
			printf("%c(%s)%c", '-',
				rnames[p->n_left->n_left->n_rval], 
				'\0' );
#endif
			p->n_op = OREG;
			p->n_rval = p->n_left->n_left->n_rval;
			q = p->n_left;
#ifdef notyet

			p->n_lval = (p->n_left->n_op == INCR ? -p->n_left->n_right->n_lval : 0);
#else
			p->n_lval = 0;
#endif
			p->n_name[0] = '\0';
			tfree(q);
		}
		return;

	default:
		cerror( "illegal address" );
		return;
	}

}

/*
 * print out a constant
 */
void
acon(NODE *p)
{

	if (p->n_name[0] == '\0') {
		printf(CONFMT, p->n_lval);
	} else if( p->n_lval == 0 ) {
		printf("%s", p->n_name);
	} else {
		printf("%s+", p->n_name);
		printf(CONFMT, p->n_lval);
	}
}

#if 0
genscall( p, cookie ) register NODE *p; {
	/* structure valued call */
	return( gencall( p, cookie ) );
	}

/* tbl */
int gc_numbytes;
/* tbl */

gencall( p, cookie ) register NODE *p; {
	/* generate the call given by p */
	register NODE *p1, *ptemp;
	register temp, temp1;
	register m;

	if( p->right ) temp = argsize( p->right );
	else temp = 0;

	if( p->op == STCALL || p->op == UNARY STCALL ){
		/* set aside room for structure return */

		if( p->stsize > temp ) temp1 = p->stsize;
		else temp1 = temp;
		}

	if( temp > maxargs ) maxargs = temp;
	SETOFF(temp1,4);

	if( p->right ){ /* make temp node, put offset in, and generate args */
		ptemp = talloc();
		ptemp->op = OREG;
		ptemp->lval = -1;
		ptemp->rval = SP;
		ptemp->name[0] = '\0';
		ptemp->rall = NOPREF;
		ptemp->su = 0;
		genargs( p->right, ptemp );
		nfree(ptemp);
		}

	p1 = p->left;
	if( p1->op != ICON ){
		if( p1->op != REG ){
			if( p1->op != OREG || R2TEST(p1->rval) ){
				if( p1->op != NAME ){
					order( p1, INAREG );
					}
				}
			}
		}

/*
	if( p1->op == REG && p->rval == R5 ){
		cerror( "call register overwrite" );
		}
 */
/* tbl
	setup gc_numbytes so reference to ZC works */

	gc_numbytes = temp;
/* tbl */

	p->op = UNARY CALL;
	m = match( p, INTAREG|INTBREG );
/* tbl
	switch( temp ) {
	case 0:
		break;
	case 2:
		printf( "	tst	(%sp)+\n" );
		break;
	case 4:
		printf( "	cmp	(%sp)+,(%sp)+\n" );
		break;
	default:
		printf( "	add	$%d,%sp\n", temp);
		}
   tbl */
	return(m != MDONE);
	}
#endif

static char *
ccbranches[] = {
	"jeql",
	"jneq",
	"jleq",
	"jlss",
	"jgeq",
	"jgtr",
	"jlequ",
	"jlssu",
	"jgequ",
	"jgtru",
};

/*
 * printf conditional and unconditional branches
 */
void
cbgen(int o, int lab)
{

	if (o == 0) {
		printf("	jbr     " LABFMT "\n", lab);
	} else {
		if (o > UGT)
			comperr("bad conditional branch: %s", opst[o]);
		printf("\t%s\t" LABFMT "\n", ccbranches[o-EQ], lab);
	}
}

static void
mkcall(NODE *p, char *name)
{
	p->n_op = CALL;
	p->n_right = mkunode(FUNARG, p->n_left, 0, p->n_left->n_type);
	p->n_left = mklnode(ICON, 0, 0, FTN|p->n_type);
	p->n_left->n_name = name;
}

/* do local tree transformations and optimizations */
static void
optim2(NODE *p, void *arg)
{
	NODE *r, *s;
	TWORD lt;

	switch (p->n_op) {
	case DIV:
	case MOD:
		if (p->n_type == USHORT || p->n_type == UCHAR) {
			r = mkunode(SCONV, p->n_left, 0, UNSIGNED);
			r = mkunode(FUNARG, r, 0, UNSIGNED);
			s = mkunode(SCONV, p->n_right, 0, UNSIGNED);
			s = mkunode(FUNARG, s, 0, UNSIGNED);
			r = mkbinode(CM, r, s, INT);
			s = mklnode(ICON, 0, 0, FTN|UNSIGNED);
			s->n_name = p->n_op == MOD ? "__urem" : "__udiv";
			p->n_left = mkbinode(CALL, s, r, UNSIGNED);
			p->n_op = SCONV;
		} else if (p->n_type == UNSIGNED) {
			p->n_left = mkunode(FUNARG, p->n_left, 0, UNSIGNED);
			p->n_right = mkunode(FUNARG, p->n_right, 0, UNSIGNED);
			p->n_right = mkbinode(CM, p->n_left, p->n_right, INT);
			p->n_left = mklnode(ICON, 0, 0, FTN|UNSIGNED);
			p->n_left->n_name = p->n_op == MOD ? "__urem" : "__udiv";
			p->n_op = CALL;
		}
		break;

	case RS:
		if (p->n_type == ULONGLONG) {
			p->n_right = mkbinode(CM, 
			    mkunode(FUNARG, p->n_left, 0, p->n_left->n_type),
			    mkunode(FUNARG, p->n_right, 0, p->n_right->n_type),
			    INT);
			p->n_left = mklnode(ICON, 0, 0, FTN|p->n_type);
			p->n_left->n_name = "__lshrdi3";
			p->n_op = CALL;
		} else if (p->n_type == INT || p->n_type == LONGLONG) {
			/* convert >> to << with negative shift count */
			/* RS of char & short must use extv */
			if (p->n_right->n_op == ICON) {
				p->n_right->n_lval = -p->n_right->n_lval;
			} else if (p->n_right->n_op == UMINUS) {
				r = p->n_right->n_left;
				nfree(p->n_right);
				p->n_right = r;
			} else {
				p->n_right = mkunode(UMINUS, p->n_right,
				    0, p->n_right->n_type);
			}
			p->n_op = LS;
		}
		break;

	case AND:
		/* commute L and R to eliminate compliments and constants */
		if ((p->n_left->n_op == ICON && p->n_left->n_name[0] == 0) ||
		    p->n_left->n_op==COMPL) {
			r = p->n_left;
			p->n_left = p->n_right;
			p->n_right = r;
		}
		/* change meaning of AND to ~R&L - bic on pdp11 */
		r = p->n_right;
		if (r->n_op == ICON && r->n_name[0] == 0) {
			/* compliment constant */
			r->n_lval = ~r->n_lval;
		} else if (r->n_op == COMPL) { /* ~~A => A */
			s = r->n_left;
			nfree(r);
			p->n_right = s;
		} else { /* insert complement node */
			p->n_right = mkunode(COMPL, r, 0, r->n_type);
		}
		break;
	case SCONV:
		lt = p->n_left->n_type;
		switch (p->n_type) {
		case LONGLONG:
			if (lt == FLOAT)
				mkcall(p, "__fixsfdi");
			else if (lt == DOUBLE)
				mkcall(p, "__fixdfdi");
			break;
		case ULONGLONG:
			if (lt == FLOAT)
				mkcall(p, "__fixunssfdi");
			else if (lt == DOUBLE)
				mkcall(p, "__fixunsdfdi");
			break;
		case FLOAT:
			if (lt == LONGLONG)
				mkcall(p, "__floatdisf");
			else if (lt == ULONGLONG) {
				p->n_left = mkunode(SCONV, p->n_left,0, DOUBLE);
				p->n_type = FLOAT;
				mkcall(p->n_left, "__floatundidf");
			} else if (lt == UNSIGNED) {
				/* insert an extra double-to-float sconv */
				p->n_left = mkunode(SCONV, p->n_left,0, DOUBLE);
			}
			break;
		case DOUBLE:
			if (lt == LONGLONG)
				mkcall(p, "__floatdidf");
			else if (lt == ULONGLONG)
				mkcall(p, "__floatundidf");
			break;
			
		}
		break;
	}
}

static void
aofname(NODE *p, void *arg)
{
	int o = optype(p->n_op);
	TWORD t;

	if (o == LTYPE || p->n_op == ADDROF)
		return;
	t = p->n_left->n_type;
	if (p->n_left->n_op == NAME && ISLONGLONG(t))
		p->n_left = mkunode(UMUL,
		    mkunode(ADDROF, p->n_left, 0, INCREF(t)), 0, t);
	if (o == BITYPE && p->n_right->n_op == NAME &&
	    ISLONGLONG(p->n_right->n_type)) {
		t = p->n_right->n_type;
		p->n_right = mkunode(UMUL,
		    mkunode(ADDROF, p->n_right, 0, INCREF(t)), 0, t);
	}
}

void
myreader(struct interpass *ipole)
{
	struct interpass *ip;

	DLIST_FOREACH(ip, ipole, qelem) {
		if (ip->type != IP_NODE)
			continue;
		if (kflag)
			walkf(ip->ip_node, aofname, 0);
		walkf(ip->ip_node, optim2, 0);
	}
}

void
mycanon(NODE *p)
{
}

void
myoptim(struct interpass *ip)
{
}

/*
 * Return argument size in regs.
 */
static int
argsiz(NODE *p)
{
	TWORD t = p->n_type;

	if (t == STRTY || t == UNIONTY)
		return p->n_stsize/(SZINT/SZCHAR);
	return szty(t);
}

/*
 * Last chance to do something before calling a function.
 */
void
lastcall(NODE *p)
{
	NODE *op = p;
	int size = 0;

	/* Calculate argument sizes */
	p->n_qual = 0;
	if (p->n_op != CALL && p->n_op != FORTCALL && p->n_op != STCALL)
		return;
	for (p = p->n_right; p->n_op == CM; p = p->n_left)
		size += argsiz(p->n_right);
	if (p->n_op != ASSIGN)
		size += argsiz(p);
	op->n_qual = size; /* XXX */
}

/*
 * Return a class suitable for a specific type.
 */
int
gclass(TWORD t)
{
	return (szty(t) == 2 ? CLASSB : CLASSA);
}

/*
 * For class c, find worst-case displacement of the number of
 * registers in the array r[] indexed by class.
 */
int
COLORMAP(int c, int *r)
{
	int num;
	int a,b;

	a = r[CLASSA];
	b = r[CLASSB];
	switch (c) {
	case CLASSA:
		/* there are 12 classa, so min 6 classb are needed to block */
		num = b * 2;
		num += a;
		return num < 12;
	case CLASSB:
		if (b > 3) return 0;
		if (b > 2 && a) return 0;
		if (b > 1 && a > 2) return 0;
		if (b && a > 3) return 0;
		if (a > 5) return 0;
		return 1;
	}
	comperr("COLORMAP");
	return 0; /* XXX gcc */
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
 * Supposed to find target-specific constraints and rewrite them.
 */
int
myxasm(struct interpass *ip, NODE *p)
{
	char *c;
	int i;

	/* Discard o<> constraints since they will not be generated */
	for (c = p->n_name; *c; c++) {
		if (*c == 'o' || *c == '<' || *c == '>') {
			for (i = 0; c[i]; i++)
				c[i] = c[i+1];
			c--;
		}
	}
	return 0;
}

int
xasmconstregs(char *s)
{
	int i;

	for (i = 0; i < 16; i++)
		if (strcmp(&rnames[i][1], s) == 0)
			return i;
	return -1;
}
