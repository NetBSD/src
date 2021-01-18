/*	$NetBSD: externs1.h,v 1.59 2021/01/18 16:41:57 rillig Exp $	*/

/*
 * Copyright (c) 1994, 1995 Jochen Pohl
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Jochen Pohl for
 *	The NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * main.c
 */
extern	int	aflag;
extern	bool	bflag;
extern	bool	cflag;
extern	bool	dflag;
extern	bool	eflag;
extern	bool	Fflag;
extern	bool	gflag;
extern	bool	hflag;
extern	bool	rflag;
extern	bool	sflag;
extern	bool	tflag;
extern	bool	uflag;
extern	bool	vflag;
extern	bool	yflag;
extern	bool	wflag;
extern	bool	zflag;
extern	bool	Sflag;
extern	bool	Pflag;

extern	void	norecover(void);

/*
 * cgram.y
 */
extern	int	blklev;
extern	int	mblklev;
extern	int	yydebug;

extern	int	yyerror(const char *);
extern	int	yyparse(void);

/*
 * scan.l
 */
extern  bool	attron;
extern	pos_t	curr_pos;
extern	pos_t	csrc_pos;
extern	bool	in_system_header;
extern	symt_t	symtyp;
extern	FILE	*yyin;
extern	uint64_t qbmasks[], qlmasks[], qumasks[];

extern	void	initscan(void);
extern	int	sign(int64_t, tspec_t, int);
extern	int	msb(int64_t, tspec_t, int);
extern	int64_t	xsign(int64_t, tspec_t, int);
extern	void	clear_warn_flags(void);
extern	sym_t	*getsym(sbuf_t *);
extern	void	cleanup(void);
extern	sym_t	*pushdown(sym_t *);
extern	sym_t	*mktempsym(type_t *);
extern	void	rmsym(sym_t *);
extern	void	rmsyms(sym_t *);
extern	void	inssym(int, sym_t *);
extern	void	freeyyv(void *, int);
extern	int	yylex(void);

/*
 * mem1.c
 */
extern	const	char *fnalloc(const char *);
extern	const	char *fnnalloc(const char *, size_t);
extern	int	getfnid(const char *);
extern	void	fnaddreplsrcdir(char *);
extern	const char *fnxform(const char *, size_t);

extern	void	initmem(void);

extern	void	*getblk(size_t);
extern	void	*getlblk(size_t, size_t);
extern	void	freeblk(void);
extern	void	freelblk(int);

extern	void	*tgetblk(size_t);
extern	tnode_t	*getnode(void);
extern	void	tfreeblk(void);
extern	struct	mbl *tsave(void);
extern	void	trestor(struct mbl *);

/*
 * err.c
 */
extern	int	nerr;
extern	int	sytxerr;
extern	const	char *msgs[];

extern	void	msglist(void);
extern	void	error(int, ...);
extern	void	warning(int, ...);
extern	void	message(int, ...);
extern	void	gnuism(int, ...);
extern	void	c99ism(int, ...);
extern	void	lerror(const char *, int, const char *, ...)
     __attribute__((__noreturn__,__format__(__printf__, 3, 4)));
extern	void	assert_failed(const char *, int, const char *, const char *)
		__attribute__((__noreturn__));

/*
 * decl.c
 */
extern	dinfo_t	*dcs;
extern	const	char *unnamed;
extern	int	enumval;

extern	void	initdecl(void);
extern	type_t	*gettyp(tspec_t);
extern	type_t	*duptyp(const type_t *);
extern	type_t	*tduptyp(const type_t *);
extern	bool	incompl(const type_t *);
extern	void	setcomplete(type_t *, bool);
extern	void	add_storage_class(scl_t);
extern	void	add_type(type_t *);
extern	void	add_qualifier(tqual_t);
extern	void	addpacked(void);
extern	void	add_attr_used(void);
extern	void	pushdecl(scl_t);
extern	void	popdecl(void);
extern	void	setasm(void);
extern	void	clrtyp(void);
extern	void	deftyp(void);
extern	int	length(const type_t *, const char *);
extern	int	getbound(const type_t *);
extern	sym_t	*lnklst(sym_t *, sym_t *);
extern	void	check_type(sym_t *);
extern	sym_t	*declarator_1_struct_union(sym_t *);
extern	sym_t	*bitfield(sym_t *, int);
extern	pqinf_t	*merge_pointers_and_qualifiers(pqinf_t *, pqinf_t *);
extern	sym_t	*add_pointer(sym_t *, pqinf_t *);
extern	sym_t	*add_array(sym_t *, bool, int);
extern	sym_t	*add_function(sym_t *, sym_t *);
extern	void	check_function_definition(sym_t *, bool);
extern	sym_t	*declarator_name(sym_t *);
extern	sym_t	*old_style_function_name(sym_t *);
extern	type_t	*mktag(sym_t *, tspec_t, bool, bool);
extern	const	char *storage_class_name(scl_t);
extern	type_t	*complete_tag_struct_or_union(type_t *, sym_t *);
extern	type_t	*complete_tag_enum(type_t *, sym_t *);
extern	sym_t	*enumeration_constant(sym_t *, int, bool);
extern	void	decl1ext(sym_t *, bool);
extern	void	copy_usage_info(sym_t *, sym_t *);
extern	bool	check_redeclaration(sym_t *, bool *);
extern	bool	eqptrtype(const type_t *, const type_t *, bool);
extern	bool	eqtype(const type_t *, const type_t *, bool, bool, bool *);
extern	void	complete_type(sym_t *, sym_t *);
extern	sym_t	*declare_argument(sym_t *, bool);
extern	void	check_func_lint_directives(void);
extern	void	check_func_old_style_arguments(void);

