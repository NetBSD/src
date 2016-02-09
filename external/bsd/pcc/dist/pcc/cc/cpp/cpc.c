/*      Id: cpc.c,v 1.7 2016/01/10 16:17:45 ragge Exp       */	
/*      $NetBSD: cpc.c,v 1.1.1.1 2016/02/09 20:28:42 plunky Exp $      */

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

/*
 * Small recursive parser for #if statements.
 */

#include "cpp.h"

static int ctok;

typedef struct nd ND;
struct nd yynode;

void qloop(void (*fun)(ND *), ND *n1, int a0, int a1, int a2, int a3);
void shft(void);
int yyparse(void);
void expr(ND *n1);
void eqcol(ND *n1);
void eoror(ND *n1);
void eandand(ND *n1);
void exor(ND *n1);
void eand(ND *n1);
void eqne(ND *n1);
void eget(ND *n1);
void elrs(ND *n1);
void eplmin(ND *n1);
void emdv(ND *n1);
void eterm(ND *n1);
void eval(int op, ND *n1, ND *n2);


void
shft(void)
{
	ctok = yylex();
}

int
yyparse(void)
{
	ND n1;

	shft();
	expr(&n1);
	if (n1.op == 0)
		error("division by zero");
	if (ctok != WARN)
		error("junk after expression");
	return (int)n1.nd_val;
}

/*
 * evaluate a complete preprocessor #if expression.
 */
void
expr(ND *n1)
{
	for (;;) {
		eqcol(n1);
		if (ctok != ',')
			return;
		shft();
	}
}

/*
 * evaluate ?: conditionals.
 */
void
eqcol(ND *n1)
{
	ND n2, n3;

	eoror(n1);
	if (ctok != '?')
		return;

	shft();
	expr(&n2);
	if (ctok != ':')
		error("no : found");
	shft();
	eqcol(&n3);
	if (n1->nd_val) 
		n1->nd_val = n2.nd_val, n1->op = n2.op;
	else
		n1->nd_val = n3.nd_val, n1->op = n3.op;
}

void
eoror(ND *n1)
{
	qloop(eandand, n1, OROR, 0, 0, 0);
}

void
eandand(ND *n1)
{
	qloop(exor, n1, ANDAND, 0, 0, 0);
}

void
exor(ND *n1)
{
	qloop(eand, n1, '|', '^', 0, 0);
}

void
eand(ND *n1)
{
	qloop(eqne, n1, '&', 0, 0, 0);
}

void
eqne(ND *n1)
{
	qloop(eget, n1, EQ, NE, 0, 0);
}

void
eget(ND *n1)
{
	qloop(elrs, n1, '<', '>', LE, GE);
}

void
elrs(ND *n1)
{
	qloop(eplmin, n1, LS, RS, 0, 0);
}

void
eplmin(ND *n1)
{
	qloop(emdv, n1, '+', '-', 0, 0);
}

void
emdv(ND *n1)
{
	qloop(eterm, n1, '*', '/', '%', 0);
}

/*
 * Loop to evaluate all operators on the same precedence level.
 */
void
qloop(void (*fun)(ND *), ND *n1, int a0, int a1, int a2, int a3)
{
	ND n2;
	int op;

	fun(n1);
	while (ctok == a0 || ctok == a1 || ctok == a2 || ctok == a3) {
		op = ctok;
		shft();
		fun(&n2);
		eval(op, n1, &n2);
	}
}

static void
gnum(int o, ND *n1)
{
	n1->op = yynode.op;
	n1->nd_val = yynode.nd_val;
	shft();
}

/*
 * Highest precedence operators + numeric terminals.
 */
void
eterm(ND *n1)
{
	int o = ctok;

	switch (o) {
	case '~':
	case '!':
	case '+':
	case '-':
		shft();
		eterm(n1);
		eval(o, n1, 0);
		break;

	case NUMBER:
	case UNUMBER:
		gnum(o, n1);
		break;

	case '(':
		shft();
		expr(n1);
		if (ctok == ')') {
			shft();
			break;
		}
		/* FALLTHROUGH */
	default:
		error("bad terminal (%d)", o);
		break;
	}
}

/*
 * keep all numeric evaluation here.
 * evaluated value returned in n1.
 */
void
eval(int op, ND *n1, ND *n2)
{

	if ((op == '/' || op == '%') && n2->nd_val == 0)
		n1->op = 0;

	if (n1->op == 0)
		return; /* div by zero involved */

	/* unary ops */
	if (n2 == 0) {
		switch (op) {
		case '+': break;
		case '-': n1->nd_val = -n1->nd_val; break;
		case '~': n1->nd_val = ~n1->nd_val; break;
		case '!': n1->nd_val = !n1->nd_val; n1->op = NUMBER; break;
		}
		return;
	} 

	if (op == OROR && n1->nd_val) {
		n1->nd_val = 1, n1->op = NUMBER;
		return;
	}
	if (op == ANDAND && n1->nd_val == 0) {
		n1->op = NUMBER;
		return;
	}

	if (n2->op == 0) {
		n1->op = 0;
		return;
	}

	if (n2->op == UNUMBER)
		n1->op = UNUMBER;

	switch (op) {
	case OROR:
		if (n2->nd_val)
			n1->nd_val = 1;
		n1->op = NUMBER;
		break;
	case ANDAND:
		n1->nd_val = n2->nd_val != 0;
		n1->op = NUMBER;
		break;
	case '+': n1->nd_val += n2->nd_val; break;
	case '-': n1->nd_val -= n2->nd_val; break;
	case '|': n1->nd_val |= n2->nd_val; break;
	case '^': n1->nd_val ^= n2->nd_val; break;
	case '&': n1->nd_val &= n2->nd_val; break;
	case LS: n1->nd_val <<= n2->nd_val; break;
	case EQ: n1->nd_val = n1->nd_val == n2->nd_val; n1->op = NUMBER; break;
	case NE: n1->nd_val = n1->nd_val != n2->nd_val; n1->op = NUMBER; break;
	}

	if (n1->op == NUMBER) {
		switch (op) {
		case '*': n1->nd_val *= n2->nd_val; break;
		case '/': n1->nd_val /= n2->nd_val; break;
		case '%': n1->nd_val %= n2->nd_val; break;
		case '<': n1->nd_val = n1->nd_val < n2->nd_val; break;
		case '>': n1->nd_val = n1->nd_val > n2->nd_val; break;
		case LE: n1->nd_val = n1->nd_val <= n2->nd_val; break;
		case GE: n1->nd_val = n1->nd_val >= n2->nd_val; break;
		case RS: n1->nd_val >>= n2->nd_val; break;
		}
		return;
	} else /* op == UNUMBER */ {
		switch (op) {
		case '*': n1->nd_uval *= n2->nd_uval; break;
		case '/': n1->nd_uval /= n2->nd_uval; break;
		case '%': n1->nd_uval %= n2->nd_uval; break;
		case '<': n1->nd_uval = n1->nd_uval < n2->nd_uval; break;
		case '>': n1->nd_uval = n1->nd_uval > n2->nd_uval; break;
		case LE: n1->nd_uval = n1->nd_uval <= n2->nd_uval; break;
		case GE: n1->nd_uval = n1->nd_uval >= n2->nd_uval; break;
		case RS: n1->nd_uval >>= n2->nd_uval; break;
		}
		return;
	}
	error("unexpected arithmetic");
}
