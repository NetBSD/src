/*	$NetBSD: npf_parse.y,v 1.6 2012/02/26 22:04:42 christos Exp $	*/

/*-
 * Copyright (c) 2011-2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Martin Husemann and Christos Zoulas.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

%{

#include <stdio.h>
#include <err.h>
#include <vis.h>
#include <netdb.h>
#include <assert.h>

#include "npfctl.h"

const char *		yyfilename;

extern int		yylineno, yycolumn;
extern int		yylex(void);

/* Variable under construction (bottom up). */
static npfvar_t *	cvar;

void
yyerror(const char *fmt, ...)
{
	extern int yyleng;
	extern char *yytext;

	char *msg, *context = xstrndup(yytext, yyleng);
	size_t len = strlen(context);
	char *dst = zalloc(len * 4 + 1);
	va_list ap;

	va_start(ap, fmt);
	vasprintf(&msg, fmt, ap);
	va_end(ap);

	strvisx(dst, context, len, VIS_WHITE|VIS_CSTYLE);
	fprintf(stderr, "%s:%d:%d: %s near '%s'\n", yyfilename, yylineno,
	    yycolumn, msg, dst);
	exit(EXIT_FAILURE);
}

%}

%token			ALL
%token			ANY
%token			APPLY
%token			ARROW
%token			BINAT
%token			BLOCK
%token			CURLY_CLOSE
%token			CURLY_OPEN
%token			CODE
%token			COLON
%token			COMMA
%token			DEFAULT
%token			TDYNAMIC
%token			EQ
%token			TFILE
%token			FLAGS
%token			FROM
%token			GROUP
%token			HASH
%token			ICMPTYPE
%token			ID
%token			IN
%token			INET
%token			INET6
%token			INTERFACE
%token			KEEP
%token			MINUS
%token			NAT
%token			NAME
%token			ON
%token			OUT
%token			PAR_CLOSE
%token			PAR_OPEN
%token			PASS
%token			PORT
%token			PROCEDURE
%token			PROTO
%token			FAMILY
%token			QUICK
%token			RDR
%token			RETURN
%token			RETURNICMP
%token			RETURNRST
%token			SEPLINE
%token			SLASH
%token			STATE
%token			TABLE
%token			TCP
%token			TO
%token			TREE
%token			TYPE
%token			UDP
%token			ICMP

%token	<num>		HEX
%token	<str>		IDENTIFIER
%token	<str>		IPV4ADDR
%token	<str>		IPV6ADDR
%token	<num>		NUM
%token	<str>		STRING
%token	<str>		TABLE_ID
%token	<str>		VAR_ID

%type	<str>		addr, iface_name, moduleargname, list_elem, table_store
%type	<str>		opt_apply
%type	<num>		ifindex, port, opt_quick, on_iface
%type	<num>		block_or_pass, rule_dir, block_opts, family, opt_family
%type	<num>		opt_keep_state, icmp_type, table_type
%type	<var>		addr_or_iface, port_range, icmp_type_and_code
%type	<var>		filt_addr, addr_and_mask, tcp_flags, tcp_flags_and_mask
%type	<var>		modulearg_opts, procs, proc_op, modulearg, moduleargs
%type	<filtopts>	filt_opts, all_or_filt_opts
%type	<optproto>	opt_proto
%type	<rulegroup>	group_attr, group_opt

%union {
	char *		str;
	unsigned long	num;
	filt_opts_t	filtopts;
	npfvar_t *	var;
	opt_proto_t	optproto;
	rule_group_t	rulegroup;
}

%%

input
	: lines
	;

lines
	: line SEPLINE lines
	| line
	;

line
	: def
	| table
	| nat
	| group
	| rproc
	|
	;

def
	: VAR_ID
	{
		cvar = npfvar_create($1);
		npfvar_add(cvar);
	}
	  EQ definition
	{
		cvar = NULL;
	}
	;

definition
	: list_elem
	| listdef
	;

listdef
	: CURLY_OPEN list_elems CURLY_CLOSE
	;

list_elems
	: list_elem COMMA list_elems
	| list_elem
	;

