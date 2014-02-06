/*	$NetBSD: npf_parse.y,v 1.30 2014/02/06 02:51:28 rmind Exp $	*/

/*-
 * Copyright (c) 2011-2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Martin Husemann, Christos Zoulas and Mindaugas Rasiukevicius.
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

#include "npfctl.h"

#define	YYSTACKSIZE	4096

int			yyparsetarget;
const char *		yyfilename;

extern int		yylineno, yycolumn;
extern int		yylex(void);

void
yyerror(const char *fmt, ...)
{
	extern int yyleng;
	extern char *yytext;

	char *msg, *context = estrndup(yytext, yyleng);
	bool eol = (*context == '\n');
	va_list ap;

	va_start(ap, fmt);
	vasprintf(&msg, fmt, ap);
	va_end(ap);

	fprintf(stderr, "%s:%d:%d: %s", yyfilename,
	    yylineno - (int)eol, yycolumn, msg);
	if (!eol) {
		size_t len = strlen(context);
		char *dst = ecalloc(1, len * 4 + 1);

		strvisx(dst, context, len, VIS_WHITE|VIS_CSTYLE);
		fprintf(stderr, " near '%s'", dst);
	}
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

#define	CHECK_PARSER_FILE				\
	if (yyparsetarget != NPFCTL_PARSE_FILE)		\
		yyerror("rule must be in the group");

#define	CHECK_PARSER_STRING				\
	if (yyparsetarget != NPFCTL_PARSE_STRING)	\
		yyerror("invalid rule syntax");

%}

%token			ALG
%token			ALL
%token			ANY
%token			APPLY
%token			ARROWBOTH
%token			ARROWLEFT
%token			ARROWRIGHT
%token			BLOCK
%token			CDB
%token			CURLY_CLOSE
%token			CURLY_OPEN
%token			CODE
%token			COLON
%token			COMMA
%token			DEFAULT
%token			TDYNAMIC
%token			TSTATIC
%token			EQ
%token			TFILE
%token			FLAGS
%token			FROM
%token			GROUP
%token			HASH
%token			ICMPTYPE
%token			ID
%token			IFNET
%token			IN
%token			INET4
%token			INET6
%token			INTERFACE
%token			MAP
%token			MINUS
%token			NAME
%token			ON
%token			OUT
%token			PAR_CLOSE
%token			PAR_OPEN
%token			PASS
%token			PCAP_FILTER
%token			PORT
%token			PROCEDURE
%token			PROTO
%token			FAMILY
%token			FINAL
%token			FORW
%token			RETURN
%token			RETURNICMP
%token			RETURNRST
%token			RULESET
%token			SEPLINE
%token			SLASH
%token			STATEFUL
%token			TABLE
%token			TCP
%token			TO
%token			TREE
%token			TYPE
%token	<num>		ICMP
%token	<num>		ICMP6

%token	<num>		HEX
%token	<str>		IDENTIFIER
%token	<str>		IPV4ADDR
%token	<str>		IPV6ADDR
%token	<num>		NUM
%token	<fpnum>		FPNUM
%token	<str>		STRING
%token	<str>		TABLE_ID
%token	<str>		VAR_ID

%type	<str>		addr, some_name, table_store
%type	<str>		proc_param_val, opt_apply, ifname, on_ifname, ifref
%type	<num>		port, opt_final, number, afamily, opt_family
%type	<num>		block_or_pass, rule_dir, group_dir, block_opts
%type	<num>		opt_stateful, icmp_type, table_type, map_sd, map_type
%type	<var>		ifaddrs, addr_or_ifaddr, port_range, icmp_type_and_code
%type	<var>		filt_addr, addr_and_mask, tcp_flags, tcp_flags_and_mask
%type	<var>		procs, proc_call, proc_param_list, proc_param
%type	<var>		element, list_elems, list, value
%type	<addrport>	mapseg
%type	<filtopts>	filt_opts, all_or_filt_opts
%type	<optproto>	opt_proto
%type	<rulegroup>	group_opts

%union {
	char *		str;
	unsigned long	num;
	double		fpnum;
	npfvar_t *	var;
	addr_port_t	addrport;
	filt_opts_t	filtopts;
	opt_proto_t	optproto;
	rule_group_t	rulegroup;
}

%%

input
	: { CHECK_PARSER_FILE	} lines
	| { CHECK_PARSER_STRING	} rule
	;

lines
	: line SEPLINE lines
	| line
	;

line
	: vardef
	| table
	| map
	| group
	| rproc
	| alg
	|
	;

alg
	: ALG STRING
	{
		npfctl_build_alg($2);
	}
	;

/*
 * A value - an element or a list of elements.
 * Can be assigned to a variable or used inline.
 */

