/*	Id: trees.c,v 1.330 2014/07/05 09:05:52 ragge Exp 	*/	
/*	$NetBSD: trees.c,v 1.1.1.6 2014/07/24 19:24:35 plunky Exp $	*/
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
/*
 * Some of the changes from 32V include:
 * - Understand "void" as type.
 * - Handle enums as ints everywhere.
 * - Convert some C-specific ops into branches.
 */

# include "pass1.h"
# include "pass2.h"

# include <stdarg.h>
# include <string.h>

static void chkpun(NODE *p);
static int opact(NODE *p);
static int moditype(TWORD);
static NODE *strargs(NODE *);
static void rmcops(NODE *p);
static NODE *tymatch(NODE *p);
static NODE *rewincop(NODE *p1, NODE *p2, int op);
void putjops(NODE *, void *);
static int has_se(NODE *p);
static struct symtab *findmember(struct symtab *, char *);
int inftn; /* currently between epilog/prolog */

static char *tnames[] = {
	"undef",
	"farg",
	"char",
	"unsigned char",
	"short",
	"unsigned short",
	"int",
	"unsigned int",
	"long",
	"unsigned long",
	"long long",
	"unsigned long long",
	"float",
	"double",
	"long double",
	"strty",
	"unionty",
	"enumty",
	"moety",
	"void",
	"signed", /* pass1 */
	"bool", /* pass1 */
	"fimag", /* pass1 */
	"dimag", /* pass1 */
	"limag", /* pass1 */
	"fcomplex", /* pass1 */
	"dcomplex", /* pass1 */
	"lcomplex", /* pass1 */
	"enumty", /* pass1 */
	"?", "?"
};

/*	some special actions, used in finding the type of nodes */
# define NCVT 01
# define PUN 02
# define TYPL 04
# define TYPR 010
# define TYMATCH 040
# define LVAL 0100
# define CVTO 0200
# define CVTL 0400
# define CVTR 01000
# define PTMATCH 02000
# define OTHER 04000
# define NCVTR 010000
# define PROML 020000	/* promote left operand */

/* node conventions:

	NAME:	rval>0 is stab index for external
		rval<0 is -inlabel number
		lval is offset in bits
	ICON:	lval has the value
		rval has the STAB index, or - label number,
			if a name whose address is in the constant
		rval = NONAME means no name
	REG:	rval is reg. identification cookie

	*/

extern int negrel[];

/* Have some defaults for most common targets */
#ifndef WORD_ADDRESSED
#define	offcon(o,t,d,ap) xbcon((o/SZCHAR), NULL, INTPTR)
#define	VBLOCK(p,b,t,d,a) buildtree(DIV, p, b)
#define	MBLOCK(p,b,t,d,a) buildtree(MUL, p, b)
#else
#define	VBLOCK(p,b,t,d,a) block(PVCONV, p, b, t, d, a)
#define	MBLOCK(p,b,t,d,a) block(PMCONV, p, b, t, d, a)
#endif

NODE *
buildtree(int o, NODE *l, NODE *r)
{
	NODE *p, *q;
	int actions;
	int opty, n;
	struct symtab *sp = NULL; /* XXX gcc */
	NODE *lr, *ll;

#ifdef PCC_DEBUG
	if (bdebug) {
		printf("buildtree(%s, %p, %p)\n", copst(o), l, r);
		if (l) fwalk(l, eprint, 0);
		if (r) fwalk(r, eprint, 0);
	}
#endif
	opty = coptype(o);

	/* check for constants */

	if (o == ANDAND || o == OROR || o == NOT) {
		if (l->n_op == FCON) {
			p = bcon(!FLOAT_ISZERO(l->n_dcon));
			nfree(l);
			l = p;
		}
		if (o != NOT && r->n_op == FCON) {
			p = bcon(!FLOAT_ISZERO(r->n_dcon));
			nfree(r);
			r = p;
		}
	}

	if( opty == UTYPE && l->n_op == ICON ){

		switch( o ){

		case NOT:
		case UMINUS:
		case COMPL:
			if( conval( l, o, l ) ) return(l);
			break;
		}
	} else if (o == NOT && l->n_op == FCON) {
		l = clocal(block(SCONV, l, NIL, INT, 0, 0));
	} else if( o == UMINUS && l->n_op == FCON ){
			l->n_dcon = FLOAT_NEG(l->n_dcon);
			return(l);

	} else if( o==QUEST &&
	    (l->n_op==ICON || (l->n_op==NAME && ISARY(l->n_type)))) {
		CONSZ c = l->n_lval;
		if (l->n_op==NAME)
			c = 1; /* will become constant later */
		nfree(l);
		if (c) {
			walkf(r->n_right, putjops, 0);
			tfree(r->n_right);
			l = r->n_left;
		} else {
			walkf(r->n_left, putjops, 0);
			tfree(r->n_left);
			l = r->n_right;
		}
		nfree(r);
		return(l);
	} else if( opty == BITYPE && l->n_op == ICON && r->n_op == ICON ){

		switch( o ){

		case PLUS:
		case MINUS:
		case MUL:
		case DIV:
		case MOD:
			/*
			 * Do type propagation for simple types here.
			 * The constant value is correct anyway.
			 * Maybe this op shortcut should be removed?
			 */
			if (l->n_sp == NULL && r->n_sp == NULL &&
			    l->n_type < BTMASK && r->n_type < BTMASK) {
				if (l->n_type > r->n_type)
					r->n_type = l->n_type;
				else
					l->n_type = r->n_type;
			}
			/* FALLTHROUGH */
		case ULT:
		case UGT:
		case ULE:
		case UGE:
		case LT:
		case GT:
		case LE:
		case GE:
		case EQ:
		case NE:
		case ANDAND:
		case OROR:
		case AND:
		case OR:
		case ER:
		case LS:
		case RS:
			if (!ISPTR(l->n_type) && !ISPTR(r->n_type)) {
				if( conval( l, o, r ) ) {
					nfree(r);
					return(l);
				}
			}
			break;
		}
	} else if (opty == BITYPE && (l->n_op == FCON || l->n_op == ICON) &&
	    (r->n_op == FCON || r->n_op == ICON) && (o == PLUS || o == MINUS ||
	    o == MUL || o == DIV || (o >= EQ && o <= GT) )) {
		TWORD t;
#ifndef CC_DIV_0
		if (o == DIV &&
		    ((r->n_op == ICON && r->n_lval == 0) ||
		     (r->n_op == FCON && r->n_dcon == 0.0)))
				goto runtime; /* HW dependent */
#endif
		if (l->n_op == ICON)
			l->n_dcon = FLOAT_CAST(l->n_lval, l->n_type);
		if (r->n_op == ICON)
			r->n_dcon = FLOAT_CAST(r->n_lval, r->n_type);
		switch(o){
		case PLUS:
		case MINUS:
		case MUL:
		case DIV:
			switch (o) {
			case PLUS:
				l->n_dcon = FLOAT_PLUS(l->n_dcon, r->n_dcon);
				break;
			case MINUS:
				l->n_dcon = FLOAT_MINUS(l->n_dcon, r->n_dcon);
				break;
			case MUL:
				l->n_dcon = FLOAT_MUL(l->n_dcon, r->n_dcon);
				break;
			case DIV:
				l->n_dcon = FLOAT_DIV(l->n_dcon, r->n_dcon);
				break;
			}
			t = (l->n_type > r->n_type ? l->n_type : r->n_type);
			l->n_op = FCON;
			l->n_type = t;
			nfree(r);
			return(l);
		case EQ:
		case NE:
		case LE:
		case LT:
		case GE:
		case GT:
			switch (o) {
			case EQ:
				n = FLOAT_EQ(l->n_dcon, r->n_dcon);
				break;
			case NE:
				n = FLOAT_NE(l->n_dcon, r->n_dcon);
				break;
			case LE:
				n = FLOAT_LE(l->n_dcon, r->n_dcon);
				break;
			case LT:
				n = FLOAT_LT(l->n_dcon, r->n_dcon);
				break;
			case GE:
				n = FLOAT_GE(l->n_dcon, r->n_dcon);
				break;
			case GT:
				n = FLOAT_GT(l->n_dcon, r->n_dcon);
				break;
			default:
				n = 0; /* XXX flow analysis */
			}
			nfree(r);
			nfree(l);
			return bcon(n);
		}
	} else if ((cdope(o)&ASGOPFLG) && o != RETURN && o != CAST) {
		/*
		 * Handle side effects by storing address in temporary q.
		 * Side effect nodes always have an UMUL.
		 */
		if (has_se(l)) {
			ll = l->n_left;

			q = cstknode(ll->n_type, ll->n_df, ll->n_ap);
			l->n_left = ccopy(q);
			q = buildtree(ASSIGN, q, ll);
		} else 
			q = bcon(0); /* No side effects */

		/*
		 * Modify the trees so that the compound op is rewritten.
		 */
		/* avoid casting of LHS */
		if ((cdope(o) & SIMPFLG) && ISINTEGER(l->n_type) && 
		    l->n_type != BOOL) 
			r = ccast(r, l->n_type, l->n_qual, l->n_df, l->n_ap);

		r = buildtree(UNASG o, ccopy(l), r);
		r = buildtree(ASSIGN, l, r);
		l = q;
		o = COMOP;
	} else if (o == INCR || o == DECR) {
		/*
		 * Rewrite to (t=d,d=d+1,t)
		 */
		if (has_se(l)) {
			ll = l->n_left;

			q = cstknode(ll->n_type, ll->n_df, ll->n_ap);
			l->n_left = ccopy(q);
			q = buildtree(ASSIGN, q, ll);
		} else 
			q = bcon(0); /* No side effects */

		/* Boolean has special syntax. */
		if (l->n_type == BOOL) {
			r = rewincop(l, r, o == INCR ? ASSIGN : EREQ);
		} else
			r = rewincop(l, r, o == INCR ? PLUSEQ : MINUSEQ);
		l = q;
		o = COMOP;
	}


#ifndef CC_DIV_0
runtime:
#endif
	/* its real; we must make a new node */

	p = block(o, l, r, INT, 0, 0);

	actions = opact(p);

	if (actions & PROML)
		p->n_left = intprom(p->n_left);

	if (actions & LVAL) { /* check left descendent */
		if (notlval(p->n_left)) {
			uerror("lvalue required");
			nfree(p);
			return l;
#ifdef notyet
		} else {
			if ((l->n_type > BTMASK && ISCON(l->n_qual)) ||
			    (l->n_type <= BTMASK && ISCON(l->n_qual << TSHIFT)))
				if (blevel > 0)
					uerror("lvalue is declared const");
#endif
		}
	}

	if( actions & NCVTR ){
		p->n_left = pconvert( p->n_left );
		}
	else if( !(actions & NCVT ) ){
		switch( opty ){

		case BITYPE:
			p->n_right = pconvert( p->n_right );
			/* FALLTHROUGH */
		case UTYPE:
			p->n_left = pconvert( p->n_left );

			}
		}

	if ((actions&PUN) && (o!=CAST))
		chkpun(p);

	if( actions & (TYPL|TYPR) ){

		q = (actions&TYPL) ? p->n_left : p->n_right;

		p->n_type = q->n_type;
		p->n_qual = q->n_qual;
		p->n_df = q->n_df;
		p->n_ap = q->n_ap;
		}

	if( actions & CVTL ) p = convert( p, CVTL );
	if( actions & CVTR ) p = convert( p, CVTR );
	if( actions & TYMATCH ) p = tymatch(p);
	if( actions & PTMATCH ) p = ptmatch(p);

	if( actions & OTHER ){
		struct symtab *sp1;

		l = p->n_left;
		r = p->n_right;

		switch(o){

		case NAME:
			cerror("buildtree NAME");

		case STREF:
			/* p->x turned into *(p+offset) */
			/* rhs must be a name; check correctness */

			/* Find member symbol struct */
			if (l->n_type != PTR+STRTY && l->n_type != PTR+UNIONTY){
				uerror("struct or union required");
				break;
			}

			if ((sp1 = strmemb(l->n_ap)) == NULL) {
				uerror("undefined struct or union");
				break;
			}

			if ((sp = findmember(sp1, r->n_name)) == NULL) {
				uerror("member '%s' not declared", r->n_name);
				break;
			}

			r->n_sp = sp;
			p = stref(p);
			break;

		case UMUL:
			if (l->n_op == ADDROF) {
				nfree(p);
				p = nfree(l);
			}
			if( !ISPTR(l->n_type))uerror("illegal indirection");
			p->n_type = DECREF(l->n_type);
			p->n_qual = DECREF(l->n_qual);
			p->n_df = l->n_df;
			p->n_ap = l->n_ap;
			break;

		case ADDROF:
			switch( l->n_op ){

			case UMUL:
				nfree(p);
				p = nfree(l);
				/* FALLTHROUGH */
			case TEMP:
			case NAME:
				p->n_type = INCREF(l->n_type);
				p->n_qual = INCQAL(l->n_qual);
				p->n_df = l->n_df;
				p->n_ap = l->n_ap;
				break;

			case COMOP:
				nfree(p);
				lr = buildtree(ADDROF, l->n_right, NIL);
				p = buildtree( COMOP, l->n_left, lr );
				nfree(l);
				break;

			case QUEST:
				lr = buildtree( ADDROF, l->n_right->n_right, NIL );
				ll = buildtree( ADDROF, l->n_right->n_left, NIL );
				nfree(p); nfree(l->n_right);
				p = buildtree( QUEST, l->n_left, buildtree( COLON, ll, lr ) );
				nfree(l);
				break;

			default:
				uerror("unacceptable operand of &: %d", l->n_op );
				break;
				}
			break;

		case LS:
		case RS: /* must make type size at least int... */
			if (p->n_type == CHAR || p->n_type == SHORT) {
				p->n_left = makety(l, INT, 0, 0, 0);
			} else if (p->n_type == UCHAR || p->n_type == USHORT) {
				p->n_left = makety(l, UNSIGNED, 0, 0, 0);
			}
			l = p->n_left;
			p->n_type = l->n_type;
			p->n_qual = l->n_qual;
			p->n_df = l->n_df;
			p->n_ap = l->n_ap;
			if(tsize(r->n_type, r->n_df, r->n_ap) > SZINT)
				p->n_right = makety(r, INT, 0, 0, 0);
			break;

		case RETURN:
		case ASSIGN:
		case CAST:
			/* structure assignment */
			/* take the addresses of the two sides; then make an
			 * operator using STASG and
			 * the addresses of left and right */

			if (strmemb(l->n_ap) != strmemb(r->n_ap))
				uerror("assignment of different structures");

			r = buildtree(ADDROF, r, NIL);

			l = block(STASG, l, r, r->n_type, r->n_df, r->n_ap);
			l = clocal(l);

			if( o == RETURN ){
				nfree(p);
				p = l;
				break;
			}

			p->n_op = UMUL;
			p->n_left = l;
			p->n_right = NIL;
			break;

		case QUEST: /* fixup types of : */
			if (r->n_left->n_type != p->n_type)
				r->n_left = makety(r->n_left, p->n_type,
				    p->n_qual, p->n_df, p->n_ap);
			if (r->n_right->n_type != p->n_type)
				r->n_right = makety(r->n_right, p->n_type,
				    p->n_qual, p->n_df, p->n_ap);
			break;

		case COLON:
			/* structure colon */

			if (strmemb(l->n_ap) != strmemb(r->n_ap))
				uerror( "type clash in conditional" );
			break;

		case CALL:
			p->n_right = r = strargs(p->n_right);
			p = funcode(p);
			/* FALLTHROUGH */
		case UCALL:
			if (!ISPTR(l->n_type))
				uerror("illegal function");
			p->n_type = DECREF(l->n_type);
			if (!ISFTN(p->n_type))
				uerror("illegal function");
			p->n_type = DECREF(p->n_type);
			p->n_df = l->n_df+1; /* add one for prototypes */
			p->n_ap = l->n_ap;
			if (p->n_type == STRTY || p->n_type == UNIONTY) {
				/* function returning structure */
				/*  make function really return ptr to str., with * */

				p->n_op += STCALL-CALL;
				p->n_type = INCREF(p->n_type);
				p = clocal(p); /* before recursing */
				p = buildtree(UMUL, p, NIL);

				}
			break;

		default:
			cerror( "other code %d", o );
			}

		}

	/*
	 * Allow (void)0 casts.
	 * XXX - anything on the right side must be possible to cast.
	 * XXX - remove void types further on.
	 */
	if (p->n_op == CAST && p->n_type == VOID &&
	    p->n_right->n_op == ICON)
		p->n_right->n_type = VOID;

	if (actions & CVTO)
		p = oconvert(p);
	p = clocal(p);

#ifdef PCC_DEBUG
	if (bdebug) {
		printf("End of buildtree:\n");
		fwalk(p, eprint, 0);
	}
#endif

	return(p);

	}