list_elem
	: IDENTIFIER
	{
		npfvar_t *vp = npfvar_create(".identifier");
		npfvar_add_element(vp, NPFVAR_IDENTIFIER, $1, strlen($1) + 1);
		npfvar_add_elements(cvar, vp);
	}
	| STRING
	{
		npfvar_t *vp = npfvar_create(".string");
		npfvar_add_element(vp, NPFVAR_STRING, $1, strlen($1) + 1);
		npfvar_add_elements(cvar, vp);
	}
	| NUM MINUS NUM
	{
		npfvar_t *vp = npfctl_parse_port_range($1, $3);
		npfvar_add_elements(cvar, vp);
	}
	| NUM
	{
		npfvar_t *vp = npfvar_create(".num");
		npfvar_add_element(vp, NPFVAR_NUM, &$1, sizeof($1));
		npfvar_add_elements(cvar, vp);
	}
	| VAR_ID
	{
		npfvar_t *vp = npfvar_create(".var_id");
		npfvar_add_element(vp, NPFVAR_VAR_ID, $1, strlen($1) + 1);
		npfvar_add_elements(cvar, vp);
	}
	| addr_and_mask
	{
		npfvar_add_elements(cvar, $1);
	}
	;

table
	: TABLE TABLE_ID TYPE table_type table_store
	{
		npfctl_build_table($2, $4, $5);
	}
	;

table_type
	: HASH		{ $$ = NPF_TABLE_HASH; }
	| TREE		{ $$ = NPF_TABLE_TREE; }
	;

table_store
	: TDYNAMIC	{ $$ = NULL; }
	| TFILE STRING	{ $$ = $2; }
	;

nat
	: natdef
	| binatdef
	| rdrdef
	;

natdef
	: NAT ifindex filt_opts ARROW addr_or_iface
	{
		npfctl_build_nat(NPFCTL_NAT, $2, &$3, $5, NULL);
	}
	;

binatdef
	: BINAT ifindex filt_opts ARROW addr_or_iface
	{
		npfctl_build_nat(NPFCTL_BINAT, $2, &$3, $5, $3.fo_from);
	}
	;

rdrdef
	: RDR ifindex filt_opts ARROW addr_or_iface port_range
	{
		npfctl_build_nat(NPFCTL_RDR, $2, &$3, $5, $6);
	}
	;

rproc
	: PROCEDURE STRING CURLY_OPEN procs CURLY_CLOSE
	{
		npfctl_build_rproc($2, $4);
	}
	;

procs
	: proc_op SEPLINE procs	{ $$ = npfvar_add_elements($1, $3); }
	| proc_op		{ $$ = $1; }
	;

proc_op
	: IDENTIFIER COLON moduleargs
	{
		proc_op_t po;

		po.po_name = xstrdup($1);
		po.po_opts = $3;
		$$ = npfvar_create(".proc_ops");
		npfvar_add_element($$, NPFVAR_PROC_OP, &po, sizeof(po));
	}
	|	{ $$ = NULL; }
	;

moduleargs
	: modulearg COMMA moduleargs
	{
		$$ = npfvar_add_elements($1, $3);
	}
	| modulearg	{ $$ = $1; }
	|		{ $$ = NULL; }
	;

modulearg
	: moduleargname modulearg_opts
	{
		module_arg_t ma;

		ma.ma_name = xstrdup($1);
		ma.ma_opts = $2;
		$$ = npfvar_create(".module_arg");
		npfvar_add_element($$, NPFVAR_MODULE_ARG, &ma, sizeof(ma));
	}
	;

moduleargname
	: STRING	{ $$ = $1; }
	| IDENTIFIER	{ $$ = $1; }
	;

modulearg_opts
	: STRING modulearg_opts
	{
		npfvar_t *vp = npfvar_create(".modstring");
		npfvar_add_element(vp, NPFVAR_STRING, $1, strlen($1) + 1);
		$$ = $2 ? npfvar_add_elements($2, vp) : vp;
	}
	| IDENTIFIER modulearg_opts
	{
		npfvar_t *vp = npfvar_create(".modident");
		npfvar_add_element(vp, NPFVAR_IDENTIFIER, $1, strlen($1) + 1);
		$$ = $2 ? npfvar_add_elements($2, vp) : vp;
	}
	| NUM modulearg_opts
	{
		npfvar_t *vp = npfvar_create(".modnum");
		npfvar_add_element(vp, NPFVAR_NUM, &$1, sizeof($1));
		$$ = $2 ? npfvar_add_elements($2, vp) : vp;
	}
	|	{ $$ = NULL; }
	;

group
	: GROUP PAR_OPEN group_attr PAR_CLOSE
	{
		npfctl_build_group($3.rg_name, $3.rg_attr, $3.rg_ifnum);
	}
	  ruleset
	;