vardef
	: VAR_ID EQ value
	{
		npfvar_add($3, $1);
	}
	;

value
	: element
	| list
	;

list
	: CURLY_OPEN list_elems CURLY_CLOSE
	{
		$$ = $2;
	}
	;

list_elems
	: element COMMA list_elems
	{
		npfvar_add_elements($1, $3);
	}
	| element
	;

element
	: IDENTIFIER
	{
		$$ = npfvar_create_from_string(NPFVAR_IDENTIFIER, $1);
	}
	| STRING
	{
		$$ = npfvar_create_from_string(NPFVAR_STRING, $1);
	}
	| number MINUS number
	{
		$$ = npfctl_parse_port_range($1, $3);
	}
	| number
	{
		$$ = npfvar_create_element(NPFVAR_NUM, &$1, sizeof($1));
	}
	| VAR_ID
	{
		$$ = npfvar_create_from_string(NPFVAR_VAR_ID, $1);
	}
	| TABLE_ID		{ $$ = npfctl_parse_table_id($1); }
	| ifaddrs		{ $$ = $1; }
	| addr_and_mask		{ $$ = $1; }
	;

/*
 * Table definition.
 */

table
	: TABLE TABLE_ID TYPE table_type table_store
	{
		npfctl_build_table($2, $4, $5);
	}
	;

table_type
	: HASH		{ $$ = NPF_TABLE_HASH; }
	| TREE		{ $$ = NPF_TABLE_TREE; }
	| CDB		{ $$ = NPF_TABLE_CDB; }
	;

table_store
	: TDYNAMIC	{ $$ = NULL; }
	| TFILE STRING	{ $$ = $2; }
	;

/*
 * Map definition.
 */

map_sd
	: TSTATIC	{ $$ = NPFCTL_NAT_STATIC; }
	| TDYNAMIC	{ $$ = NPFCTL_NAT_DYNAMIC; }
	|		{ $$ = NPFCTL_NAT_DYNAMIC; }
	;

map_type
	: ARROWBOTH	{ $$ = NPF_NATIN | NPF_NATOUT; }
	| ARROWLEFT	{ $$ = NPF_NATIN; }
	| ARROWRIGHT	{ $$ = NPF_NATOUT; }
	;

mapseg
	: addr_or_ifaddr port_range
	{
		$$.ap_netaddr = $1;
		$$.ap_portrange = $2;
	}
	;

map
	: MAP ifref map_sd mapseg map_type mapseg PASS filt_opts
	{
		npfctl_build_natseg($3, $5, $2, &$4, &$6, &$8);
	}
	| MAP ifref map_sd mapseg map_type mapseg
	{
		npfctl_build_natseg($3, $5, $2, &$4, &$6, NULL);
	}
	| MAP RULESET group_opts
	{
		npfctl_build_maprset($3.rg_name, $3.rg_attr, $3.rg_ifname);
	}
	;

/*
 * Rule procedure definition and its parameters.
 */

rproc
	: PROCEDURE STRING CURLY_OPEN procs CURLY_CLOSE
	{
		npfctl_build_rproc($2, $4);
	}
	;

procs
	: proc_call SEPLINE procs
	{
		$$ = npfvar_add_elements($1, $3);
	}
	| proc_call	{ $$ = $1; }
	;

proc_call
	: IDENTIFIER COLON proc_param_list
	{
		proc_call_t pc;

		pc.pc_name = estrdup($1);
		pc.pc_opts = $3;

		$$ = npfvar_create_element(NPFVAR_PROC, &pc, sizeof(pc));
	}
	|		{ $$ = NULL; }
	;

proc_param_list
	: proc_param COMMA proc_param_list
	{
		$$ = npfvar_add_elements($1, $3);
	}
	| proc_param	{ $$ = $1; }
	|		{ $$ = NULL; }
	;

proc_param
	: some_name proc_param_val
	{
		proc_param_t pp;

		pp.pp_param = estrdup($1);
		pp.pp_value = $2 ? estrdup($2) : NULL;

		$$ = npfvar_create_element(NPFVAR_PROC_PARAM, &pp, sizeof(pp));
	}
	;

