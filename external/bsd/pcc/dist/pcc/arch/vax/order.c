/*	Id: order.c,v 1.10 2014/10/12 13:16:32 ragge Exp 	*/	
/*	$NetBSD: order.c,v 1.1.1.4 2016/02/09 20:28:36 plunky Exp $	*/
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

int canaddr(NODE *);

int maxargs = { -1 };

#if 0
stoasg( p, o ) register NODE *p; {
	/* should the assignment op p be stored,
	   given that it lies as the right operand of o
	   (or the left, if o==UNARY MUL) */
/*
	if( p->op == INCR || p->op == DECR ) return;
	if( o==UNARY MUL && p->left->op == REG && !isbreg(p->left->rval) ) SETSTO(p,INAREG);
 */
	}

int
deltest( p ) register NODE *p; {
	/* should we delay the INCR or DECR operation p */
	p = p->n_left;
	return( p->n_op == REG || p->n_op == NAME || p->n_op == OREG );
	}

autoincr( p ) NODE *p; {
	register NODE *q = p->left, *r;

	if( q->op == INCR && (r=q->left)->op == REG &&
	    ISPTR(q->type) && p->type == DECREF(q->type) &&
	    tlen(p) == q->right->lval ) return(1);

	return(0);
	}

mkadrs(p) register NODE *p; {
	register o;

	o = p->op;

	if( asgop(o) ){
		if( p->left->su >= p->right->su ){
			if( p->left->op == UNARY MUL ){
				SETSTO( p->left->left, INTEMP );
				}
			else if( p->left->op == FLD && p->left->left->op == UNARY MUL ){
				SETSTO( p->left->left->left, INTEMP );
				}
			else { /* should be only structure assignment */
				SETSTO( p->left, INTEMP );
				}
			}
		else SETSTO( p->right, INTEMP );
		}
	else {
		if( p->left->su > p->right->su ){
			SETSTO( p->left, INTEMP );
			}
		else {
			SETSTO( p->right, INTEMP );
			}
		}
	}
#endif

int
notoff(TWORD t, int r, CONSZ off, char *cp)
{
	/* is it legal to make an OREG or NAME entry which has an
	 * offset of off, (from a register of r), if the
	 * resulting thing had type t */

/*	if( r == R0 ) return( 1 );  / * NO */
	return(0);  /* YES */
	}

# define max(x,y) ((x)<(y)?(y):(x))

#if 0
sucomp( p ) register NODE *p; {

	/* set the su field in the node to the sethi-ullman
	   number, or local equivalent */

	register o, ty, sul, sur, r;

	o = p->op;
	ty = optype( o );
	p->su = szty( p->type );   /* 2 for float or double, else 1 */;

	if( ty == LTYPE ){
		if( o == OREG ){
			r = p->rval;
			/* oreg cost is (worst case) 1 + number of temp registers used */
			if( R2TEST(r) ){
				if( R2UPK1(r)!=100 && istreg(R2UPK1(r)) ) ++p->su;
				if( istreg(R2UPK2(r)) ) ++p->su;
				}
			else {
				if( istreg( r ) ) ++p->su;
				}
			}
		if( p->su == szty(p->type) &&
		   (p->op!=REG || !istreg(p->rval)) &&
		   (p->type==INT || p->type==UNSIGNED || p->type==DOUBLE) )
			p->su = 0;
		return;
		}

	else if( ty == UTYPE ){
		switch( o ) {
		case UNARY CALL:
		case UNARY STCALL:
			p->su = fregs;  /* all regs needed */
			return;

		default:
			p->su =  p->left->su + (szty( p->type ) > 1 ? 2 : 0) ;
			return;
			}
		}


	/* If rhs needs n, lhs needs m, regular su computation */

	sul = p->left->su;
	sur = p->right->su;

	if( o == ASSIGN ){
		/* computed by doing right, then left (if not in mem), then doing it */
		p->su = max(sur,sul+1);
		return;
		}

	if( o == CALL || o == STCALL ){
		/* in effect, takes all free registers */
		p->su = fregs;
		return;
		}

	if( o == STASG ){
		/* right, then left */
		p->su = max( max( 1+sul, sur), fregs );
		return;
		}

	if( asgop(o) ){
		/* computed by doing right, doing left address, doing left, op, and store */
		p->su = max(sur,sul+2);
/*
		if( o==ASG MUL || o==ASG DIV || o==ASG MOD) p->su = max(p->su,fregs);
 */
		return;
		}

	switch( o ){
	case ANDAND:
	case OROR:
	case QUEST:
	case COLON:
	case COMOP:
		p->su = max( max(sul,sur), 1);
		return;

	case PLUS:
	case OR:
	case ER:
		/* commutative ops; put harder on left */
		if( p->right->su > p->left->su && !istnode(p->left) ){
			register NODE *temp;
			temp = p->left;
			p->left = p->right;
			p->right = temp;
			}
		break;
		}

	/* binary op, computed by left, then right, then do op */
	p->su = max(sul,szty(p->right->type)+sur);
/*
	if( o==MUL||o==DIV||o==MOD) p->su = max(p->su,fregs);
 */

	}