group_attr
	: group_opt COMMA group_attr
	{
		$$ = $3;

		if (($1.rg_name && $$.rg_name) ||
		    ($1.rg_ifnum && $$.rg_ifnum) ||
		    ($1.rg_attr & $$.rg_attr) != 0)
			yyerror("duplicate group option");

		if ($1.rg_name) {
			$$.rg_name = $1.rg_name;
		}
		if ($1.rg_attr) {
			$$.rg_attr |= $1.rg_attr;
		}
		if ($1.rg_ifnum) {
			$$.rg_ifnum = $1.rg_ifnum;
		}
	}
	| group_opt		{ $$ = $1; }
	;

group_opt
	: DEFAULT
	{
		$$.rg_name = NULL;
		$$.rg_ifnum = 0;
		$$.rg_attr = NPF_RULE_DEFAULT;
	}
	| NAME STRING
	{
		$$.rg_name = $2;
		$$.rg_ifnum = 0;
		$$.rg_attr = 0;
	}
	| INTERFACE ifindex
	{
		$$.rg_name = NULL;
		$$.rg_ifnum = $2;
		$$.rg_attr = 0;
	}
	| rule_dir
	{
		$$.rg_name = NULL;
		$$.rg_ifnum = 0;
		$$.rg_attr = $1;
	}
	;

ruleset
	: CURLY_OPEN rules CURLY_CLOSE
	;

rules
	: rule SEPLINE rules
	| rule
	;

rule
	: block_or_pass rule_dir opt_quick on_iface opt_family
	  opt_proto all_or_filt_opts opt_keep_state opt_apply
	{
		/*
		 * Arguments: attributes, interface index, address
		 * family, protocol options, filter options.
		 */
		npfctl_build_rule($1 | $2 | $3 | $8, $4,
		    $5, &$6, &$7, $9);
	}
	|
	;

block_or_pass
	: BLOCK block_opts	{ $$ = $2; }
	| PASS			{ $$ = NPF_RULE_PASS; }
	;

rule_dir
	: IN			{ $$ = NPF_RULE_IN; }
	| OUT			{ $$ = NPF_RULE_OUT; }
	|			{ $$ = NPF_RULE_IN | NPF_RULE_OUT; }
	;

opt_quick
	: QUICK			{ $$ = NPF_RULE_FINAL; }
	|			{ $$ = 0; }
	;

on_iface
	: ON ifindex		{ $$ = $2; }
	|			{ $$ = 0; }
	;

family
	: INET			{ $$ = AF_INET; }
	| INET6			{ $$ = AF_INET6; }
	;

opt_proto
	: PROTO TCP tcp_flags_and_mask
	{
		$$.op_proto = IPPROTO_TCP;
		$$.op_opts = $3;
	}
	| PROTO ICMP icmp_type_and_code
	{
		$$.op_proto = IPPROTO_ICMP;
		$$.op_opts = $3;
	}
	| PROTO UDP
	{
		$$.op_proto = IPPROTO_UDP;
		$$.op_opts = NULL;
	}
	|
	{
		$$.op_proto = -1;
		$$.op_opts = NULL;
	}
	;

opt_family
	: FAMILY family		{ $$ = $2; }
	|			{ $$ = AF_UNSPEC; }
	;

all_or_filt_opts
	: ALL
	{
		$$.fo_from = NULL;
		$$.fo_from_port_range = NULL;
		$$.fo_to = NULL;
		$$.fo_to_port_range = NULL;
	}
	| filt_opts	{ $$ = $1; }
	;

opt_keep_state
	: KEEP STATE	{ $$ = NPF_RULE_KEEPSTATE; }
	|		{ $$ = 0; }
	;

opt_apply
	: APPLY STRING	{ $$ = $2; }
	|		{ $$ = NULL; }
	;

block_opts
	: RETURNRST	{ $$ = NPF_RULE_RETRST; }
	| RETURNICMP	{ $$ = NPF_RULE_RETICMP; }
	| RETURN	{ $$ = NPF_RULE_RETRST | NPF_RULE_RETICMP; }
	|		{ $$ = 0; }
	;

filt_opts
	: FROM filt_addr port_range TO filt_addr port_range
	{
		$$.fo_from = $2;
		$$.fo_from_port_range = $3;
		$$.fo_to = $5;
		$$.fo_to_port_range = $6;
	}
	| FROM filt_addr port_range
	{
		$$.fo_from = $2;
		$$.fo_from_port_range = $3;
		$$.fo_to = NULL;
		$$.fo_to_port_range = NULL;
	}
	| TO filt_addr port_range
	{
		$$.fo_from = NULL;
		$$.fo_from_port_range = NULL;
		$$.fo_to = $2;
		$$.fo_to_port_range = $3;
	}
	;