proc_param_val
	: some_name	{ $$ = $1; }
	| number	{ (void)asprintf(&$$, "%ld", $1); }
	| FPNUM		{ (void)asprintf(&$$, "%lf", $1); }
	|		{ $$ = NULL; }
	;

/*
 * Group and dynamic ruleset definition.
 */

group
	: GROUP group_opts
	{
		/* Build a group.  Increase the nesting level. */
		npfctl_build_group($2.rg_name, $2.rg_attr,
		    $2.rg_ifname, $2.rg_default);
	}
	  ruleset_block
	{
		/* Decrease the nesting level. */
		npfctl_build_group_end();
	}
	;

ruleset
	: RULESET group_opts
	{
		/* Ruleset is a dynamic group. */
		npfctl_build_group($2.rg_name, $2.rg_attr | NPF_RULE_DYNAMIC,
		    $2.rg_ifname, $2.rg_default);
		npfctl_build_group_end();
	}
	;

group_dir
	: FORW		{ $$ = NPF_RULE_FORW; }
	| rule_dir
	;

group_opts
	: DEFAULT
	{
		memset(&$$, 0, sizeof(rule_group_t));
		$$.rg_default = true;
	}
	| STRING group_dir on_ifname
	{
		memset(&$$, 0, sizeof(rule_group_t));
		$$.rg_name = $1;
		$$.rg_attr = $2;
		$$.rg_ifname = $3;
	}
	;

ruleset_block
	: CURLY_OPEN ruleset_def CURLY_CLOSE
	;

ruleset_def
	: rule_group SEPLINE ruleset_def
	| rule_group
	;

rule_group
	: rule
	| group
	| ruleset
	|
	;

/*
 * Rule and misc.
 */

rule
	: block_or_pass opt_stateful rule_dir opt_final on_ifname
	  opt_family opt_proto all_or_filt_opts opt_apply
	{
		npfctl_build_rule($1 | $2 | $3 | $4, $5,
		    $6, &$7, &$8, NULL, $9);
	}
	| block_or_pass opt_stateful rule_dir opt_final on_ifname
	  PCAP_FILTER STRING opt_apply
	{
		npfctl_build_rule($1 | $2 | $3 | $4, $5,
		    AF_UNSPEC, NULL, NULL, $7, $8);
	}
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

opt_final
	: FINAL			{ $$ = NPF_RULE_FINAL; }
	|			{ $$ = 0; }
	;

on_ifname
	: ON ifref		{ $$ = $2; }
	|			{ $$ = NULL; }
	;

afamily
	: INET4			{ $$ = AF_INET; }
	| INET6			{ $$ = AF_INET6; }
	;

opt_family
	: FAMILY afamily	{ $$ = $2; }
	|			{ $$ = AF_UNSPEC; }
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
	| PROTO ICMP6 icmp_type_and_code
	{
		$$.op_proto = IPPROTO_ICMPV6;
		$$.op_opts = $3;
	}
	| PROTO some_name
	{
		$$.op_proto = npfctl_protono($2);
		$$.op_opts = NULL;
	}
	| PROTO number
	{
		$$.op_proto = $2;
		$$.op_opts = NULL;
	}
	|
	{
		$$.op_proto = -1;
		$$.op_opts = NULL;
	}
	;

all_or_filt_opts
	: ALL
	{
		$$.fo_from.ap_netaddr = NULL;
		$$.fo_from.ap_portrange = NULL;
		$$.fo_to.ap_netaddr = NULL;
		$$.fo_to.ap_portrange = NULL;
	}
	| filt_opts	{ $$ = $1; }
	;

opt_stateful
	: STATEFUL	{ $$ = NPF_RULE_STATEFUL; }
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
		$$.fo_from.ap_netaddr = $2;
		$$.fo_from.ap_portrange = $3;
		$$.fo_to.ap_netaddr = $5;
		$$.fo_to.ap_portrange = $6;
	}
	| FROM filt_addr port_range
	{
		$$.fo_from.ap_netaddr = $2;
		$$.fo_from.ap_portrange = $3;
		$$.fo_to.ap_netaddr = NULL;
		$$.fo_to.ap_portrange = NULL;
	}
	| TO filt_addr port_range
	{
		$$.fo_from.ap_netaddr = NULL;
		$$.fo_from.ap_portrange = NULL;
		$$.fo_to.ap_netaddr = $2;
		$$.fo_to.ap_portrange = $3;
	}
	;

filt_addr
	: addr_or_ifaddr	{ $$ = $1; }
	| TABLE_ID		{ $$ = npfctl_parse_table_id($1); }
	| ANY			{ $$ = NULL; }
	;