/*                      
 * Rewrite ++/-- to (t=p, p++, t) ops on types that do not act act as usual.
 */
static NODE *
rewincop(NODE *p1, NODE *p2, int op)
{
	NODE *t, *r;
		
	t = cstknode(p1->n_type, p1->n_df, p1->n_ap);
	r = buildtree(ASSIGN, ccopy(t), ccopy(p1));
	r = buildtree(COMOP, r, buildtree(op, p1, eve(p2)));
	return buildtree(COMOP, r, t);
}


/* Find a member in a struct or union.	May be an unnamed member */
static struct symtab *
findmember(struct symtab *sp, char *s)
{
	struct symtab *sp2, *sp3;

	for (; sp != NULL; sp = sp->snext) {
		if (sp->sname[0] == '*') {
			/* unnamed member, recurse down */
			if ((sp2 = findmember(strmemb(sp->sap), s))) {
				sp3 = tmpalloc(sizeof (struct symtab));
				*sp3 = *sp2;
				sp3->soffset += sp->soffset;
				return sp3;
			}
		} else if (sp->sname == s)
			return sp;
	}
	return NULL;
}


/*
 * Check if there will be a lost label destination inside of a ?:
 * It cannot be reached so just print it out.
 */
void
putjops(NODE *p, void *arg)
{
	if (p->n_op == COMOP && p->n_left->n_op == GOTO)
		plabel((int)p->n_left->n_left->n_lval+1);
}

/*
 * Build a name node based on a symtab entry.
 * broken out from buildtree().
 */
NODE *
nametree(struct symtab *sp)
{
	NODE *p;

	p = block(NAME, NIL, NIL, sp->stype, sp->sdf, sp->sap);
	p->n_qual = sp->squal;
	p->n_sp = sp;

#ifndef NO_C_BUILTINS
	if (sp->sname[0] == '_' && strncmp(sp->sname, "__builtin_", 10) == 0)
		return p;  /* do not touch builtins here */
	
#endif

	if (sp->sflags & STNODE) {
		/* Generated for optimizer */
		p->n_op = TEMP;
		p->n_rval = sp->soffset;
	}

#ifdef GCC_COMPAT
	/* Get a label name */
	if (sp->sflags == SLBLNAME)
		sp->stype = p->n_type = VOID;
#endif
	if (sp->stype == UNDEF) {
		uerror("%s undefined", sp->sname);
		/* make p look reasonable */
		p->n_type = INT;
		p->n_df = NULL;
		defid(p, SNULL);
	}
	if (sp->sclass == MOE) {
		p->n_op = ICON;
		p->n_lval = sp->soffset;
		p->n_df = NULL;
		p->n_sp = NULL;
	}
	return clocal(p);
}

/*
 * Cast a node to another type by inserting a cast.
 * Just a nicer interface to buildtree.
 * Returns the new tree.
 */
NODE *
cast(NODE *p, TWORD t, TWORD u)
{
	NODE *q;

	q = block(NAME, NIL, NIL, t, 0, 0);
	q->n_qual = u;
	q = buildtree(CAST, q, p);
	p = q->n_right;
	nfree(q->n_left);
	nfree(q);
	return p;
}

/*
 * Cast and complain if necessary by not inserining a cast.
 */
NODE *
ccast(NODE *p, TWORD t, TWORD u, union dimfun *df, struct attr *ap)
{
	NODE *q;

	/* let buildtree do typechecking (and casting) */ 
	q = block(NAME, NIL, NIL, t, df, ap);
	p = buildtree(ASSIGN, q, p);
	nfree(p->n_left);
	q = optim(p->n_right);
	nfree(p);
	return q;
}

/*
 * Do an actual cast of a constant (if possible).
 * Routine assumes 2-complement (is there anything else today?)
 * Returns 1 if handled, 0 otherwise.
 */
int
concast(NODE *p, TWORD t)
{
	extern short sztable[];
	CONSZ val;

	if (p->n_op != ICON && p->n_op != FCON) /* only constants */
		return 0;
	if (p->n_op == ICON && p->n_sp != NULL) { /* no addresses */
		if (t == BOOL) {
			p->n_lval = 1, p->n_type = BOOL, p->n_sp = NULL;
			return 1;
		}
		return 0;
	}
	if (((p->n_type & TMASK) && t != BOOL) || (t & TMASK)) /* no pointers */
		return 0;

//printf("concast till %d\n", t);
//fwalk(p, eprint, 0);

#define	TYPMSK(y) ((((1LL << (y-1))-1) << 1) | 1)
	if (p->n_op == ICON) {
		val = p->n_lval;

		if (t == BOOL) {
			if (val)
				p->n_lval = 1;
		} else if (t <= ULONGLONG) {
			p->n_lval = val & TYPMSK(sztable[t]);
			if (!ISUNSIGNED(t)) {
				if (val & (1LL << (sztable[t]-1)))
					p->n_lval |= ~TYPMSK(sztable[t]);
			}
		} else if (t <= LDOUBLE) {
			p->n_op = FCON;
			p->n_dcon = FLOAT_CAST(val, p->n_type);
		}
	} else { /* p->n_op == FCON */
		if (t == BOOL) {
			p->n_op = ICON;
			p->n_lval = FLOAT_NE(p->n_dcon,0.0);
			p->n_sp = NULL;
		} else if (t <= ULONGLONG) {
			p->n_op = ICON;
			p->n_lval = ISUNSIGNED(t) ? /* XXX FIXME */
			    ((U_CONSZ)p->n_dcon) : p->n_dcon;
			p->n_sp = NULL;
		} else {
			p->n_dcon = t == FLOAT ? (float)p->n_dcon :
			    t == DOUBLE ? (double)p->n_dcon : p->n_dcon;
		}
	}
	p->n_type = t;
//fwalk(p, eprint, 0);
	return 1;
}

/*
 * Do a conditional branch.
 */
