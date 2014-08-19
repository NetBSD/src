/*	Id: optim.c,v 1.57 2014/04/08 19:54:56 ragge Exp 	*/	
/*	$NetBSD: optim.c,v 1.1.1.5.2.1 2014/08/19 23:52:09 tls Exp $	*/
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

# define SWAP(p,q) {sp=p; p=q; q=sp;}
# define RCON(p) (p->n_right->n_op==ICON)
# define RO(p) p->n_right->n_op
# define RV(p) p->n_right->n_lval
# define LCON(p) (p->n_left->n_op==ICON)
# define LO(p) p->n_left->n_op
# define LV(p) p->n_left->n_lval

/* remove left node */
static NODE *
zapleft(NODE *p)
{
	NODE *q;

	q = p->n_left;
	nfree(p->n_right);
	nfree(p);
	return q;
}

/*
 * fortran function arguments
 */
static NODE *
fortarg(NODE *p)
{
	if( p->n_op == CM ){
		p->n_left = fortarg( p->n_left );
		p->n_right = fortarg( p->n_right );
		return(p);
	}

	while( ISPTR(p->n_type) ){
		p = buildtree( UMUL, p, NIL );
	}
	return( optim(p) );
}

	/* mapping relationals when the sides are reversed */
short revrel[] ={ EQ, NE, GE, GT, LE, LT, UGE, UGT, ULE, ULT };

/*
 * local optimizations, most of which are probably
 * machine independent
 */
