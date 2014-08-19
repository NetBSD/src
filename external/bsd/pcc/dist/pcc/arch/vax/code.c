/*	Id: code.c,v 1.25 2014/04/19 07:47:50 ragge Exp 	*/	
/*	$NetBSD: code.c,v 1.1.1.4.8.1 2014/08/19 23:52:09 tls Exp $	*/
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

# include "pass1.h"

/*
 * Print out assembler segment name.
 */
void
setseg(int seg, char *name)
{
	switch (seg) {
	case PROG: name = ".text"; break;

	case DATA:
	case LDATA: name = ".data"; break;

	case STRNG:
	case RDATA: name = ".section .rodata"; break;

	case UDATA: break;

	case DTORS:
		name = ".section .dtors,\"aw\",@progbits";
		break;
	case CTORS:
		name = ".section .ctors,\"aw\",@progbits";
		break;

	case TLSDATA:
	case TLSUDATA:
		uerror("FIXME: unsupported segment %d", seg);
		break;

	case PICRDATA:
		name = ".section .data.rel.ro.local,\"aw\",@progbits";
		break;

	case PICDATA:
		name = ".section .data.rel,\"aw\",@progbits";
		break;
	case PICLDATA:
		name = ".section .data.rel.local,\"aw\",@progbits";
		break;

	case NMSEG: 
		printf("\t.section %s,\"a%c\",@progbits\n", name,
		    cftnsp ? 'x' : 'w');
		return;
	}
	printf("\t%s\n", name);
}

/*
 * Define everything needed to print out some data (or text).
 * This means segment, alignment, visibility, etc.
 */
void
defloc(struct symtab *sp)
{
	char *name;

	if ((name = sp->soname) == NULL)
		name = exname(sp->sname);

	if (sp->sclass == EXTDEF) {
		printf("\t.globl %s\n", name);
		if (ISFTN(sp->stype)) {
			printf("\t.type %s,@function\n", name);
		} else {
			printf("\t.type %s,@object\n", name);
			printf("\t.size %s,%d\n", name,
			    (int)tsize(sp->stype, sp->sdf, sp->sap)/SZCHAR);
		}
	}
	if (sp->slevel == 0)
		printf("%s:\n", name);
	else
		printf(LABFMT ":\n", sp->soffset);
}

static int strtemp;

void
efcode(void)
{
	TWORD t;
	NODE *p, *q;

	/* code for the end of a function */
	if (cftnsp->stype != STRTY+FTN && cftnsp->stype != UNIONTY+FTN)
		return;

	t = PTR+BTYPE(cftnsp->stype);
	/* Create struct assignment */
	q = tempnode(strtemp, t, 0, cftnsp->sap);
	q = buildtree(UMUL, q, NIL);
	p = block(REG, NIL, NIL, t, 0, cftnsp->sap);
	regno(p) = R0;
	p = buildtree(UMUL, p, NIL);
	p = buildtree(ASSIGN, q, p);
	ecomp(p);

	/* put hidden arg in r0 on return */
	q = tempnode(strtemp, INT, 0, 0);
	p = block(REG, NIL, NIL, INT, 0, 0);
	regno(p) = R0;
        ecomp(buildtree(ASSIGN, p, q));
}

void
bfcode(struct symtab **sp, int n)
{
	struct symtab *sp2;
	NODE *p, *q;
	int i;

	if (cftnsp->stype == STRTY+FTN || cftnsp->stype == UNIONTY+FTN) {
		/* Move return address into temporary */
		p = tempnode(0, INT, 0, 0);
		strtemp = regno(p);
		q = block(REG, 0, 0, INT, 0, 0);
		regno(q) = R1;
		ecomp(buildtree(ASSIGN, p, q));
	}
	if (xtemps == 0)
		return;

	/* put arguments in temporaries */
	for (i = 0; i < n; i++) {
		if (sp[i]->stype == STRTY || sp[i]->stype == UNIONTY ||
		    cisreg(sp[i]->stype) == 0)
			continue;
		if (cqual(sp[i]->stype, sp[i]->squal) & VOL)
			continue;
		sp2 = sp[i];
		p = tempnode(0, sp[i]->stype, sp[i]->sdf, sp[i]->sap);
		p = buildtree(ASSIGN, p, nametree(sp2));
		sp[i]->soffset = regno(p->n_left);
		sp[i]->sflags |= STNODE;
		ecomp(p);
	}

}