void
cbranch(NODE *p, NODE *q)
{
	p = buildtree(CBRANCH, p, q);
	if (p->n_left->n_op == ICON) {
		if (p->n_left->n_lval != 0) {
			branch((int)q->n_lval); /* branch always */
			reached = 0;
		}
		tfree(p);
		tfree(q);
		return;
	}
	ecomp(p);
}

NODE *
strargs(register NODE *p)
{
	/* rewrite structure flavored arguments */

	if( p->n_op == CM ){
		p->n_left = strargs( p->n_left );
		p->n_right = strargs( p->n_right );
		return( p );
		}

	if( p->n_type == STRTY || p->n_type == UNIONTY ){
		p = block(STARG, p, NIL, p->n_type, p->n_df, p->n_ap);
		p->n_left = buildtree( ADDROF, p->n_left, NIL );
		p = clocal(p);
		}
	return( p );
}

/*
 * apply the op o to the lval part of p; if binary, rhs is val
 */
int
conval(NODE *p, int o, NODE *q)
{
	TWORD tl = p->n_type, tr = q->n_type, td;
	int i, u;
	CONSZ val;
	U_CONSZ v1, v2;

	val = q->n_lval;

	/* make both sides same type */
	if (tl < BTMASK && tr < BTMASK) {
		td = tl > tr ? tl : tr;
		if (td < INT)
			td = INT;
		u = ISUNSIGNED(td);
		if (tl != td)
			p = makety(p, td, 0, 0, 0);
		if (tr != td)
			q = makety(q, td, 0, 0, 0);
	} else
		u = ISUNSIGNED(tl) || ISUNSIGNED(tr);
	if( u && (o==LE||o==LT||o==GE||o==GT)) o += (UGE-GE);

	if (p->n_sp != NULL && q->n_sp != NULL)
		return(0);
	if (q->n_sp != NULL && o != PLUS)
		return(0);
	if (p->n_sp != NULL && o != PLUS && o != MINUS)
		return(0);

	v1 = p->n_lval;
	v2 = q->n_lval;
	if (v2 == 0 && (cdope(o) & DIVFLG))
		return 0; /* leave division by zero to runtime */
	switch( o ){

	case PLUS:
		p->n_lval += val;
		if (p->n_sp == NULL) {
			p->n_right = q->n_right;
			p->n_type = q->n_type;
		}
		break;
	case MINUS:
		p->n_lval -= val;
		break;
	case MUL:
		p->n_lval *= val;
		break;
	case DIV:
		if (u) {
			v1 /= v2;
			p->n_lval = v1;
		} else
			p->n_lval /= val;
		break;
	case MOD:
		if (u) {
			v1 %= v2;
			p->n_lval = v1;
		} else
			p->n_lval %= val;
		break;
	case AND:
		p->n_lval &= val;
		break;
	case OR:
		p->n_lval |= val;
		break;
	case ER:
		p->n_lval ^= val;
		break;
	case LS:
		i = (int)val;
		p->n_lval = p->n_lval << i;
		break;
	case RS:
		i = (int)val;
		if (u) {
			v1 = v1 >> i;
			p->n_lval = v1;
		} else
			p->n_lval = p->n_lval >> i;
		break;

	case UMINUS:
		p->n_lval = - p->n_lval;
		break;
	case COMPL:
		p->n_lval = ~p->n_lval;
		break;
	case NOT:
		p->n_lval = !p->n_lval;
		p->n_type = INT;
		break;
	case LT:
		p->n_lval = p->n_lval < val;
		break;
	case LE:
		p->n_lval = p->n_lval <= val;
		break;
	case GT:
		p->n_lval = p->n_lval > val;
		break;
	case GE:
		p->n_lval = p->n_lval >= val;
		break;
	case ULT:
		p->n_lval = v1 < v2;
		break;
	case ULE:
		p->n_lval = v1 <= v2;
		break;
	case UGT:
		p->n_lval = v1 > v2;
		break;
	case UGE:
		p->n_lval = v1 >= v2;
		break;
	case EQ:
		p->n_lval = p->n_lval == val;
		break;
	case NE:
		p->n_lval = p->n_lval != val;
		break;
	case ANDAND:
		p->n_lval = p->n_lval && val;
		break;
	case OROR:
		p->n_lval = p->n_lval || val;
		break;
	default:
		return(0);
		}
	/* Do the best in making everything type correct after calc */
	if (clogop(o))
		p->n_type = INT;
	if (p->n_sp == NULL && q->n_sp == NULL)
		p->n_lval = valcast(p->n_lval, p->n_type);
	return(1);
	}

/*
 * Ensure that v matches the type t; sign- or zero-extended
 * as suitable to CONSZ.
 * Only to be used for integer types.
 */
CONSZ
valcast(CONSZ v, TWORD t)
{
	CONSZ r;
	int sz;

	if (t < CHAR || t > ULONGLONG)
		return v; /* cannot cast */

	if (t >= LONGLONG)
		return v; /* already largest */

#define M(x)	((((1ULL << ((x)-1)) - 1) << 1) + 1)
#define	NOTM(x)	(~M(x))
#define	SBIT(x)	(1ULL << ((x)-1))

	sz = (int)tsize(t, NULL, NULL);
	r = v & M(sz);
	if (!ISUNSIGNED(t) && (SBIT(sz) & r))
		r = r | NOTM(sz);
	return r;
}

/*
 * Checks p for the existence of a pun.  This is called when the op of p
 * is ASSIGN, RETURN, CAST, COLON, or relational.
 * One case is when enumerations are used: this applies only to lint.
 * In the other case, one operand is a pointer, the other integer type
 * we check that this integer is in fact a constant zero...
 * in the case of ASSIGN, any assignment of pointer to integer is illegal
 * this falls out, because the LHS is never 0.
 * XXX - check for COMOPs in assignment RHS?
 */
void
chkpun(NODE *p)
{
	union dimfun *d1, *d2;
	NODE *q;
	int t1, t2;

	t1 = p->n_left->n_type;
	t2 = p->n_right->n_type;

	switch (p->n_op) {
	case RETURN:
		/* return of void allowed but nothing else */
		if (t1 == VOID && t2 == VOID)
			return;
		if (t1 == VOID) {
			werror("returning value from void function");
			return;
		}
		if (t2 == VOID) {
			uerror("using void value");
			return;
		}
		break;
	case COLON:
		if (t1 == VOID && t2 == VOID)
			return;
		break;
	default:
		if ((t1 == VOID && t2 != VOID) || (t1 != VOID && t2 == VOID)) {
			uerror("value of void expression used");
			return;
		}
		break;
	}

	/* allow void pointer assignments in any direction */
	if (BTYPE(t1) == VOID && (t2 & TMASK))
		return;
	if (BTYPE(t2) == VOID && (t1 & TMASK))
		return;

	/* boolean have special syntax */
	if (t1 == BOOL) {
		if (!ISARY(t2)) /* Anything scalar */
			return;
	}

	if (ISPTR(t1) || ISARY(t1))
		q = p->n_right;
	else
		q = p->n_left;

	if (!ISPTR(q->n_type) && !ISARY(q->n_type)) {
		if (q->n_op != ICON || q->n_lval != 0)
			werror("illegal combination of pointer and integer");
	} else {
		if (t1 == t2) {
			if (ISSOU(BTYPE(t1)) &&
			    !suemeq(p->n_left->n_ap, p->n_right->n_ap))
				werror("illegal structure pointer combination");
			return;
		}
		d1 = p->n_left->n_df;
		d2 = p->n_right->n_df;
		for (;;) {
			if (ISARY(t1) || ISPTR(t1)) {
				if (!ISARY(t2) && !ISPTR(t2))
					break;
				if (ISARY(t1) && ISARY(t2) && d1->ddim != d2->ddim) {
					werror("illegal array size combination");
					return;
				}
				if (ISARY(t1))
					++d1;
				if (ISARY(t2))
					++d2;
			} else if (ISFTN(t1)) {
				if (chkftn(d1->dfun, d2->dfun)) {
					werror("illegal function "
					    "pointer combination");
					return;
				}
				++d1;
				++d2;
			} else
				break;
			t1 = DECREF(t1);
			t2 = DECREF(t2);
		}
		if (DEUNSIGN(t1) != DEUNSIGN(t2))
			warner(Wpointer_sign, NULL);
	}
}

static NODE *
offplus(NODE *p, int off, TWORD t, TWORD q, union dimfun *d, struct attr *ap) {
	if (off != 0) {
		p = block(PLUS, p, offcon(off, t, d, ap), t, d, ap);
		p->n_qual = q;
		p = optim(p);
	}

	return buildtree(UMUL, p, NIL);
}

NODE *
stref(NODE *p)
{
	NODE *r;
	struct attr *ap, *xap, *yap;
	union dimfun *d;
	TWORD t, q;
	int dsc;
	OFFSZ off;
	struct symtab *s;

	/* make p->x */
	/* this is also used to reference automatic variables */

	s = p->n_right->n_sp;
	nfree(p->n_right);
	r = nfree(p);
#ifdef GCC_COMPAT
	xap = attr_find(r->n_ap, GCC_ATYP_PACKED);
#else
	xap = NULL;
#endif

	p = pconvert(r);

	/* make p look like ptr to x */

	if (!ISPTR(p->n_type))
		p->n_type = PTR+UNIONTY;

	t = INCREF(s->stype);
	q = INCQAL(s->squal);
	d = s->sdf;
	ap = s->sap;
#ifdef GCC_COMPAT
	if ((yap = attr_find(ap, GCC_ATYP_PACKED)) != NULL)
		xap = yap;
	else if (xap != NULL)
		ap = attr_add(ap, attr_dup(xap, 3));
	/* xap set if packed struct */
#else
	yap = NULL;
#endif

	p = makety(p, t, q, d, ap);

	/* compute the offset to be added */

	off = s->soffset;
	dsc = s->sclass;

	if (dsc & FIELD) {
		TWORD ftyp = s->stype;
		int fal = talign(ftyp, ap);
		off = (off/fal)*fal;
		p = offplus(p, off, t, q, d, ap);
		p = block(FLD, p, NIL, ftyp, 0, ap);
		p->n_qual = q;
		p->n_rval = PKFIELD(dsc&FLDSIZ, s->soffset%fal);
	} else {
		p = offplus(p, off, t, q, d, ap);
#ifndef CAN_UNALIGN
		/* if target cannot handle unaligned addresses, fix here */
#endif
	}

	p = clocal(p);
	return p;
}

int
notlval(register NODE *p)
{
	/* return 0 if p an lvalue, 1 otherwise */

	again:

	switch( p->n_op ){

	case FLD:
		p = p->n_left;
		goto again;

	case NAME:
	case OREG:
	case UMUL:
		if( ISARY(p->n_type) || ISFTN(p->n_type) ) return(1);
		/* FALLTHROUGH */
	case TEMP:
	case REG:
		return(0);

	default:
		return(1);
	}
}

/* make a constant node with value i */
NODE *
bcon(int i)
{
	return xbcon(i, NULL, INT);
}

NODE *
xbcon(CONSZ val, struct symtab *sp, TWORD type)
{
	NODE *p;

	p = block(ICON, NIL, NIL, type, 0, 0);
	p->n_lval = val;
	p->n_sp = sp;
	return clocal(p);
}