addr_and_mask
	: addr SLASH number
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

addr_or_ifaddr
	: addr_and_mask
	{
		assert($1 != NULL);
		$$ = $1;
	}
	| ifaddrs
	{
		ifnet_addr_t *ifna = npfvar_get_data($1, NPFVAR_INTERFACE, 0);
		$$ = ifna->ifna_addrs;
	}
	| VAR_ID
	{
		npfvar_t *vp = npfvar_lookup($1);
		int type = npfvar_get_type(vp, 0);
		ifnet_addr_t *ifna;

again:
		switch (type) {
		case NPFVAR_IDENTIFIER:
		case NPFVAR_STRING:
			vp = npfctl_parse_ifnet(npfvar_expand_string(vp),
			    AF_UNSPEC);
			type = npfvar_get_type(vp, 0);
			goto again;
		case NPFVAR_FAM:
			$$ = vp;
			break;
		case NPFVAR_INTERFACE:
			ifna = npfvar_get_data(vp, type, 0);
			$$ = ifna->ifna_addrs;
			break;
		case -1:
			yyerror("undefined variable '%s'", $1);
			break;
		default:
			yyerror("wrong variable '%s' type '%s' for address "
			    "or interface", $1, npfvar_type(type));
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
	| PORT port MINUS port	/* port from-to */
	{
		$$ = npfctl_parse_port_range($2, $4);
	}
	| PORT VAR_ID
	{
		$$ = npfctl_parse_port_range_variable($2);
	}
	|
	{
		$$ = NULL;
	}
	;

port
	: number	{ $$ = $1; }
	| IDENTIFIER	{ $$ = npfctl_portno($1); }
	| STRING	{ $$ = npfctl_portno($1); }
	;

icmp_type_and_code
	: ICMPTYPE icmp_type
	{
		$$ = npfctl_parse_icmp($<num>0, $2, -1);
	}
	| ICMPTYPE icmp_type CODE number
	{
		$$ = npfctl_parse_icmp($<num>0, $2, $4);
	}
	| ICMPTYPE icmp_type CODE IDENTIFIER
	{
		$$ = npfctl_parse_icmp($<num>0, $2,
		    npfctl_icmpcode($<num>0, $2, $4));
	}
	| ICMPTYPE icmp_type CODE VAR_ID
	{
		char *s = npfvar_expand_string(npfvar_lookup($4));
		$$ = npfctl_parse_icmp($<num>0, $2,
		    npfctl_icmpcode($<num>0, $2, s));
	}
	|		{ $$ = NULL; }
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
	: number	{ $$ = $1; }
	| IDENTIFIER	{ $$ = npfctl_icmptype($<num>-1, $1); }
	| VAR_ID
	{
		char *s = npfvar_expand_string(npfvar_lookup($1));
		$$ = npfctl_icmptype($<num>-1, s);
	}
	;

ifname
	: some_name
	{
		npfctl_note_interface($1);
		$$ = $1;
	}
	| VAR_ID
	{
		npfvar_t *vp = npfvar_lookup($1);
		const int type = npfvar_get_type(vp, 0);
		ifnet_addr_t *ifna;

		switch (type) {
		case NPFVAR_STRING:
		case NPFVAR_IDENTIFIER:
			$$ = npfvar_expand_string(vp);
			break;
		case NPFVAR_INTERFACE:
			ifna = npfvar_get_data(vp, type, 0);
			$$ = ifna->ifna_name;
			break;
		case -1:
			yyerror("undefined variable '%s' for interface", $1);
			break;
		default:
			yyerror("wrong variable '%s' type '%s' for interface",
			    $1, npfvar_type(type));
			break;
		}
		npfctl_note_interface($$);
	}
	;

ifaddrs
	: afamily PAR_OPEN ifname PAR_CLOSE
	{
		$$ = npfctl_parse_ifnet($3, $1);
	}
	;

ifref
	: ifname
	| ifaddrs
	{
		ifnet_addr_t *ifna = npfvar_get_data($1, NPFVAR_INTERFACE, 0);
		npfctl_note_interface(ifna->ifna_name);
		$$ = ifna->ifna_name;
	}
	;

number
	: HEX		{ $$ = $1; }
	| NUM		{ $$ = $1; }
	;

some_name
	: IDENTIFIER	{ $$ = $1; }
	| STRING	{ $$ = $1; }
	;

%%