NODE *
optim(NODE *p)
{
	int o, ty;
	NODE *sp, *q;
	OFFSZ sz;
	int i;

	if (odebug) return(p);

	ty = coptype(p->n_op);
	if( ty == LTYPE ) return(p);

	if( ty == BITYPE ) p->n_right = optim(p->n_right);
	p->n_left = optim(p->n_left);

	/* collect constants */
again:	o = p->n_op;
	switch(o){

	case SCONV:
		if (concast(p->n_left, p->n_type)) {
			q = p->n_left;
			nfree(p);
			p = q;
			break;
		}
		/* FALLTHROUGH */
	case PCONV:
		if (p->n_type != VOID)
			p = clocal(p);
		break;

	case FORTCALL:
		p->n_right = fortarg( p->n_right );
		break;

	case ADDROF:
		if (LO(p) == TEMP)
			break;
		if( LO(p) != NAME ) cerror( "& error" );

		if( !andable(p->n_left) && !statinit)
			break;

		LO(p) = ICON;

		setuleft:
		/* paint over the type of the left hand side with the type of the top */
		p->n_left->n_type = p->n_type;
		p->n_left->n_df = p->n_df;
		p->n_left->n_ap = p->n_ap;
		q = p->n_left;
		nfree(p);
		p = q;
		break;

	case NOT:
	case UMINUS:
	case COMPL:
		if (LCON(p) && conval(p->n_left, o, p->n_left))
			p = nfree(p);
		break;

	case UMUL:
		/* Do not discard ADDROF TEMP's */
		if (LO(p) == ADDROF && LO(p->n_left) != TEMP) {
			q = p->n_left->n_left;
			nfree(p->n_left);
			nfree(p);
			p = q;
			break;
		}
		if( LO(p) != ICON ) break;
		LO(p) = NAME;
		goto setuleft;

	case RS:
		if (LCON(p) && RCON(p) && conval(p->n_left, o, p->n_right))
			goto zapright;

		sz = tsize(p->n_type, p->n_df, p->n_ap);

		if (LO(p) == RS && RCON(p->n_left) && RCON(p) &&
		    (RV(p) + RV(p->n_left)) < sz) {
			/* two right-shift  by constants */
			RV(p) += RV(p->n_left);
			p->n_left = zapleft(p->n_left);
		}
#if 0
		  else if (LO(p) == LS && RCON(p->n_left) && RCON(p)) {
			RV(p) -= RV(p->n_left);
			if (RV(p) < 0)
				o = p->n_op = LS, RV(p) = -RV(p);
			p->n_left = zapleft(p->n_left);
		}
#endif
		if (RO(p) == ICON) {
			if (RV(p) < 0) {
				RV(p) = -RV(p);
				p->n_op = LS;
				goto again;
			}
#ifdef notyet /* must check for side effects, --a >> 32; */
			if (RV(p) >= tsize(p->n_type, p->n_df, p->n_sue) &&
			    ISUNSIGNED(p->n_type)) { /* ignore signed shifts */
				/* too many shifts */
				tfree(p->n_left);
				nfree(p->n_right);
				p->n_op = ICON; p->n_lval = 0; p->n_sp = NULL;
			} else
#endif
			/* avoid larger shifts than type size */
			if (RV(p) >= sz)
				werror("shift larger than type");
			if (RV(p) == 0)
				p = zapleft(p);
		}
		break;

	case LS:
		if (LCON(p) && RCON(p) && conval(p->n_left, o, p->n_right))
			goto zapright;

		sz = tsize(p->n_type, p->n_df, p->n_ap);

		if (LO(p) == LS && RCON(p->n_left) && RCON(p)) {
			/* two left-shift  by constants */
			RV(p) += RV(p->n_left);
			p->n_left = zapleft(p->n_left);
		}
#if 0
		  else if (LO(p) == RS && RCON(p->n_left) && RCON(p)) {
			RV(p) -= RV(p->n_left);
			p->n_left = zapleft(p->n_left);
		}
#endif
		if (RO(p) == ICON) {
			if (RV(p) < 0) {
				RV(p) = -RV(p);
				p->n_op = RS;
				goto again;
			}
#ifdef notyet /* must check for side effects */
			if (RV(p) >= tsize(p->n_type, p->n_df, p->n_sue)) {
				/* too many shifts */
				tfree(p->n_left);
				nfree(p->n_right);
				p->n_op = ICON; p->n_lval = 0; p->n_sp = NULL;
			} else
#endif
			/* avoid larger shifts than type size */
			if (RV(p) >= sz)
				werror("shift larger than type");
			if (RV(p) == 0)  
				p = zapleft(p);
		}
		break;

	case QUEST:
		if (!LCON(p))
			break;
		if (LV(p) == 0) {
			q = p->n_right->n_right;
		} else {
			q = p->n_right->n_left;
			p->n_right->n_left = p->n_right->n_right;
		}
		p->n_right->n_op = UMUL; /* for tfree() */
		tfree(p);
		p = q;
		break;

	case MINUS:
		if (LCON(p) && RCON(p) && p->n_left->n_sp == p->n_right->n_sp) {
			/* link-time constants, but both are the same */
			/* solve it now by forgetting the symbols */
			p->n_left->n_sp = p->n_right->n_sp = NULL;
		}
		if( !nncon(p->n_right) ) break;
		RV(p) = -RV(p);
		o = p->n_op = PLUS;
		/* FALLTHROUGH */
	case MUL:
		/*
		 * Check for u=(x-y)+z; where all vars are pointers to
		 * the same struct. This has two advantages:
		 * 1: avoid a mul+div
		 * 2: even if not allowed, people may get surprised if this
		 *    calculation do not give correct result if using
		 *    unaligned structs.
		 */
		if (o == MUL && p->n_type == INTPTR && RCON(p) &&
		    LO(p) == DIV && RCON(p->n_left) &&
		    RV(p) == RV(p->n_left) &&
		    LO(p->n_left) == MINUS) {
			q = p->n_left->n_left;
			if (q->n_left->n_type == PTR+STRTY &&
			    q->n_right->n_type == PTR+STRTY &&
			    strmemb(q->n_left->n_ap) ==
			    strmemb(q->n_right->n_ap)) {
				p = zapleft(p);
				p = zapleft(p);
			}
		}
		/* FALLTHROUGH */
	case PLUS:
	case AND:
	case OR:
	case ER:
		/* commutative ops; for now, just collect constants */
		/* someday, do it right */
		if( nncon(p->n_left) || ( LCON(p) && !RCON(p) ) )
			SWAP( p->n_left, p->n_right );
		/* make ops tower to the left, not the right */
		if( RO(p) == o ){
			SWAP(p->n_left, p->n_right);
#ifdef notdef
		/* Yetch, this breaks type correctness in trees */
		/* Code was probably written before types */
		/* All we can do here is swap and pray */
			NODE *t1, *t2, *t3;
			t1 = p->n_left;
			sp = p->n_right;
			t2 = sp->n_left;
			t3 = sp->n_right;
			/* now, put together again */
			p->n_left = sp;
			sp->n_left = t1;
			sp->n_right = t2;
			sp->n_type = p->n_type;
			p->n_right = t3;
#endif
			}
		if(o == PLUS && LO(p) == MINUS && RCON(p) && RCON(p->n_left) &&
		   conval(p->n_right, MINUS, p->n_left->n_right)){
			zapleft:

			q = p->n_left->n_left;
			nfree(p->n_left->n_right);
			nfree(p->n_left);
			p->n_left = q;
		}
		if( RCON(p) && LO(p)==o && RCON(p->n_left) &&
		    conval( p->n_right, o, p->n_left->n_right ) ){
			goto zapleft;
			}
		else if( LCON(p) && RCON(p) && conval( p->n_left, o, p->n_right ) ){
			zapright:
			nfree(p->n_right);
			q = makety(p->n_left, p->n_type, p->n_qual,
			    p->n_df, p->n_ap);
			nfree(p);
			p = clocal(q);
			break;
			}

		/* change muls to shifts */

		if( o == MUL && nncon(p->n_right) && (i=ispow2(RV(p)))>=0){
			if( i == 0 ) { /* multiplication by 1 */
				goto zapright;
				}
			o = p->n_op = LS;
			p->n_right->n_type = INT;
			p->n_right->n_df = NULL;
			RV(p) = i;
			}

		/* change +'s of negative consts back to - */
		if( o==PLUS && nncon(p->n_right) && RV(p)<0 ){
			RV(p) = -RV(p);
			o = p->n_op = MINUS;
			}

		/* remove ops with RHS 0 */
		if ((o == PLUS || o == MINUS || o == OR || o == ER) &&
		    nncon(p->n_right) && RV(p) == 0) {
			goto zapright;
		}
		break;

	case DIV:
		if( nncon( p->n_right ) && p->n_right->n_lval == 1 )
			goto zapright;
		if (LCON(p) && RCON(p) && conval(p->n_left, DIV, p->n_right))
			goto zapright;
		if (RCON(p) && ISUNSIGNED(p->n_type) && (i=ispow2(RV(p))) > 0) {
			p->n_op = RS;
			RV(p) = i;
			q = p->n_right;
			if(tsize(q->n_type, q->n_df, q->n_ap) > SZINT)
				p->n_right = makety(q, INT, 0, 0, 0);

			break;
		}
		break;

	case MOD:
		if (RCON(p) && ISUNSIGNED(p->n_type) && ispow2(RV(p)) > 0) {
			p->n_op = AND;
			RV(p) = RV(p) -1;
			break;
		}
		break;

	case EQ:
	case NE:
	case LT:
	case LE:
	case GT:
	case GE:
	case ULT:
	case ULE:
	case UGT:
	case UGE:
		if (LCON(p) && RCON(p) &&
		    !ISPTR(p->n_left->n_type) && !ISPTR(p->n_right->n_type)) {
			/* Do constant evaluation */
			q = p->n_left;
			if (conval(q, o, p->n_right)) {
				nfree(p->n_right);
				nfree(p);
				p = q;
				break;
			}
		}

		if( !LCON(p) ) break;

		/* exchange operands */

		sp = p->n_left;
		p->n_left = p->n_right;
		p->n_right = sp;
		p->n_op = revrel[p->n_op - EQ ];
		break;

	case CBRANCH:
		if (LCON(p)) {
			if (LV(p) == 0) {
				tfree(p);
				p = bcon(0);
			} else {
				tfree(p->n_left);
				p->n_left = p->n_right;
				p->n_op = GOTO;
			}
		}
		break;
				

#ifdef notyet
	case ASSIGN:
		/* Simple test to avoid two branches */
		if (RO(p) != NE)
			break;
		q = p->n_right;
		if (RCON(q) && RV(q) == 0 && LO(q) == AND &&
		    RCON(q->n_left) && (i = ispow2(RV(q->n_left))) &&
		    q->n_left->n_type == INT) {
			q->n_op = RS;
			RV(q) = i;
		}
		break;
#endif
	}

	return(p);
	}

int
ispow2(CONSZ c)
{
	int i;
	if( c <= 0 || (c&(c-1)) ) return(-1);
	for( i=0; c>1; ++i) c >>= 1;
	return(i);
}

int
nncon(NODE *p)
{
	/* is p a constant without a name */
	return( p->n_op == ICON && p->n_sp == NULL );
}