int radebug = 0;

rallo( p, down ) NODE *p; {
	/* do register allocation */
	register o, type, down1, down2, ty;

	if( radebug ) printf( "rallo( %o, %d )\n", p, down );

	down2 = NOPREF;
	p->rall = down;
	down1 = ( down &= ~MUSTDO );

	ty = optype( o = p->op );
	type = p->type;


	if( type == DOUBLE || type == FLOAT ){
		if( o == FORCE ) down1 = R0|MUSTDO;
		}
	else switch( o ) {
	case ASSIGN:	
		down1 = NOPREF;
		down2 = down;
		break;

/*
	case MUL:
	case DIV:
	case MOD:
		down1 = R3|MUSTDO;
		down2 = R5|MUSTDO;
		break;

	case ASG MUL:
	case ASG DIV:
	case ASG MOD:
		p->left->rall = down1 = R3|MUSTDO;
		if( p->left->op == UNARY MUL ){
			rallo( p->left->left, R4|MUSTDO );
			}
		else if( p->left->op == FLD  && p->left->left->op == UNARY MUL ){
			rallo( p->left->left->left, R4|MUSTDO );
			}
		else rallo( p->left, R3|MUSTDO );
		rallo( p->right, R5|MUSTDO );
		return;
 */

	case CALL:
	case STASG:
	case EQ:
	case NE:
	case GT:
	case GE:
	case LT:
	case LE:
	case NOT:
	case ANDAND:
	case OROR:
		down1 = NOPREF;
		break;

	case FORCE:	
		down1 = R0|MUSTDO;
		break;

		}

	if( ty != LTYPE ) rallo( p->left, down1 );
	if( ty == BITYPE ) rallo( p->right, down2 );

	}
#endif

/*
 * Turn a UMUL-referenced node into OREG.
 * Be careful about register classes, this is a place where classes change.
 */
void
offstar(NODE *p, int shape)
{

	if (x2debug)
		printf("offstar(%p)\n", p);

	if (isreg(p))
		return; /* Is already OREG */

#if 0 /* notyet */
	NODE *r;
	r = p->n_right;
	if( p->n_op == PLUS || p->n_op == MINUS ){
		if( r->n_op == ICON ){
			if (isreg(p->n_left) == 0)
				(void)geninsn(p->n_left, INAREG);
			/* Converted in ormake() */
			return;
		}
		if (r->n_op == LS && r->n_right->n_op == ICON &&
		    r->n_right->n_lval == 2 && p->n_op == PLUS) {
			if (isreg(p->n_left) == 0)
				(void)geninsn(p->n_left, INAREG);
			if (isreg(r->n_left) == 0)
				(void)geninsn(r->n_left, INAREG);
			return;
		}
	}
#endif
	(void)geninsn(p, INAREG);
}