NODE *
bpsize(NODE *p)
{
	int isdyn(struct symtab *sp);
	struct symtab s;
	NODE *q, *r;
	TWORD t;
	int sz;

	s.stype = DECREF(p->n_type);
	s.sdf = p->n_df;
	if (isdyn(&s)) {
		q = bcon(1);
		for (t = s.stype; t > BTMASK; t = DECREF(t)) {
			if (ISPTR(t))
				return buildtree(MUL, q, bcon(SZPOINT(t)));
			if (ISARY(t)) {
				if (s.sdf->ddim < 0)
					r = tempnode(-s.sdf->ddim, INT, 0, 0);
				else
					r = bcon(s.sdf->ddim/SZCHAR);
				q = buildtree(MUL, q, r);
				s.sdf++;
			}
		}
		sz = (int)tsize(t, s.sdf, p->n_ap);
		p = buildtree(MUL, q, bcon(sz/SZCHAR));
	} else
		p = (offcon(psize(p), p->n_type, p->n_df, p->n_ap));
	return p;
}

/*
 * p is a node of type pointer; psize returns the
 * size of the thing pointed to
 */
OFFSZ
psize(NODE *p)
{

	if (!ISPTR(p->n_type)) {
		uerror("pointer required");
		return(SZINT);
	}
	/* note: no pointers to fields */
	return(tsize(DECREF(p->n_type), p->n_df, p->n_ap));
}

/*
 * convert an operand of p
 * f is either CVTL or CVTR
 * operand has type int, and is converted by the size of the other side
 * convert is called when an integer is to be added to a pointer, for
 * example in arrays or structures.
 */
NODE *
convert(NODE *p, int f)
{
	union dimfun *df;
	TWORD ty, ty2;
	NODE *q, *r, *s, *rv;

	if (f == CVTL) {
		q = p->n_left;
		s = p->n_right;
	} else {
		q = p->n_right;
		s = p->n_left;
	}
	ty2 = ty = DECREF(s->n_type);
	while (ISARY(ty))
		ty = DECREF(ty);

	r = offcon(tsize(ty, s->n_df, s->n_ap), s->n_type, s->n_df, s->n_ap);
	ty = ty2;
	rv = bcon(1);
	df = s->n_df;
	while (ISARY(ty)) {
		rv = buildtree(MUL, rv, df->ddim >= 0 ? bcon(df->ddim) :
		    tempnode(-df->ddim, INT, 0, 0));
		df++;
		ty = DECREF(ty);
	}
	rv = clocal(MBLOCK(rv, r, INT, 0, 0));
	rv = optim(rv);

	r = MBLOCK(q, rv, INT, 0, 0);
	r = clocal(r);
	/*
	 * Indexing is only allowed with integer arguments, so insert
	 * SCONV here if arg is not an integer.
	 * XXX - complain?
	 */
	if (r->n_type != INTPTR)
		r = clocal(makety(r, INTPTR, 0, 0, 0));
	if (f == CVTL)
		p->n_left = r;
	else
		p->n_right = r;
	return(p);
}

NODE *
pconvert(register NODE *p)
{
	/* if p should be changed into a pointer, do so */

	if( ISARY( p->n_type) ){
		p->n_type = DECREF( p->n_type );
		++p->n_df;
		return( buildtree( ADDROF, p, NIL ) );
	}
	if( ISFTN( p->n_type) )
		return( buildtree( ADDROF, p, NIL ) );

	return( p );
}

NODE *
oconvert(register NODE *p)
{
	/* convert the result itself: used for pointer and unsigned */

	switch(p->n_op) {

	case LE:
	case LT:
	case GE:
	case GT:
		if(ISUNSIGNED(p->n_left->n_type) ||
		    ISUNSIGNED(p->n_right->n_type) ||
		    ISPTR(p->n_left->n_type) ||
		    ISPTR(p->n_right->n_type))
			 p->n_op += (ULE-LE);
		/* FALLTHROUGH */
	case EQ:
	case NE:
		return( p );

	case MINUS:
		p->n_type = INTPTR;
		p->n_ap = NULL;
		return(clocal(VBLOCK(p, bpsize(p->n_left), INT, 0, 0)));
		}

	cerror( "illegal oconvert: %d", p->n_op );

	return(p);
}

/*
 * makes the operands of p agree; they are
 * either pointers or integers, by this time
 * with MINUS, the sizes must be the same
 * with COLON, the types must be the same
 */
NODE *
ptmatch(NODE *p)
{
	struct attr *ap, *ap2;
	union dimfun *d, *d2;
	TWORD t1, t2, t, q1, q2, q;
	int o;

	o = p->n_op;
	t = t1 = p->n_left->n_type;
	q = q1 = p->n_left->n_qual;
	t2 = p->n_right->n_type;
	q2 = p->n_right->n_qual;
	d = p->n_left->n_df;
	d2 = p->n_right->n_df;
	ap = p->n_left->n_ap;
	ap2 = p->n_right->n_ap;

	switch( o ){

	case ASSIGN:
	case RETURN:
		{  break; }

	case CAST:
		if (t == VOID) {
			/* just paint over */
			p->n_right = block(SCONV, p->n_right, NIL, VOID, 0, 0);
			return p;
		}
		break;

	case MINUS: {
		int isdyn(struct symtab *sp);
		struct symtab s1, s2;

		s1.stype = DECREF(t);
		s1.sdf = d;
		s2.stype = DECREF(t2);
		s2.sdf = d2;
		if (isdyn(&s1) || isdyn(&s2))
			; /* We don't know */
		else if (psize(p->n_left) != psize(p->n_right))
			uerror("illegal pointer subtraction");
		break;
		}

	case COLON:
		if (t1 != t2) {
			/*
			 * Check for void pointer types. They are allowed
			 * to cast to/from any pointers.
			 */
			if (ISPTR(t1) && ISPTR(t2) &&
			    (BTYPE(t1) == VOID || BTYPE(t2) == VOID))
				break;
			uerror("illegal types in :");
		}
		break;

	default:  /* must work harder: relationals or comparisons */

		if( !ISPTR(t1) ){
			t = t2;
			q = q2;
			d = d2;
			ap = ap2;
			break;
			}
		if( !ISPTR(t2) ){
			break;
			}

		/* both are pointers */
		if( talign(t2,ap2) < talign(t,ap) ){
			t = t2;
			q = q2;
			ap = ap2;
			}
		break;
		}

	p->n_left = makety( p->n_left, t, q, d, ap );
	p->n_right = makety( p->n_right, t, q, d, ap );
	if( o!=MINUS && !clogop(o) ){

		p->n_type = t;
		p->n_qual = q;
		p->n_df = d;
		p->n_ap = ap;
		}

	return(clocal(p));
}

/*
 * Satisfy the types of various arithmetic binary ops.
 *
 * rules are:
 *  if assignment, type of LHS
 *  if any doubles, make double
 *  else if any float make float
 *  else if any longlongs, make long long
 *  else if any longs, make long
 *  else etcetc.
 *
 *  If the op with the highest rank is unsigned, this is the resulting type.
 *  See:  6.3.1.1 rank order equal of signed and unsigned types
 *        6.3.1.8 Usual arithmetic conversions
 */
static NODE *
tymatch(NODE *p)
{
	TWORD tl, tr, t;
	NODE *l, *r;
	int o;

	o = p->n_op;
	r = p->n_right;
	l = p->n_left;

	tl = l->n_type;
	tr = r->n_type;

	if (tl == BOOL) tl = BOOL_TYPE;
	if (tr == BOOL) tr = BOOL_TYPE;

	if (casgop(o)) {
		if (r->n_op != ICON && tl < FLOAT && tr < FLOAT &&
		    DEUNSIGN(tl) < DEUNSIGN(tr) && o != CAST)
			warner(Wtruncate, tnames[tr], tnames[tl]);
		p->n_right = makety(p->n_right, l->n_type, 0, 0, 0);
		t = p->n_type = l->n_type;
		p->n_ap = l->n_ap;
	} else {
		t = tl > tr ? tl : tr; /* MAX */
		/* This depends on ctype() called early */
		if (o != COLON && t < INT)
			t = INT;
		if (tl != t) p->n_left = makety(p->n_left, t, 0, 0, 0);
		if (tr != t) p->n_right = makety(p->n_right, t, 0, 0, 0);
		if (o == COLON && l->n_type == BOOL && r->n_type == BOOL)
			t = p->n_type = BOOL;
		else if (!clogop(o))
			p->n_type = t;
	}
#ifdef PCC_DEBUG
	if (tdebug) {
		printf("tymatch(%p): ", p);
		tprint(tl, 0);
		printf(" %s ", copst(o));
		tprint(tr, 0);
		printf(" => ");
		tprint(t, 0);
		printf("\n");
		fwalk(p, eprint, 0);
	}
#endif
	return p;
}

/*
 * make p into type t by inserting a conversion
 */
NODE *
makety(NODE *p, TWORD t, TWORD q, union dimfun *d, struct attr *ap)
{

	if (t == p->n_type) {
		p->n_df = d;
		p->n_ap = ap;
		p->n_qual = q;
		return(p);
	}

	if (ISITY(t) || ISCTY(t) || ISITY(p->n_type) || ISCTY(p->n_type))
		cerror("makety");

	if (concast(p, t))
		return clocal(p);

	p = block(t & TMASK ? PCONV : SCONV, p, NIL, t, d, ap);
	p->n_qual = q;
	return clocal(p);
}

NODE *
block(int o, NODE *l, NODE *r, TWORD t, union dimfun *d, struct attr *ap)
{
	register NODE *p;

	p = talloc();
	p->n_rval = 0;
	p->n_op = o;
	p->n_lval = 0; /* Protect against large lval */
	p->n_left = l;
	p->n_right = r;
	p->n_type = t;
	p->n_qual = 0;
	p->n_df = d;
	p->n_ap = ap;
#if !defined(MULTIPASS)
	/* p->n_reg = */p->n_su = 0;
	p->n_regw = 0;
#endif
	return(p);
}

/*
 * Return the constant value from an ICON.
 */
CONSZ
icons(NODE *p)
{
	/* if p is an integer constant, return its value */
	CONSZ val;

	if (p->n_op != ICON || p->n_sp != NULL) {
		uerror( "constant expected");
		val = 1;
	} else
		val = p->n_lval;
	tfree(p);
	return(val);
}

/* 
 * the intent of this table is to examine the
 * operators, and to check them for
 * correctness.
 * 
 * The table is searched for the op and the
 * modified type (where this is one of the
 * types INT (includes char and short), LONG,
 * DOUBLE (includes FLOAT), and POINTER
 * 
 * The default action is to make the node type integer
 * 
 * The actions taken include:
 * 	PUN	  check for puns
 * 	CVTL	  convert the left operand
 * 	CVTR	  convert the right operand
 * 	TYPL	  the type is determined by the left operand
 * 	TYPR	  the type is determined by the right operand
 * 	TYMATCH	  force type of left and right to match,by inserting conversions
 * 	PTMATCH	  like TYMATCH, but for pointers
 * 	LVAL	  left operand must be lval
 * 	CVTO	  convert the op
 * 	NCVT	  do not convert the operands
 * 	OTHER	  handled by code
 * 	NCVTR	  convert the left operand, not the right...
 * 
 */

# define MINT 01	/* integer */
# define MDBI 02	/* integer or double */
# define MSTR 04	/* structure */
# define MPTR 010	/* pointer */
# define MPTI 020	/* pointer or integer */