void
ejobcode(int flag)
{
	/* called just before final exit */
	/* flag is 1 if errors, 0 if none */
}

void
bjobcode(void)
{
	astypnames[INT] = astypnames[UNSIGNED] = "\t.long";
	astypnames[SHORT] = astypnames[USHORT] = "\t.word";
}

#if 0
aobeg(void)
{
	/* called before removing automatics from stab */
}

aocode(struct symtab *p)
{
	/* called when automatic p removed from stab */
}

aoend(void)
{
	/* called after removing all automatics from stab */
}
#endif

void
fldty(struct symtab *p)
{
	/* fix up type of field p */
}

/*
 * XXX - fix genswitch.
 */
int
mygenswitch(int num, TWORD type, struct swents **p, int n)
{
	return 0;
}

#ifdef notyet
struct sw heapsw[SWITSZ];	/* heap for switches */

genswitch(register struct sw *p, int n)
{
	/*	p points to an array of structures, each consisting
		of a constant value and a label.
		The first is >=0 if there is a default label;
		its value is the label number
		The entries p[1] to p[n] are the nontrivial cases
		*/
	register i;
	register CONSZ j, range;
	register dlab, swlab;

	range = p[n].sval-p[1].sval;

	if( range>0 && range <= 3*n && n>=4 ){ /* implement a direct switch */

		swlab = getlab();
		dlab = p->slab >= 0 ? p->slab : getlab();

		/* already in r0 */
		printf("	casel	r0,$%ld,$%ld\n", p[1].sval, range);
		deflab1(swlab);
		for( i=1,j=p[1].sval; i<=n; j++) {
			printf("	.word	" LABFMT "-" LABFMT "\n",
			    (j == p[i].sval ? ((j=p[i++].sval), p[i-1].slab) : dlab),
				swlab);
			}

		if( p->slab >= 0 ) branch( dlab );
		else deflab1(dlab);
		return;

		}

	if( n>8 ) {	/* heap switch */

		heapsw[0].slab = dlab = p->slab >= 0 ? p->slab : getlab();
		makeheap(p, n, 1);	/* build heap */

		walkheap(1, n);	/* produce code */

		if( p->slab >= 0 )
			branch( dlab );
		else
			deflab1(dlab);
		return;
	}

	/* debugging code */

	/* out for the moment
	if( n >= 4 ) werror( "inefficient switch: %d, %d", n, (int) (range/n) );
	*/

	/* simple switch code */

	for( i=1; i<=n; ++i ){
		/* already in r0 */

		printf( "	cmpl	r0,$" );
		printf( CONFMT, p[i].sval );
		printf( "\n	jeql	" LBLFMT "\n", p[i].slab );
		}

	if( p->slab>=0 ) branch( p->slab );
}

makeheap(register struct sw *p, int m, int n)
{
	register int q;

	q = select(m);
	heapsw[n] = p[q];
	if( q>1 ) makeheap(p, q-1, 2*n);
	if( q<m ) makeheap(p+q, m-q, 2*n+1);
}

select(int m)
{
	register int l,i,k;

	for(i=1; ; i*=2)
		if( (i-1) > m ) break;
	l = ((k = i/2 - 1) + 1)/2;
	return( l + (m-k < l ? m-k : l));
}