#if 0
void
offstar( p, s ) register NODE *p; {
	if( p->n_op == PLUS ) {
		if( p->n_left->n_su == fregs ) {
			order( p->n_left, INAREG );
			return;
		} else if( p->n_right->n_su == fregs ) {
			order( p->n_right, INAREG );
			return;
		}
		if( p->n_left->n_op==LS && 
		  (p->n_left->n_left->n_op!=REG || tlen(p->n_left->n_left)!=sizeof(int) ) ) {
			order( p->n_left->n_left, INAREG );
			return;
		}
		if( p->n_right->n_op==LS &&
		  (p->n_right->n_left->n_op!=REG || tlen(p->n_right->n_left)!=sizeof(int) ) ) {
			order( p->n_right->n_left, INAREG );
			return;
		}
		if( p->n_type == (PTR|CHAR) || p->n_type == (PTR|UCHAR) ) {
			if( p->n_left->n_op!=REG || tlen(p->n_left)!=sizeof(int) ) {
				order( p->n_left, INAREG );
				return;
			}
			else if( p->n_right->n_op!=REG || tlen(p->n_right)!=sizeof(int) ) {
				order(p->n_right, INAREG);
				return;
			}
		}
	}
	if( p->n_op == PLUS || p->n_op == MINUS ){
		if( p->n_right->n_op == ICON ){
			p = p->n_left;
			order( p , INAREG);
			return;
			}
		}

	if( p->n_op == UMUL && !canaddr(p) ) {
		offstar( p->n_left, 0 );
		return;
	}

	order( p, INAREG );
	}
#endif

int
setbin( p ) register NODE *p; {

#if 0
	register int ro, rt;

	rt = p->n_right->n_type;
	ro = p->n_right->n_op;

	if( canaddr( p->n_left ) && !canaddr( p->n_right ) ) { /* address rhs */
		if( ro == UMUL ) {
			offstar( p->n_right->n_left, 0 );
			return(1);
		} else {
			order( p->n_right, INAREG|SOREG );
			return(1);
		}
	}
	if( !istnode( p->n_left) ) { /* try putting LHS into a reg */
/*		order( p->n_left, logop(p->n_op)?(INAREG|INBREG|INTAREG|INTBREG|SOREG):(INTAREG|INTBREG|SOREG) );*/
		order( p->n_left, INAREG|INTAREG|INBREG|INTBREG|SOREG );
		return(1);
		}
	else if( ro == UNARY MUL && rt != CHAR && rt != UCHAR ){
		offstar( p->n_right->n_left );
		return(1);
		}
	else if( rt == CHAR || rt == UCHAR || rt == SHORT || rt == USHORT || (ro != REG &&
			ro != NAME && ro != OREG && ro != ICON ) ){
		order( p->n_right, INAREG|INBREG );
		return(1);
		}
/*
	else if( logop(p->n_op) && rt==USHORT ){  / * must get rhs into register */
/*
		order( p->n_right, INAREG );
		return( 1 );
		}
 */
#endif
	return(0);
	}

#if 0
int
setstr( p ) register NODE *p; { /* structure assignment */
	if( p->right->op != REG ){
		order( p->right, INTAREG );
		return(1);
		}
	p = p->left;
	if( p->op != NAME && p->op != OREG ){
		if( p->op != UNARY MUL ) cerror( "bad setstr" );
		order( p->left, INTAREG );
		return( 1 );
		}
	return( 0 );
	}
#endif

int
setasg( p, s ) register NODE *p; {

#if 0
	/* setup for assignment operator */

	if( !canaddr(p->n_right) ) {
		if( p->n_right->n_op == UNARY MUL )
			offstar(p->n_right->n_left);
		else
			order( p->n_right, INAREG|INBREG|SOREG );
		return(1);
		}
	if( p->n_left->n_op == UMUL ) {
		offstar( p->n_left->n_left );
		return(1);
		}
	if( p->left->op == FLD && p->left->left->op == UNARY MUL ){
		offstar( p->left->left->left );
		return(1);
		}
/* FLD patch */
	if( p->left->op == FLD && !(p->right->type==INT || p->right->type==UNSIGNED)) {
		order( p->right, INAREG);
		return(1);
		}
/* end of FLD patch */
#endif
	return(0);
	}

/* setup for unary operator */
int
setuni(NODE *p, int cookie)
{
	return 0;
}