int
opact(NODE *p)
{
	int mt12, mt1, mt2, o;

	mt1 = mt2 = mt12 = 0;

	switch (coptype(o = p->n_op)) {
	case BITYPE:
		mt12=mt2 = moditype(p->n_right->n_type);
		/* FALLTHROUGH */
	case UTYPE:
		mt12 &= (mt1 = moditype(p->n_left->n_type));
		break;
	}

	switch( o ){

	case NAME :
	case ICON :
	case FCON :
	case CALL :
	case UCALL:
	case UMUL:
		{  return( OTHER ); }
	case UMINUS:
		if( mt1 & MDBI ) return( TYPL+PROML );
		break;

	case COMPL:
		if( mt1 & MINT ) return( TYPL+PROML );
		break;

	case ADDROF:
		return( NCVT+OTHER );
	case NOT:
		return( PROML );

/*	case INIT: */
	case CM:
	case CBRANCH:
	case ANDAND:
	case OROR:
		return( 0 );

	case MUL:
	case DIV:
		if( mt12 & MDBI ) return( TYMATCH );
		break;

	case MOD:
	case AND:
	case OR:
	case ER:
		if( mt12 & MINT ) return( TYMATCH );
		break;

	case LS:
	case RS:
		if( mt12 & MINT ) return( TYPL+OTHER );
		break;

	case EQ:
	case NE:
	case LT:
	case LE:
	case GT:
	case GE:
		if( mt12 & MDBI ) return( TYMATCH+CVTO );
		else if( mt12 & MPTR ) return( PTMATCH+PUN+CVTO );
		else if( mt12 & MPTI ) return( PTMATCH+PUN );
		else break;

	case QUEST:
		return( TYPR+OTHER );
	case COMOP:
		return( TYPR );

	case STREF:
		return( NCVTR+OTHER );

	case FORCE:
		return( TYPL );

	case COLON:
		if( mt12 & MDBI ) return( TYMATCH );
		else if( mt12 & MPTR ) return( TYPL+PTMATCH+PUN );
		else if( (mt1&MINT) && (mt2&MPTR) ) return( TYPR+PUN );
		else if( (mt1&MPTR) && (mt2&MINT) ) return( TYPL+PUN );
		else if( mt12 & MSTR ) return( NCVT+TYPL+OTHER );
		break;

	case ASSIGN:
	case RETURN:
		if( mt12 & MSTR ) return( LVAL+NCVT+TYPL+OTHER );
		/* FALLTHROUGH */
	case CAST:
		if( mt12 & MDBI ) return( TYPL+LVAL+TYMATCH );
		else if( mt1 & MPTR) return( LVAL+PTMATCH+PUN );
		else if( mt12 & MPTI ) return( TYPL+LVAL+TYMATCH+PUN );
		break;

	case MINUS:
		if (mt12 & MPTR)
			return(CVTO+PTMATCH+PUN);
		if (mt2 & MPTR)
			break;
		/* FALLTHROUGH */
	case PLUS:
		if (mt12 & MDBI)
			return(TYMATCH);
		else if ((mt1&MPTR) && (mt2&MINT))
			return(TYPL+CVTR);
		else if ((mt1&MINT) && (mt2&MPTR))
			return(TYPR+CVTL);

	}
	uerror("operands of %s have incompatible types", copst(o));
	return(NCVT);
}

int
moditype(TWORD ty)
{
	switch (ty) {

	case STRTY:
	case UNIONTY:
		return( MSTR );

	case BOOL:
	case CHAR:
	case SHORT:
	case UCHAR:
	case USHORT:
	case UNSIGNED:
	case ULONG:
	case ULONGLONG:
	case INT:
	case LONG:
	case LONGLONG:
		return( MINT|MDBI|MPTI );
	case FLOAT:
	case DOUBLE:
	case LDOUBLE:
#ifndef NO_COMPLEX
	case FCOMPLEX:
	case COMPLEX:
	case LCOMPLEX:
	case FIMAG:
	case IMAG:
	case LIMAG:
#endif
		return( MDBI );
	default:
		return( MPTR|MPTI );

	}
}

int tvaloff = MAXREGS+NPERMREG > 100 ? MAXREGS+NPERMREG + 100 : 100;

/*
 * Returns a TEMP node with temp number nr.
 * If nr == 0, return a node with a new number.
 */
NODE *
tempnode(int nr, TWORD type, union dimfun *df, struct attr *ap)
{
	NODE *r;

	if (tvaloff == -NOOFFSET)
		tvaloff++; /* Skip this for array indexing */
	r = block(TEMP, NIL, NIL, type, df, ap);
	regno(r) = nr ? nr : tvaloff;
	tvaloff += szty(type);
	return r;
}

/*
 * Do sizeof on p.
 */
NODE *
doszof(NODE *p)
{
	extern NODE *arrstk[10];
	extern int arrstkp;
	union dimfun *df;
	TWORD ty;
	NODE *rv, *q;
	int astkp;

	if (p->n_op == FLD)
		uerror("can't apply sizeof to bit-field");

	/*
	 * Arrays may be dynamic, may need to make computations.
	 */

	rv = bcon(1);
	df = p->n_df;
	ty = p->n_type;
	astkp = 0;
	while (ISARY(ty)) {
		if (df->ddim == NOOFFSET)
			uerror("sizeof of incomplete type");
		if (df->ddim < 0) {
			if (arrstkp)
				q = arrstk[astkp++];
			else
				q = tempnode(-df->ddim, INT, 0, 0);
		} else
			q = bcon(df->ddim);
		rv = buildtree(MUL, rv, q);
		df++;
		ty = DECREF(ty);
	}
	rv = buildtree(MUL, rv, 
	    xbcon(tsize(ty, p->n_df, p->n_ap)/SZCHAR, NULL, INTPTR));
	tfree(p);
	arrstkp = 0; /* XXX - may this fail? */
	return rv;
}

#ifdef PCC_DEBUG
void
eprint(NODE *p, int down, int *a, int *b)
{
	int ty;

	*a = *b = down+1;
	while( down > 1 ){
		printf( "\t" );
		down -= 2;
		}
	if( down ) printf( "    " );

	ty = coptype( p->n_op );

	printf("%p) %s, ", p, copst(p->n_op));
	if (p->n_op == XARG || p->n_op == XASM)
		printf("id '%s', ", p->n_name);
	if (ty == LTYPE) {
		printf(CONFMT, p->n_lval);
		if (p->n_op == NAME || p->n_op == ICON)
			printf(", %p, ", p->n_sp);
		else
			printf(", %d, ", p->n_rval);
	}
	tprint(p->n_type, p->n_qual);
	printf( ", %p, ", p->n_df);
#ifdef GCC_COMPAT
	dump_attr(p->n_ap);
#endif
}
# endif

/*
 * Emit everything that should be emitted on the left side 
 * of a comma operator, and remove the operator.
 * Do not traverse through QUEST, ANDAND and OROR.
 * Enable this for all targets when stable enough.
 */
static void
comops(NODE *p)
{
	int o;
	NODE *q;

	while (p->n_op == COMOP) {
		/* XXX hack for GCC ({ }) ops */
		if (p->n_left->n_op == GOTO) {
			int v = (int)p->n_left->n_left->n_lval;
			ecomp(p->n_left);
			plabel(v+1);
		} else
			ecomp(p->n_left); /* will recurse if more COMOPs */
		q = p->n_right;
		*p = *q;
		nfree(q);
	}
	o = coptype(p->n_op);
	if (p->n_op == QUEST || p->n_op == ANDAND || p->n_op == OROR)
		o = UTYPE;
	if (o != LTYPE)
		comops(p->n_left);
	if (o == BITYPE)
		comops(p->n_right);
}

/*
 * Walk up through the tree from the leaves,
 * removing constant operators.
 */
static void
logwalk(NODE *p)
{
	int o = coptype(p->n_op);
	NODE *l, *r;

	l = p->n_left;
	r = p->n_right;
	switch (o) {
	case LTYPE:
		return;
	case BITYPE:
		logwalk(r);
		/* FALLTHROUGH */
	case UTYPE:
		logwalk(l);
	}
	if (!clogop(p->n_op))
		return;
	if (p->n_op == NOT && l->n_op == ICON) {
		p->n_lval = l->n_lval == 0;
		nfree(l);
		p->n_op = ICON;
	}
	if (l->n_op == ICON && r->n_op == ICON) {
		if (conval(l, p->n_op, r) == 0) {
			/*
			 * people sometimes tend to do really odd compares,
			 * like "if ("abc" == "def")" etc.
			 * do it runtime instead.
			 */
		} else {
			p->n_lval = l->n_lval;
			p->n_op = ICON;
			nfree(l);
			nfree(r);
		}
	}
}

/*
 * Removes redundant logical operators for branch conditions.
 */
static void
fixbranch(NODE *p, int label)
{

	logwalk(p);

	if (p->n_op == ICON) {
		if (p->n_lval != 0)
			branch(label);
		nfree(p);
	} else {
		if (!clogop(p->n_op)) /* Always conditional */
			p = buildtree(NE, p, bcon(0));
		ecode(buildtree(CBRANCH, p, bcon(label)));
	}
}

/*
 * Write out logical expressions as branches.
 */
static void
andorbr(NODE *p, int true, int false)
{
	NODE *q;
	int o, lab;

	lab = -1;
	switch (o = p->n_op) { 
	case EQ:
	case NE:
		/*
		 * Remove redundant EQ/NE nodes.
		 */
		while (((o = p->n_left->n_op) == EQ || o == NE) && 
		    p->n_right->n_op == ICON) {
			o = p->n_op;
			q = p->n_left;
			if (p->n_right->n_lval == 0) {
				nfree(p->n_right);
				*p = *q;
				nfree(q);
				if (o == EQ)
					p->n_op = negrel[p->n_op - EQ];
#if 0
					p->n_op = NE; /* toggla */
#endif
			} else if (p->n_right->n_lval == 1) {
				nfree(p->n_right);
				*p = *q;
				nfree(q);
				if (o == NE)
					p->n_op = negrel[p->n_op - EQ];
#if 0
					p->n_op = EQ; /* toggla */
#endif
			} else
				break; /* XXX - should always be false */
			
		}
		/* FALLTHROUGH */
	case LE:
	case LT:
	case GE:
	case GT:
calc:		if (true < 0) {
			p->n_op = negrel[p->n_op - EQ];
			true = false;
			false = -1;
		}

		rmcops(p->n_left);
		rmcops(p->n_right);
		fixbranch(p, true);
		if (false >= 0)
			branch(false);
		break;

	case ULE:
	case UGT:
		/* Convert to friendlier ops */
		if (nncon(p->n_right) && p->n_right->n_lval == 0)
			p->n_op = o == ULE ? EQ : NE;
		goto calc;

	case UGE:
	case ULT:
		/* Already true/false by definition */
		if (nncon(p->n_right) && p->n_right->n_lval == 0) {
			if (true < 0) {
				o = o == ULT ? UGE : ULT;
				true = false;
			}
			rmcops(p->n_left);
			ecode(p->n_left);
			rmcops(p->n_right);
			ecode(p->n_right);
			nfree(p);
			if (o == UGE) /* true */
				branch(true);
			break;
		}
		goto calc;

	case ANDAND:
		lab = false<0 ? getlab() : false ;
		andorbr(p->n_left, -1, lab);
		comops(p->n_right);
		andorbr(p->n_right, true, false);
		if (false < 0)
			plabel( lab);
		nfree(p);
		break;

	case OROR:
		lab = true<0 ? getlab() : true;
		andorbr(p->n_left, lab, -1);
		comops(p->n_right);
		andorbr(p->n_right, true, false);
		if (true < 0)
			plabel( lab);
		nfree(p);
		break;

	case NOT:
		andorbr(p->n_left, false, true);
		nfree(p);
		break;

	default:
		rmcops(p);
		if (true >= 0)
			fixbranch(p, true);
		if (false >= 0) {
			if (true >= 0)
				branch(false);
			else
				fixbranch(buildtree(EQ, p, bcon(0)), false);
		}
	}
}