filt_addr
	: addr_or_iface		{ $$ = $1; }
	| TABLE_ID		{ $$ = npfctl_parse_table_id($1); }
	| ANY			{ $$ = NULL; }
	;

addr_and_mask
	: addr SLASH NUM
	{
		$$ = npfctl_parse_fam_addr_mask($1, NULL, &$3);
	}
	| addr SLASH HEX
	{
		$$ = npfctl_parse_fam_addr_mask($1, NULL, &$3);
	}
	| addr SLASH addr
	{
		$$ = npfctl_parse_fam_addr_mask($1, $3, NULL);
	}
	| addr
	{
		$$ = npfctl_parse_fam_addr_mask($1, NULL, NULL);
	}
	;

addr_or_iface
	: addr_and_mask	{ assert($1 != NULL); $$ = $1; }
	| iface_name
	{
		$$ = npfctl_parse_iface($1);
	}
	| VAR_ID
	{
		npfvar_t *vp = npfvar_lookup($1);
		const int type = npfvar_get_type(vp, 0);

		switch (type) {
		case NPFVAR_VAR_ID:
		case NPFVAR_STRING:
			$$ = npfctl_parse_iface(npfvar_expand_string(vp));
			break;
		case NPFVAR_FAM:
			$$ = vp;
			break;
		case -1:
			yyerror("undefined variable '%s' for interface", $1);
			break;
		default:
			yyerror("wrong variable '%s' type '%s' or interface",
			    $1, npfvar_type(type));
			$$ = NULL;
			break;
		}
	}
	;

addr
	: IPV4ADDR	{ $$ = $1; }
	| IPV6ADDR	{ $$ = $1; }
	;


port_range
	: PORT port		/* just port */
	{
		$$ = npfctl_parse_port_range($2, $2);
	}
	| PORT port MINUS port	/* port from:to */
	{
		$$ = npfctl_parse_port_range($2, $4);
	}
	| PORT VAR_ID {
		$$ = npfctl_parse_port_range_variable($2);
	}
	|
	{
		$$ = NULL;
	}
	;

port
	: NUM		{ $$ = htons($1); }
	| IDENTIFIER	{ $$ = npfctl_portno($1); }
	;

icmp_type_and_code
	: ICMPTYPE icmp_type
	{
		$$ = npfctl_parse_icmp($2, -1);
	}
	| ICMPTYPE icmp_type CODE NUM
	{
		$$ = npfctl_parse_icmp($2, $4);
	}
	| ICMPTYPE icmp_type CODE IDENTIFIER
	{
		$$ = npfctl_parse_icmp($2, npfctl_icmpcode($2, $4));
	}
	| ICMPTYPE icmp_type CODE VAR_ID
	{
		char *s = npfvar_expand_string(npfvar_lookup($4));
		$$ = npfctl_parse_icmp($2, npfctl_icmpcode($2, s));
	}
	|
	{
		$$ = npfctl_parse_icmp(-1, -1);
	}
	;

tcp_flags_and_mask
	: FLAGS tcp_flags SLASH tcp_flags
	{
		npfvar_add_elements($2, $4);
		$$ = $2;
	}
	| FLAGS tcp_flags
	{
		char *s = npfvar_get_data($2, NPFVAR_TCPFLAG, 0);
		npfvar_add_elements($2, npfctl_parse_tcpflag(s));
		$$ = $2;
	}
	|		{ $$ = NULL; }
	;

tcp_flags
	: IDENTIFIER	{ $$ = npfctl_parse_tcpflag($1); }
	;

icmp_type
	: NUM		{ $$ = $1; }
	| IDENTIFIER	{ $$ = npfctl_icmptype($1); }
	| VAR_ID
	{
		char *s = npfvar_expand_string(npfvar_lookup($1));
		$$ = npfctl_icmptype(s);
	}
	;

ifindex
	: iface_name
	{
		$$ = npfctl_find_ifindex($1);
	}
	| VAR_ID
	{
		npfvar_t *vp = npfvar_lookup($1);
		const int type = npfvar_get_type(vp, 0);

		switch (type) {
		case NPFVAR_VAR_ID:
		case NPFVAR_STRING:
			$$ = npfctl_find_ifindex(npfvar_expand_string(vp));
			break;
		case -1:
			yyerror("undefined variable '%s' for interface", $1);
			break;
		default:
			yyerror("wrong variable '%s' type '%s' for interface",
			    $1, npfvar_type(type));
			$$ = -1;
			break;
		}
	}
	;

iface_name
	: IDENTIFIER	{ $$ = $1; }
	| STRING	{ $$ = $1; }
	;

%%