extern	void	declare_local(sym_t *, bool);
extern	sym_t	*abstract_name(void);
extern	void	global_clean_up(void);
extern	sym_t	*declare_1_abstract(sym_t *);
extern	void	check_size(sym_t *);
extern	void	mark_as_set(sym_t *);
extern	void	mark_as_used(sym_t *, bool, bool);
extern	void	check_usage(dinfo_t *);
extern	void	check_usage_sym(bool, sym_t *);
extern	void	check_global_symbols(void);
extern	void	print_previous_declaration(int, const sym_t *);

/*
 * tree.c
 */
extern	type_t	*incref(type_t *, tspec_t);
extern	type_t	*tincref(type_t *, tspec_t);
extern	tnode_t	*new_constant_node(type_t *, val_t *);
extern	tnode_t	*new_name_node(sym_t *, int);
extern	tnode_t	*new_string_node(strg_t *);
extern	sym_t	*struct_or_union_member(tnode_t *, op_t, sym_t *);
extern	tnode_t	*build(op_t, tnode_t *, tnode_t *);
extern	tnode_t	*cconv(tnode_t *);
extern	bool	is_strict_bool(const tnode_t *);
extern	bool	typeok(op_t, int, const tnode_t *, const tnode_t *);
extern	tnode_t	*promote(op_t, bool, tnode_t *);
extern	tnode_t	*convert(op_t, int, type_t *, tnode_t *);
extern	void	convert_constant(op_t, int, type_t *, val_t *, val_t *);
extern	tnode_t	*build_sizeof(type_t *);
extern	tnode_t	*build_offsetof(type_t *, sym_t *);
extern	tnode_t	*build_alignof(type_t *);
extern	tnode_t	*cast(tnode_t *, type_t *);
extern	tnode_t	*new_function_argument_node(tnode_t *, tnode_t *);
extern	tnode_t	*new_function_call_node(tnode_t *, tnode_t *);
extern	val_t	*constant(tnode_t *, bool);
extern	void	expr(tnode_t *, bool, bool, bool);
extern	void	check_expr_misc(const tnode_t *, bool, bool, bool,
		    bool, bool, bool);
extern	bool	constant_addr(tnode_t *, sym_t **, ptrdiff_t *);
extern	strg_t	*cat_strings(strg_t *, strg_t *);
extern  int64_t tsize(type_t *);

/*
 * func.c
 */
extern	sym_t	*funcsym;
extern	bool	reached;
extern	bool	rchflg;
extern	bool	ftflg;
extern	int	nargusg;
extern	pos_t	argsused_pos;
extern	int	nvararg;
extern	pos_t	vapos;
extern	int	printflike_argnum;
extern	pos_t	printflike_pos;
extern	int	scanflike_argnum;
extern	pos_t	scanflike_pos;
extern	bool	constcond_flag;
extern	bool	llibflg;
extern	int	lwarn;
extern	bool	bitfieldtype_ok;
extern	bool	plibflg;
extern	bool	quadflg;

extern	void	pushctrl(int);
extern	void	popctrl(int);
extern	void	check_statement_reachable(void);
extern	void	funcdef(sym_t *);
extern	void	funcend(void);
extern	void	named_label(sym_t *);
extern	void	case_label(tnode_t *);
extern	void	default_label(void);
extern	void	if1(tnode_t *);
extern	void	if2(void);
extern	void	if3(bool);
extern	void	switch1(tnode_t *);
extern	void	switch2(void);
extern	void	while1(tnode_t *);
extern	void	while2(void);
extern	void	do1(void);
extern	void	do2(tnode_t *);
extern	void	for1(tnode_t *, tnode_t *, tnode_t *);
extern	void	for2(void);
extern	void	dogoto(sym_t *);
extern	void	docont(void);
extern	void	dobreak(void);
extern	void	doreturn(tnode_t *);
extern	void	global_clean_up_decl(bool);
extern	void	argsused(int);
extern	void	constcond(int);
extern	void	fallthru(int);
extern	void	notreach(int);
extern	void	lintlib(int);
extern	void	linted(int);
extern	void	varargs(int);
extern	void	printflike(int);
extern	void	scanflike(int);
extern	void	protolib(int);
extern	void	longlong(int);
extern	void	bitfieldtype(int);

/*
 * init.c
 */
extern	bool	initerr;
extern	sym_t	*initsym;
extern	int	startinit;

extern	void	initstack_init(void);
extern	void	init_rbrace(void);
extern	void	init_lbrace(void);
extern	void	mkinit(tnode_t *);
extern	void	push_member(sbuf_t *);

/*
 * emit.c
 */
extern	void	outtype(const type_t *);
extern	const	char *ttos(const type_t *);
extern	void	outsym(const sym_t *, scl_t, def_t);
extern	void	outfdef(const sym_t *, const pos_t *, bool, bool,
		    const sym_t *);
extern	void	outcall(const tnode_t *, bool, bool);
extern	void	outusg(const sym_t *);

/*
 * print.c
 */
extern	char	*print_tnode(char *, size_t, const tnode_t *);