/*
 * Create a node for either TEMP or on-stack storage.
 */
NODE *
cstknode(TWORD t, union dimfun *df, struct attr *ap)
{
	struct symtab *sp;

	/* create a symtab entry suitable for this type */
	sp = getsymtab("0hej", STEMP);
	sp->stype = t;
	sp->sdf = df;
	sp->sap = ap;
	sp->sclass = AUTO;
	sp->soffset = NOOFFSET;
	oalloc(sp, &autooff);
	return nametree(sp);

}

/*
 * Massage the output trees to remove C-specific nodes:
 *	COMOPs are split into separate statements.
 *	QUEST/COLON are rewritten to branches.
 *	ANDAND/OROR/NOT are rewritten to branches for lazy-evaluation.
 *	CBRANCH conditions are rewritten for lazy-evaluation.
 */
static void
rmcops(NODE *p)
{
	TWORD type;
	NODE *q, *r, *tval;
	int o, ty, lbl, lbl2;

	tval = NIL;
	o = p->n_op;
	ty = coptype(o);
	if (BTYPE(p->n_type) == ENUMTY) { /* fixup enum */
		struct symtab *sp = strmemb(p->n_ap);
		MODTYPE(p->n_type, sp->stype);
		/*
		 * XXX may fail if these are true:
		 * - variable-sized enums
		 * - non-byte-addressed targets.
		 */
		if (BTYPE(p->n_type) == ENUMTY && ISPTR(p->n_type))
			MODTYPE(p->n_type, INT); /* INT ok? */
	}
	switch (o) {
	case QUEST:

		/*
		 * Create a branch node from ?:
		 * || and && must be taken special care of.
		 */
		type = p->n_type;
		andorbr(p->n_left, -1, lbl = getlab());

		/* Make ASSIGN node */
		/* Only if type is not void */
		q = p->n_right->n_left;
		comops(q);
		if (type != VOID) {
			tval = cstknode(q->n_type, q->n_df, q->n_ap);
			q = buildtree(ASSIGN, ccopy(tval), q);
		}
		rmcops(q);
		ecode(q); /* Done with assign */
		branch(lbl2 = getlab());
		plabel( lbl);

		q = p->n_right->n_right;
		comops(q);
		if (type != VOID) {
			q = buildtree(ASSIGN, ccopy(tval), q);
		}
		rmcops(q);
		ecode(q); /* Done with assign */

		plabel( lbl2);

		nfree(p->n_right);
		if (p->n_type != VOID) {
			*p = *tval;
			nfree(tval);
		} else {
			p->n_op = ICON;
			p->n_lval = 0;
			p->n_sp = NULL;
		}
		break;

	case ULE:
	case ULT:
	case UGE:
	case UGT:
	case EQ:
	case NE:
	case LE:
	case LT:
	case GE:
	case GT:
	case ANDAND:
	case OROR:
	case NOT:
#ifdef SPECIAL_CCODES
#error fix for private CCODES handling
#else
		r = talloc();
		*r = *p;
		andorbr(r, -1, lbl = getlab());

		tval = cstknode(p->n_type, p->n_df, p->n_ap);

		ecode(buildtree(ASSIGN, ccopy(tval), bcon(1)));
		branch(lbl2 = getlab());
		plabel( lbl);
		ecode(buildtree(ASSIGN, ccopy(tval), bcon(0)));
		plabel( lbl2);

		*p = *tval;
		nfree(tval);

#endif
		break;
	case CBRANCH:
		andorbr(p->n_left, p->n_right->n_lval, -1);
		nfree(p->n_right);
		p->n_op = ICON; p->n_type = VOID;
		break;
	case COMOP:
		cerror("COMOP error");

	default:
		if (ty == LTYPE)
			return;
		rmcops(p->n_left);
		if (ty == BITYPE)
			rmcops(p->n_right);
       }
}

/*
 * Return 1 if an assignment is found.
 */
static int
has_se(NODE *p)
{
	if (cdope(p->n_op) & ASGFLG)
		return 1;
	if (coptype(p->n_op) == LTYPE)
		return 0;
	if (has_se(p->n_left))
		return 1;
	if (coptype(p->n_op) == BITYPE)
		return has_se(p->n_right);
	return 0;
}

#ifndef FIELDOPS

/* avoid promotion to int */
#define	TYPMOD(o, p, n, t)	clocal(block(o, p, n, t, 0, 0))
#define	TYPLS(p, n, t)	TYPMOD(LS, p, n, t)
#define	TYPRS(p, n, t)	TYPMOD(RS, p, n, t)
#define	TYPOR(p, q, t)	TYPMOD(OR, p, q, t)
#define	TYPAND(p, q, t)	TYPMOD(AND, p, q, t)

/*
 * Read an unaligned bitfield from position pointed to by p starting at
 * off and size fsz and return a tree of type t with resulting data.
 * ct is the type we must use to read data.
 */
static NODE *
rdualfld(NODE *p, TWORD t, TWORD ct, int off, int fsz)
{
	int t2f, inbits, tsz, ctsz;
	NODE *q, *r;

	ct = ENUNSIGN(ct);
	ctsz = (int)tsize(ct, 0, 0);

	/* traverse until first data byte */
	for (t2f = 0; off >= ctsz; t2f++, off -= ctsz)
		;
#ifdef UNALIGNED_ACCESS
	/* try to squeeze it into an int */
	if (off + fsz > ctsz && off + fsz <= SZINT) {
		ct = UNSIGNED;
		ctsz = SZINT;
	}
#endif
	p = makety(p, PTR|ct, 0, 0, 0);
	if (off + fsz <= ctsz) {
		/* only one operation needed */
		q = buildtree(UMUL, buildtree(PLUS, p, bcon(t2f)), 0);
		if (!ISUNSIGNED(t)) {
			ct = DEUNSIGN(ct);
			q = makety(q, ct, 0, 0, 0);
		}
		q = TYPLS(q, bcon(ctsz-fsz-off), ct);
		q = TYPRS(q, bcon(ctsz-fsz), ct);
		q = makety(q, t, 0, 0, 0);
	} else {
		q = buildtree(UMUL, buildtree(PLUS, ccopy(p), bcon(t2f)), 0);
		q = makety(TYPRS(q, bcon(off), ct), t, 0, 0, 0);
		inbits = ctsz - off;
		t2f++;

		while (fsz > inbits) {
			r = buildtree(UMUL,
			    buildtree(PLUS, ccopy(p), bcon(t2f)), 0);
			r = makety(r, t, 0, 0, 0);
			r = TYPLS(r, bcon(inbits), t);
			q = TYPOR(q, r, t);
			inbits += ctsz;
			t2f++;
		}
		/* sign/zero extend XXX - RS must sign extend */
		tsz = (int)tsize(t, 0, 0);
		if (!ISUNSIGNED(t)) {
			t = DEUNSIGN(t);
			q = makety(q, t, 0, 0, 0);
		}
		q = TYPLS(q, bcon(tsz-fsz), t);
		q = TYPRS(q, bcon(tsz-fsz), t);
		tfree(p);
	}

	return q;
}

/*
 * Write val to a (unaligned) bitfield with length fsz positioned off bits  
 * from d. Bitfield type is t, and type to use when writing is ct.
 * neither f nor d should have any side effects if copied.
 * Multiples of ct are supposed to be written without problems.
 * Both val and d are free'd after use.
 */
static NODE *
wrualfld(NODE *val, NODE *d, TWORD t, TWORD ct, int off, int fsz)
{ 
	NODE *p, *q, *r, *rn, *s;
	int ctsz, t2f, inbits;
 
	ctsz = (int)tsize(ct, 0, 0);
  
	ct = ENUNSIGN(ct);
	d = makety(d, PTR|ct, 0, 0, 0);

	for (t2f = 0; off >= ctsz; t2f++, off -= ctsz)
		;
 
	if (off + fsz <= ctsz) {
		r = tempnode(0, ct, 0, 0);

		/* only one operation needed */
		d = buildtree(UMUL, buildtree(PLUS, d, bcon(t2f)), 0);	
		p = ccopy(d); 
		p = TYPAND(p, xbcon(~(SZMASK(fsz) << off), 0, ct), ct);

		val = makety(val, ct, 0, 0, 0);
		q = TYPAND(val, xbcon(SZMASK(fsz), 0, ct), ct);
		q = buildtree(ASSIGN, ccopy(r), q);

		q = TYPLS(q, bcon(off), ct);   
		p = TYPOR(p, q, ct);
		p = makety(p, t, 0, 0, 0);     
		rn = buildtree(ASSIGN, d, p);
		rn = buildtree(COMOP, rn, makety(r, t, 0, 0, 0));
	} else {
		s = makety(ccopy(val), t, 0, 0, 0);
		s = TYPAND(s, xbcon(SZMASK(fsz), 0, t), t);

		r = buildtree(UMUL, buildtree(PLUS, ccopy(d), bcon(t2f)), 0);
		p = ccopy(r);
		p = TYPAND(p, xbcon(SZMASK(off), 0, ct), ct);
		q = ccopy(val); 
		q = TYPLS(q, bcon(off), t);  
		q = makety(q, ct, 0, 0, 0);
		p = TYPOR(p, q, ct);
		rn = buildtree(ASSIGN, r, p);
		inbits = ctsz - off;
		t2f++;

		while (fsz > inbits+ctsz) {
			r = buildtree(UMUL,
			    buildtree(PLUS, ccopy(d), bcon(t2f)), 0);
			q = ccopy(val);
			q = TYPRS(q, bcon(inbits), t);
			q = makety(q, ct, 0, 0, 0);
			rn = buildtree(COMOP, rn, buildtree(ASSIGN, r, q));
			t2f++;
			inbits += ctsz;
		}

		r = buildtree(UMUL, buildtree(PLUS, d, bcon(t2f)), 0);
		p = ccopy(r);
		p = TYPAND(p, makety(xbcon(~SZMASK(fsz-inbits), 0, ct),
		    ct, 0, 0, 0), ct);
		q = TYPRS(val, bcon(inbits), t);
		q = TYPAND(q, xbcon(SZMASK(fsz-inbits), 0, t), t);
		q = makety(q, ct, 0, 0, 0);
		p = TYPOR(p, q, ct);
		rn = buildtree(COMOP, rn, buildtree(ASSIGN, r, p));
		rn = buildtree(COMOP, rn, s);
	}
	return rn;
}

/*
 * Rewrite bitfield operations to shifts.
 */