#if 0
int
setasop( p ) register NODE *p; {
	/* setup for =ops */
	register rt, ro;

	rt = p->right->type;
	ro = p->right->op;

	if( ro == UNARY MUL && rt != CHAR ){
		offstar( p->right->left );
		return(1);
		}
	if( ( rt == CHAR || rt == SHORT || rt == UCHAR || rt == USHORT ||
			( ro != REG && ro != ICON && ro != NAME && ro != OREG ) ) ){
		order( p->right, INAREG|INBREG );
		return(1);
		}
/*
	if( (p->op == ASG LS || p->op == ASG RS) && ro != ICON && ro != REG ){
		order( p->right, INAREG );
		return(1);
		}
 */


	p = p->left;
	if( p->op == FLD ) p = p->left;

	switch( p->op ){

	case REG:
	case ICON:
	case NAME:
	case OREG:
		return(0);

	case UNARY MUL:
		if( p->left->op==OREG )
			return(0);
		else
			offstar( p->left );
		return(1);

		}
	cerror( "illegal setasop" );
	}
#endif

void
deflab(int l)
{
	printf(LABFMT ":\n", l);
}

#if 0
genargs( p, ptemp ) register NODE *p, *ptemp; {
	register NODE *pasg;
	register align;
	register size;
	register TWORD type;

	/* generate code for the arguments */

	/*  first, do the arguments on the right */
	while( p->op == CM ){
		genargs( p->right, ptemp );
		p->op = FREE;
		p = p->left;
		}

	if( p->op == STARG ){ /* structure valued argument */

		size = p->stsize;
		align = p->stalign;

 /*		ptemp->lval = (ptemp->lval/align)*align;  / * SETOFF for negative numbers */
 		ptemp->lval = 0;	/* all moves to (sp) */

		p->op = STASG;
		p->right = p->left;
		p->left = tcopy( ptemp );

		/* the following line is done only with the knowledge
		that it will be undone by the STASG node, with the
		offset (lval) field retained */

		if( p->right->op == OREG ) p->right->op = REG;  /* only for temporaries */

 		order( p, FORARG );
		ptemp->lval += size;
		return;
		}

	/* ordinary case */

	order( p, FORARG );
	}

argsize( p ) register NODE *p; {
	register t;
	t = 0;
	if( p->op == CM ){
		t = argsize( p->left );
		p = p->right;
		}
	if( p->type == DOUBLE || p->type == FLOAT ){
		SETOFF( t, 4 );
		return( t+8 );
		}
	else if( p->op == STARG ){
 		SETOFF( t, 4 );  /* alignment */
 		return( t + ((p->stsize+3)/4)*4 );  /* size */
		}
	else {
		SETOFF( t, 4 );
		return( t+4 );
		}
	}
#endif

/*
 * Special handling of some instruction register allocation.
 */
struct rspecial *
nspecial(struct optab *q)
{
	switch (q->op) {
	case STARG:
	case STASG:
		{
		static struct rspecial s[] = {
		    { NEVER, R0, }, { NEVER, R1, }, { NEVER, R2, },
		    { NEVER, R3, }, { NEVER, R4, }, { NEVER, R5 }, { 0 } };
		return s;
		}
	case MOD:
	case MUL:
	case DIV:
		{
		static struct rspecial s[] = {
		    { NEVER, R0, }, { NEVER, R1, }, { NEVER, R2, },
		    { NEVER, R3, }, { NEVER, R4, }, { NEVER, R5 },
		    { NRES, XR0 }, { 0 }, };
		return s;
		}
	default:
		comperr("nspecial");
		return NULL;
	}
}

/*
 * Set evaluation order of a binary node if it differs from default.
 */
int
setorder(NODE *p)
{
	return 0; /* nothing differs on vax */
}

/*
 * Do the actual conversion of offstar-found OREGs into real OREGs.
 */
void
myormake(NODE *q)
{
}
/*
 * Set registers "live" at function calls (like arguments in registers).
 * This is for liveness analysis of registers.
 */
int *
livecall(NODE *p)
{
	static int r[1] = { -1 }; /* Terminate with -1 */

	return &r[0];
}

/*
 * Signal whether the instruction is acceptable for this target.
 */
int
acceptable(struct optab *op)
{
	return 1;
}