walkheap(int start, int limit)
{
	int label;

	if( start > limit ) return;
	printf("	cmpl	r0,$%d\n",  heapsw[start].sval);
	printf("	jeql	" LBLFMT "\n", heapsw[start].slab);
	if( (2*start) > limit ) {
		printf("	jbr 	" LBLFMT "\n", heapsw[0].slab);
		return;
	}
	if( (2*start+1) <= limit ) {
		label = getlab();
		printf("	jgtr	" LBLFMT "\n", label);
	} else
		printf("	jgtr	" LBLFMT "\n", heapsw[0].slab);
	walkheap( 2*start, limit);
	if( (2*start+1) <= limit ) {
		deflab1(label);
		walkheap( 2*start+1, limit);
	}
}
#endif

/*
 * Called with a function call with arguments as argument.
 * This is done early in buildtree() and only done once.
 */
NODE *
funcode(NODE *p)
{
	NODE *r, *l;

	/* Fix function call arguments. On vax, just add funarg */
	for (r = p->n_right; r->n_op == CM; r = r->n_left) {
		if (r->n_right->n_op != STARG) {
			r->n_right = intprom(r->n_right);
			r->n_right = block(FUNARG, r->n_right, NIL,
			    r->n_right->n_type, r->n_right->n_df,
			    r->n_right->n_ap);
		}
	}
	if (r->n_op != STARG) {
		l = talloc();
		*l = *r;
		r->n_op = FUNARG;
		r->n_left = l;
		r->n_left = intprom(r->n_left);
		r->n_type = r->n_left->n_type;
	}
	return p;
}

/*
 * Generate the builtin code for FFS.
 */
NODE *
builtin_ffs(const struct bitable *bt, NODE *a)
{
	NODE *p, *q, *r;

	p = tempnode(0, bt->rt, 0, 0);
	r = block(XARG, ccopy(p), NIL, INT, 0, 0);
	r->n_name = "=&r";
	q = block(XARG, a, NIL, INT, 0, 0);
	q->n_name = "g";
	q = block(CM, r, q, INT, 0, 0);
	q = block(XASM, q, block(ICON, 0, 0, STRTY, 0, 0), INT, 0, 0);
	q->n_name = "ffs $0,$32,%1,%0;bneq 1f;mnegl $1,%0;1:;incl %0";
	p = block(COMOP, q, p, bt->rt, 0, 0);
	return p;
}

NODE *  
builtin_ffsl(const struct bitable *bt, NODE *a)
{       
	return builtin_ffs(bt, a);
}

NODE *  
builtin_ffsll(const struct bitable *bt, NODE *a)
{
	cerror("builtin_ffsll unimplemented");
	return NIL;
}

NODE *
builtin_return_address(const struct bitable *bt, NODE *a)
{
	NODE *f;

	if (a->n_op != ICON)
		goto bad;

	if (a->n_lval != 0)
		werror("unsupported argument");

	tfree(a);

	f = block(REG, NIL, NIL, INCREF(PTR+CHAR), 0, 0);
	regno(f) = FPREG;
	f = block(UMUL,
		block(PLUS, f,
		    bcon(16), INCREF(PTR+CHAR), 0, 0), NIL, PTR+CHAR, 0, 0);
	f = makety(f, PTR+VOID, 0, 0, 0);

	return f;
bad:
	uerror("bad argument to __builtin_return_address");
	return bcon(0);
}

NODE *
builtin_frame_address(const struct bitable *bt, NODE *a)
{
	int nframes;
	NODE *f;

	if (a->n_op != ICON)
		goto bad;

	nframes = a->n_lval;

	tfree(a);

	f = block(REG, NIL, NIL, PTR+CHAR, 0, 0);
	regno(f) = FPREG;

	while (nframes--) {
		f = block(UMUL,
			block(PLUS, f,
			    bcon(12), INCREF(PTR+CHAR), 0, 0),
				NIL, PTR+CHAR, 0, 0);
		f = makety(f, PTR+CHAR, 0, 0, 0);
	}

	return f;
bad:
	uerror("bad argument to __builtin_frame_address");
	return bcon(0);
}

/*
 * Return "canonical frame address".
 */
NODE *
builtin_cfa(const struct bitable *bt, NODE *a)
{
	uerror("missing builtin_cfa");
	return bcon(0);
}