static NODE *
rmfldops(NODE *p)
{
	TWORD t, ct;
	NODE *q, *r, *t1, *t2, *bt, *t3, *t4;
	int fsz, foff;

	if (p->n_op == FLD) {
		/* Rewrite a field read operation */
		fsz = UPKFSZ(p->n_rval);
		foff = UPKFOFF(p->n_rval);
		q = buildtree(ADDROF, p->n_left, NIL);

		ct = t = p->n_type;
#ifdef GCC_COMPAT
		if (attr_find(p->n_ap, GCC_ATYP_PACKED) &&
		    coptype(q->n_op) != LTYPE) {
			t1 = tempnode(0, q->n_type, 0, 0);
			bt = buildtree(ASSIGN, ccopy(t1), q);
			q = t1;
#ifndef UNALIGNED_ACCESS
			ct = UCHAR;
#endif
		} else
#endif
			bt = bcon(0);
		q = rdualfld(q, t, ct, foff, fsz);
		p->n_left = bt;
		p->n_right = q;
		p->n_op = COMOP;
	} else if (((cdope(p->n_op)&ASGOPFLG) || p->n_op == ASSIGN ||
	    p->n_op == INCR || p->n_op == DECR) && p->n_left->n_op == FLD) {
		/*
		 * Rewrite a field write operation
		 * More difficult than a read op since we must care
		 * about side effects.
		 */
		q = p->n_left;
		fsz = UPKFSZ(q->n_rval);
		foff = UPKFOFF(q->n_rval);
		t = q->n_left->n_type;
#if TARGET_ENDIAN == TARGET_BE
		foff = (int)tsize(t, 0, 0) - fsz - foff;
#endif
		bt = NULL;
		if (p->n_right->n_op != ICON && p->n_right->n_op != NAME) {
			t2 = tempnode(0, p->n_right->n_type, 0, 0);
			bt = buildtree(ASSIGN, ccopy(t2), p->n_right);
		} else
			t2 = p->n_right;

		ct = t;
#ifdef GCC_COMPAT
#ifndef UNALIGNED_ACCESS
		if (attr_find(q->n_ap, GCC_ATYP_PACKED))
			ct = UCHAR;
#endif
#endif
		/* t2 is what we have to write (RHS of ASSIGN) */
		/* bt is (eventually) something that must be written */


		if (q->n_left->n_op == UMUL) {
			/* LHS of assignment may have side effects */
			q = q->n_left;
			t1 = tempnode(0, q->n_left->n_type, 0, 0);
			r = buildtree(ASSIGN, ccopy(t1), q->n_left);
			
			bt = bt ? block(COMOP, bt, r, INT, 0, 0) : r;
			q->n_left = t1;
		}
		t1 = buildtree(ADDROF, p->n_left->n_left, 0);

		/* t1 is lval where to write (and read) */

		if (p->n_op == ASSIGN) {
			q = wrualfld(t2, t1, t, ct, foff, fsz);
			if (bt)
				q = block(COMOP, bt, q, t, 0, 0);
			nfree(p->n_left);
			p->n_left = bcon(0);
			p->n_right = q;
			p->n_op = COMOP;
		} else if ((cdope(p->n_op)&ASGOPFLG)) {
			/* And here is the asgop-specific code */
			t3 = tempnode(0, t, 0, 0);
			q = rdualfld(ccopy(t1), t, ct, foff, fsz);
			q = buildtree(UNASG p->n_op, q, t2);
			q = buildtree(ASSIGN, ccopy(t3), q);
			r = wrualfld(ccopy(t3), t1, t, ct, foff, fsz);
			q = buildtree(COMOP, q, r);
			q = buildtree(COMOP, q, t3);

			nfree(p->n_left);
			p->n_left = bt ? bt : bcon(0);
			p->n_right = q;
			p->n_op = COMOP;
		} else {
			t3 = tempnode(0, t, 0, 0);
			t4 = tempnode(0, t, 0, 0);

			q = rdualfld(ccopy(t1), t, ct, foff, fsz);
			q = buildtree(ASSIGN, ccopy(t3), q);
			r = buildtree(p->n_op==INCR?PLUS:MINUS, ccopy(t3), t2);
			r = buildtree(ASSIGN, ccopy(t4), r);
			q = buildtree(COMOP, q, r);
			r = wrualfld(t4, t1, t, ct, foff, fsz);
			q = buildtree(COMOP, q, r);
		
			if (bt)
				q = block(COMOP, bt, q, t, 0, 0);
			nfree(p->n_left);
			p->n_left = q;
			p->n_right = t3;
			p->n_op = COMOP;
		}
	}
	if (coptype(p->n_op) != LTYPE)
		p->n_left = rmfldops(p->n_left);
	if (coptype(p->n_op) == BITYPE)
		p->n_right = rmfldops(p->n_right);
	return p;
}
#endif

void
ecomp(NODE *p)
{

#ifdef PCC_DEBUG
	if (edebug)
		fwalk(p, eprint, 0);
#endif
	if (!reached) {
		warner(Wunreachable_code, NULL);
		reached = 1;
	}
	p = optim(p);
#ifndef FIELDOPS
	p = rmfldops(p);
#endif
	comops(p);
	rmcops(p);
	if (p->n_op == ICON && p->n_type == VOID)
		tfree(p);
	else
		ecode(p);
}


#if defined(MULTIPASS)
void	
p2tree(NODE *p)
{
	struct symtab *q;
	int ty;

	myp2tree(p);  /* local action can be taken here */

	ty = coptype(p->n_op);

	printf("%d\t", p->n_op);

	if (ty == LTYPE) {
		printf(CONFMT, p->n_lval);
		printf("\t");
	}
	if (ty != BITYPE) {
		if (p->n_op == NAME || p->n_op == ICON)
			printf("0\t");
		else
			printf("%d\t", p->n_rval);
		}

	printf("%o\t", p->n_type);

	/* handle special cases */

	switch (p->n_op) {

	case NAME:
	case ICON:
		/* print external name */
		if ((q = p->n_sp) != NULL) {
			if ((q->sclass == STATIC && q->slevel > 0)) {
				printf(LABFMT, q->soffset);
			} else
				printf("%s\n",
				    q->soname ? q->soname : exname(q->sname));
		} else
			printf("\n");
		break;

	case STARG:
	case STASG:
	case STCALL:
	case USTCALL:
		/* print out size */
		/* use lhs size, in order to avoid hassles
		 * with the structure `.' operator
		 */

		/* note: p->left not a field... */
		printf(CONFMT, (CONSZ)tsize(STRTY, p->n_left->n_df,
		    p->n_left->n_ap));
		printf("\t%d\t\n", talign(STRTY, p->n_left->n_ap));
		break;

	case XARG:
	case XASM:
		break;

	default:
		printf(	 "\n" );
	}

	if (ty != LTYPE)
		p2tree(p->n_left);
	if (ty == BITYPE)
		p2tree(p->n_right);
}
#else
static char *
sptostr(struct symtab *sp)
{
	char *cp = inlalloc(32);
	int n = sp->soffset;
	if (n < 0)
		n = -n;
	snprintf(cp, 32, LABFMT, n);
	return cp;
}

void
p2tree(NODE *p)
{
	struct symtab *q;
	int ty;

	myp2tree(p);  /* local action can be taken here */

	/* Fix left imaginary types */
	if (ISITY(BTYPE(p->n_type)))
		MODTYPE(p->n_type, p->n_type - (FIMAG-FLOAT));

	ty = coptype(p->n_op);

	switch( p->n_op ){

	case NAME:
	case ICON:
		if ((q = p->n_sp) != NULL) {
			if ((q->sclass == STATIC && q->slevel > 0)
#ifdef GCC_COMPAT
			    || q->sflags == SLBLNAME
#endif
			    ) {
				p->n_name = sptostr(q);
			} else {
				if ((p->n_name = q->soname) == NULL)
					p->n_name = addname(exname(q->sname));
			}
		} else
			p->n_name = "";
		break;

	case STASG:
		/* STASG used for stack array init */
		if (ISARY(p->n_type)) {
			int size1 = (int)tsize(p->n_type, p->n_left->n_df,
			    p->n_left->n_ap)/SZCHAR;
			p->n_stsize = (int)tsize(p->n_type, p->n_right->n_df,
			    p->n_right->n_ap)/SZCHAR;
			if (size1 < p->n_stsize)
				p->n_stsize = size1;
			p->n_stalign = talign(p->n_type,
			    p->n_left->n_ap)/SZCHAR;
			break;
		}
		/* FALLTHROUGH */
	case STARG:
	case STCALL:
	case USTCALL:
		/* set up size parameters */
		p->n_stsize = (int)((tsize(STRTY, p->n_left->n_df,
		    p->n_left->n_ap)+SZCHAR-1)/SZCHAR);
		p->n_stalign = talign(STRTY,p->n_left->n_ap)/SZCHAR;
		if (p->n_stalign == 0)
			p->n_stalign = 1; /* At least char for packed structs */
		break;

	case XARG:
	case XASM:
		break;

	default:
		p->n_name = "";
		}

	if( ty != LTYPE ) p2tree( p->n_left );
	if( ty == BITYPE ) p2tree( p->n_right );
	}

#endif

/*
 * Change void data types into char.
 */
static void
delvoid(NODE *p, void *arg)
{
	/* Convert "PTR undef" (void *) to "PTR uchar" */
	if (BTYPE(p->n_type) == VOID)
		p->n_type = (p->n_type & ~BTMASK) | UCHAR;
	if (BTYPE(p->n_type) == BOOL) {
		if (p->n_op == SCONV && p->n_type == BOOL) {
			/* create a jump and a set */
			NODE *r;
			int l, l2;

			r = tempnode(0, BOOL_TYPE, NULL, 0);
			cbranch(buildtree(EQ, p->n_left, bcon(0)),
			    bcon(l = getlab()));
			*p = *r;
			ecode(buildtree(ASSIGN, tcopy(r), bcon(1)));
			branch(l2 = getlab());
			plabel(l);
			ecode(buildtree(ASSIGN, r, bcon(0)));
			plabel(l2);
		} else
			p->n_type = (p->n_type & ~BTMASK) | BOOL_TYPE;
	}
		
}

/*
 * Change calls inside calls to separate statement.
 */
static NODE *
deldcall(NODE *p, int split)
{
	NODE *q, *r;
	int o = p->n_op;

	if (cdope(o) & CALLFLG) {
		if (split) {
			q = cstknode(p->n_type, p->n_df, p->n_ap);
			r = ccopy(q);
			q = block(ASSIGN, q, p, p->n_type, p->n_df, p->n_ap);
			ecode(q);
			return r;
		}
		split++;
	}
	if (coptype(o) == BITYPE)
		p->n_right = deldcall(p->n_right, split);
	if (coptype(o) != LTYPE)
		p->n_left = deldcall(p->n_left, split);
	return p;
}

#ifndef WORD_ADDRESSED

static NODE *
pprop(NODE *p, TWORD t, struct attr *ap)
{
	int o = p->n_op;
	TWORD t2;

#ifdef PCC_DEBUG
	if (p->n_op == TEMP && p->n_type != t &&
	    !(ISPTR(p->n_type) && ISPTR(t))) {
		cerror("TEMP type change: %x -> %x", p->n_type, t);
	}
#endif

	p->n_type = t;
	p->n_ap = ap;
	switch (o) {
	case UMUL:
		t = INCREF(t);
		break;
	case ADDROF:
		t2 = p->n_left->n_type;
		if (p->n_left->n_op == TEMP) {
			/* Will be converted to memory in pass2 */
			/* Do not convert if:
			 * - t2 not ptr and decref(t) != t2
			 * - decref(t) not ptr and decref(t) != t2
			 * XXX add PCONV again? Will be removed upwards.
			 */
			if ((!ISPTR(t2) && DECREF(t) != t2) ||
			    (ISPTR(t2) && !ISPTR(DECREF(t))))
				; /* Cannot convert this */
			else
				p->n_left->n_type = DECREF(t);
			return p;
		}
		if (ISPTR(t2) && !ISPTR(DECREF(t)))
			break; /* not quite correct */
		t = DECREF(t);
		break;
	case PCONV:
		return p;

	case PLUS:
		if (!ISPTR(p->n_left->n_type)) {
			if (!ISPTR(p->n_right->n_type))
				cerror("%p: no * in PLUS", p);
			p->n_right = pprop(p->n_right, t, ap);
		} else
			p->n_left = pprop(p->n_left, t, ap);
		return p;

	case MINUS:
		if (ISPTR(p->n_left->n_type)) {
			if (ISPTR(p->n_right->n_type))
				break; /* change both */
			p->n_left = pprop(p->n_left, t, ap);
		} else 
			p->n_right = pprop(p->n_right, t, ap);
		return p;

	case CALL:
	case UCALL:
	case STCALL: /* may end up here if struct passed in regs */
	case USTCALL:
		return p;

	case STASG: /* if struct is cast to pointer */
		return p;

	case ASSIGN:
		break;

	case COMOP:
		p->n_right = pprop(p->n_right, t, ap);
		return p;

	default:
		if (coptype(o) == LTYPE)
			break;

#ifdef PCC_DEBUG
		fwalk(p, eprint, 0);
#endif
		cerror("pprop op error %d\n", o);
	}
	if (coptype(o) == BITYPE)
		p->n_right = pprop(p->n_right, t, ap);
	if (coptype(o) != LTYPE)
		p->n_left = pprop(p->n_left, t, ap);
	return p;
}

/*
 * Search for PCONV's that can be removed while still keeping
 * the type correctness.
 */
NODE *
rmpconv(NODE *p)
{
	struct symtab *sp;
	int o = p->n_op;
	int ot = coptype(o);
	NODE *q, *l;

	if (ot != LTYPE)
		p->n_left = rmpconv(p->n_left);
	if (ot == BITYPE)
		p->n_right = rmpconv(p->n_right);
	if (o != PCONV)
		return p;
	l = p->n_left;
	if (nncon(l) || (cdope(l->n_op) & CALLFLG))
		; /* Let any nonamed constant be cast to pointer directly */
	else if (l->n_type >= INTPTR && l->n_op == ICON) {
		/* named constants only if >= pointer size */
		/* create INTPTR type */
		sp = l->n_sp;
		l->n_sp = NULL;
		concast(l, INTPTR);
		l->n_sp = sp;
	} else if (!ISPTR(l->n_type))
		return p;
	q = pprop(p->n_left, p->n_type, p->n_ap);
	nfree(p);
	return q;
}
#endif

NODE *
optloop(NODE *p)
{
	extern int usednodes;
	int n;

	do {
		n = usednodes;
		p = rmpconv(p);
		p = optim(p);
	} while (n != usednodes);

	return p;
}

void
ecode(NODE *p)	
{
	/* walk the tree and write out the nodes.. */

	if (nerrors)	
		return;

#ifdef GCC_COMPAT
	{
		NODE *q = p;

		if (q->n_op == UMUL)
			q = p->n_left;
		if (cdope(q->n_op)&CALLFLG &&
		    attr_find(q->n_ap, GCC_ATYP_WARN_UNUSED_RESULT))
			werror("return value ignored");
	}
#endif
#ifndef WORD_ADDRESSED
	p = rmpconv(p);
#endif
	p = optim(p);
	p = deldcall(p, 0);
	walkf(p, delvoid, 0);
#ifdef PCC_DEBUG
	if (xdebug) {
		printf("Fulltree:\n"); 
		fwalk(p, eprint, 0); 
	}
#endif
	p2tree(p);
#if !defined(MULTIPASS)
	send_passt(IP_NODE, p);
#endif
}

/*
 * Send something further on to the next pass.
 */
void
send_passt(int type, ...)
{
	struct interpass *ip;
	struct interpass_prolog *ipp;
	extern int crslab;
	va_list ap;
	int sz;

	va_start(ap, type);
	if (cftnsp == NULL && type != IP_ASM) {
#ifdef notyet
		cerror("no function");
#endif
		if (type == IP_NODE)
			tfree(va_arg(ap, NODE *));
		return;
	}
	if (type == IP_PROLOG || type == IP_EPILOG)
		sz = sizeof(struct interpass_prolog);
	else
		sz = sizeof(struct interpass);

	ip = inlalloc(sz);
	ip->type = type;
	ip->lineno = lineno;
	switch (type) {
	case IP_NODE:
		ip->ip_node = va_arg(ap, NODE *);
		if (ip->ip_node->n_op == LABEL) {
			NODE *p = ip->ip_node;
			ip->ip_lbl = p->n_left->n_lval;
			ip->type = IP_DEFLAB;
			nfree(nfree(p));
		}
		break;
	case IP_EPILOG:
		if (!isinlining) {
			locctr(PROG, cftnsp);
			defloc(cftnsp);
		}
		/* FALLTHROUGH */
	case IP_PROLOG:
		inftn = type == IP_PROLOG ? 1 : 0;
		ipp = (struct interpass_prolog *)ip;
		memset(ipp->ipp_regs, (type == IP_PROLOG)? -1 : 0,
		    sizeof(ipp->ipp_regs));
		ipp->ipp_autos = va_arg(ap, int);
		ipp->ipp_name = va_arg(ap, char *);
		ipp->ipp_type = va_arg(ap, TWORD);
		ipp->ipp_vis = va_arg(ap, int);
		ip->ip_lbl = va_arg(ap, int);
		ipp->ip_tmpnum = va_arg(ap, int);
		ipp->ip_lblnum = crslab;
		ipp->ip_labels = va_arg(ap, int *);;
		if (type == IP_PROLOG)
			ipp->ip_lblnum--;
		break;
	case IP_DEFLAB:
		ip->ip_lbl = va_arg(ap, int);
		break;
	case IP_ASM:
		if (blevel == 0) { /* outside function */
			printf("%s", va_arg(ap, char *));
			va_end(ap);
			locctr(NOSEG, NULL);
			return;
		}
		ip->ip_asm = va_arg(ap, char *);
		break;
	default:
		cerror("bad send_passt type %d", type);
	}
	va_end(ap);
	pass1_lastchance(ip); /* target-specific info */
	if (isinlining)
		inline_addarg(ip);
	else
		pass2_compile(ip);
}

char *
copst(int op)
{
	if (op <= MAXOP)
		return opst[op];
#define	SNAM(x,y) case x: return #y;
	switch (op) {
	SNAM(QUALIFIER,QUALIFIER)
	SNAM(CLASS,CLASS)
	SNAM(RB,])
	SNAM(DOT,.)
	SNAM(ELLIPSIS,...)
	SNAM(LB,[)
	SNAM(TYPE,TYPE)
	SNAM(COMOP,COMOP)
	SNAM(QUEST,?)
	SNAM(BIQUEST,?:)
	SNAM(COLON,:)
	SNAM(ANDAND,&&)
	SNAM(OROR,||)
	SNAM(NOT,!)
	SNAM(CAST,CAST)
	SNAM(PLUSEQ,+=)
	SNAM(MINUSEQ,-=)
	SNAM(MULEQ,*=)
	SNAM(DIVEQ,/=)
	SNAM(MODEQ,%=)
	SNAM(ANDEQ,&=)
	SNAM(OREQ,|=)
	SNAM(EREQ,^=)
	SNAM(LSEQ,<<=)
	SNAM(RSEQ,>>=)
	SNAM(INCR,++)
	SNAM(DECR,--)
	SNAM(STRING,STRING)
	SNAM(SZOF,SIZEOF)
	SNAM(ATTRIB,ATTRIBUTE)
	SNAM(TYMERGE,TYMERGE)
	SNAM(LABEL,LABEL)
	SNAM(UPLUS,U+)
#ifdef GCC_COMPAT
	SNAM(XREAL,__real__)
	SNAM(XIMAG,__imag__)
#endif
	default:
		cerror("bad copst %d", op);
	}
	return 0; /* XXX gcc */
}

int
cdope(int op)
{
	if (op <= MAXOP)
		return dope[op];
	switch (op) {
	case CLOP:
	case STRING:
	case QUALIFIER:
	case CLASS:
	case RB:
	case ELLIPSIS:
	case TYPE:
		return LTYPE;
	case DOT:
	case SZOF:
	case COMOP:
	case QUEST:
	case BIQUEST:
	case COLON:
	case LB:
	case TYMERGE:
		return BITYPE;
	case XIMAG:
	case XREAL:
	case ATTRIB:
	case LABEL:
	case UPLUS:
		return UTYPE;
	case ANDAND:
	case OROR:
		return BITYPE|LOGFLG;
	case NOT:
		return UTYPE|LOGFLG;
	case CAST:
		return BITYPE|ASGFLG|ASGOPFLG;
	case PLUSEQ:
		return BITYPE|ASGFLG|ASGOPFLG|FLOFLG|SIMPFLG|COMMFLG;
	case MINUSEQ:
		return BITYPE|FLOFLG|SIMPFLG|ASGFLG|ASGOPFLG;
	case MULEQ:
		return BITYPE|FLOFLG|MULFLG|ASGFLG|ASGOPFLG;
	case OREQ:
	case EREQ:
	case ANDEQ:
		return BITYPE|SIMPFLG|COMMFLG|ASGFLG|ASGOPFLG;
	case DIVEQ:
		return BITYPE|FLOFLG|MULFLG|DIVFLG|ASGFLG|ASGOPFLG;
	case MODEQ:
		return BITYPE|DIVFLG|ASGFLG|ASGOPFLG;
	case LSEQ:
	case RSEQ:
		return BITYPE|SHFFLG|ASGFLG|ASGOPFLG;
	case INCR:
	case DECR:
		return BITYPE|ASGFLG;
	}
	cerror("cdope missing op %d", op);
	return 0; /* XXX gcc */
}

/* 
 * make a fresh copy of p
 */
NODE *
ccopy(NODE *p) 
{  
	NODE *q;

	q = talloc();
	*q = *p;

	switch (coptype(q->n_op)) {
	case BITYPE:
		q->n_right = ccopy(p->n_right);
		/* FALLTHROUGH */
	case UTYPE: 
		q->n_left = ccopy(p->n_left);
	}

	return(q);
}

NODE *
nlabel(int label)
{
	return block(LABEL, bcon(label), NIL, 0, 0, 0);
}

/*
 * set PROG-seg label.
 */
void
plabel(int label)
{
	reached = 1; /* Will this always be correct? */
	send_passt(IP_NODE, nlabel(label));
}

/*
 * Perform integer promotion on node n.
 */
NODE *
intprom(NODE *n)
{
	if ((n->n_type >= CHAR && n->n_type < INT) || n->n_type == BOOL) {
		if ((n->n_type == UCHAR && MAX_UCHAR > MAX_INT) ||
		    (n->n_type == USHORT && MAX_USHORT > MAX_INT))
			return makety(n, UNSIGNED, 0, 0, 0);
		return makety(n, INT, 0, 0, 0);
	}
	return n;
}

/*
 * Return CON/VOL/0, whichever are active for the current type.
 */
int
cqual(TWORD t, TWORD q)
{
	while (ISARY(t))
		t = DECREF(t), q = DECQAL(q);
	if (t <= BTMASK)
		q <<= TSHIFT;
	return q & (CON|VOL);
}

int crslab = 10;
/*
 * Return a number for internal labels.
 */
int
getlab(void)
{
	return crslab++;
}
