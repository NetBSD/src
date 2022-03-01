/*	$NetBSD: externs1.h,v 1.149 2022/03/01 00:17:12 rillig Exp $	*/

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
extern	bool	c11flag;
extern	bool	eflag;
extern	bool	Fflag;
extern	bool	gflag;
extern	bool	hflag;
extern	bool	pflag;
extern	bool	rflag;
extern	bool	sflag;
extern	bool	tflag;
extern	bool	uflag;
extern	bool	vflag;
extern	bool	yflag;
extern	bool	wflag;
extern	bool	zflag;
extern	bool	Sflag;
extern	bool	Tflag;
extern	bool	Pflag;

extern	void	norecover(void);

/*
 * cgram.y
 */
extern	int	block_level;
extern	size_t	mem_block_level;
extern	int	yydebug;

extern	int	yyerror(const char *);
extern	int	yyparse(void);

/*
 * scan.l
 */
extern  bool	in_gcc_attribute;
extern	pos_t	curr_pos;
extern	pos_t	csrc_pos;
extern	bool	in_system_header;
extern	symt_t	symtyp;
extern	FILE	*yyin;

extern	void	initscan(void);
extern	int64_t	convert_integer(int64_t, tspec_t, unsigned int);
extern	void	clear_warn_flags(void);
extern	sym_t	*getsym(sbuf_t *);
extern	void	cleanup(void);
extern	sym_t	*pushdown(const sym_t *);
extern	sym_t	*mktempsym(type_t *);
extern	void	rmsym(sym_t *);
extern	void	rmsyms(sym_t *);
extern	void	inssym(int, sym_t *);
extern	void	freeyyv(void *, int);
extern	int	yylex(void);

/*
 * mem1.c
 */
extern	const	char *record_filename(const char *, size_t);
extern	int	get_filename_id(const char *);
extern	void	add_directory_replacement(char *);
extern	const char *transform_filename(const char *, size_t);

extern	void	initmem(void);

extern	void	*block_zero_alloc(size_t);
extern	void	*level_zero_alloc(size_t, size_t);
extern	void	level_free_all(size_t);

extern	void	*expr_zero_alloc(size_t);
extern	tnode_t	*expr_alloc_tnode(void);
extern	void	expr_free_all(void);
extern	struct	memory_block *expr_save_memory(void);
extern	void	expr_restore_memory(struct memory_block *);

/*
 * debug.c
 */

#ifdef DEBUG
const char *scl_name(scl_t);
const char *symt_name(symt_t);
const char *tqual_name(tqual_t);
void	debug_node(const tnode_t *);
void	debug_sym(const sym_t *);
void	debug_symtab(void);
void	debug_printf(const char *fmt, ...) __printflike(1, 2);
void	debug_print_indent(void);
void	debug_indent_inc(void);
void	debug_indent_dec(void);
void	debug_enter(const char *);
void	debug_step(const char *fmt, ...) __printflike(1, 2);
void	debug_leave(const char *);
#define	debug_enter()		(debug_enter)(__func__)
#define	debug_leave()		(debug_leave)(__func__)
#else
#define	debug_noop()		do { } while (false)
#define	debug_node(tn)		debug_noop()
#define	debug_printf(...)	debug_noop()
#define	debug_print_indent()	debug_noop()
#define	debug_indent_inc()	debug_noop()
#define	debug_indent_dec()	debug_noop()
#define	debug_enter()		debug_noop()
#define	debug_step(...)		debug_noop()
#define	debug_leave()		debug_noop()
#endif

/*
 * err.c
 */
extern	int	nerr;
extern	int	sytxerr;
extern	const char *const msgs[];

extern	void	msglist(void);
extern	void	error_at(int, const pos_t *, ...);
extern	void	warning_at(int, const pos_t *, ...);
extern	void	message_at(int, const pos_t *, ...);
extern	void	error(int, ...);
extern	void	warning(int, ...);
extern	void	gnuism(int, ...);
extern	void	c99ism(int, ...);
extern	void	c11ism(int, ...);
extern	void	internal_error(const char *, int, const char *, ...)
     __attribute__((__noreturn__,__format__(__printf__, 3, 4)));
extern	void	assert_failed(const char *, int, const char *, const char *)
		__attribute__((__noreturn__));
extern	void	update_location(const char *, int, bool, bool);

/*
 * decl.c
 */
extern	dinfo_t	*dcs;
extern	const	char *unnamed;
extern	int	enumval;

extern	void	initdecl(void);
extern	type_t	*gettyp(tspec_t);
extern	type_t	*block_dup_type(const type_t *);
extern	type_t	*expr_dup_type(const type_t *);
extern	type_t	*expr_unqualified_type(const type_t *);
extern	bool	is_incomplete(const type_t *);
extern	void	setcomplete(type_t *, bool);
extern	void	add_storage_class(scl_t);
extern	void	add_type(type_t *);
extern	void	add_qualifier(tqual_t);
extern	void	addpacked(void);
extern	void	add_attr_used(void);
extern	void	begin_declaration_level(scl_t);
extern	void	end_declaration_level(void);
extern	void	setasm(void);
extern	void	begin_type(void);
extern	void	end_type(void);
extern	int	length(const type_t *, const char *);
extern	unsigned int alignment_in_bits(const type_t *);
extern	sym_t	*lnklst(sym_t *, sym_t *);
extern	void	check_type(sym_t *);
extern	sym_t	*declarator_1_struct_union(sym_t *);
extern	sym_t	*bitfield(sym_t *, int);
extern	qual_ptr *merge_qualified_pointer(qual_ptr *, qual_ptr *);
extern	sym_t	*add_pointer(sym_t *, qual_ptr *);
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
extern	void	declare(sym_t *, bool, sbuf_t *);
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
extern	int	to_int_constant(tnode_t *, bool);

/*
 * tree.c
 */
extern	const tnode_t *before_conversion(const tnode_t *);
extern	type_t	*block_derive_type(type_t *, tspec_t);
extern	type_t	*expr_derive_type(type_t *, tspec_t);
extern	bool	is_compiler_builtin(const char *);
extern	tnode_t	*build_constant(type_t *, val_t *);
extern	tnode_t	*build_name(sym_t *, bool);
extern	tnode_t	*build_string(strg_t *);
extern	tnode_t	*build_generic_selection(const tnode_t *,
		    struct generic_association *);

extern	tnode_t	*build_binary(tnode_t *, op_t, bool, tnode_t *);
extern	tnode_t	*build_unary(op_t, bool, tnode_t *);
extern	tnode_t	*build_member_access(tnode_t *, op_t, bool, sbuf_t *);
extern	tnode_t	*cconv(tnode_t *);
extern	bool	is_typeok_bool_operand(const tnode_t *);
extern	bool	typeok(op_t, int, const tnode_t *, const tnode_t *);
extern	tnode_t	*promote(op_t, bool, tnode_t *);
extern	tnode_t	*convert(op_t, int, type_t *, tnode_t *);
extern	void	convert_constant(op_t, int, const type_t *, val_t *, val_t *);
extern	tnode_t	*build_sizeof(const type_t *);
extern	tnode_t	*build_offsetof(const type_t *, const sym_t *);
extern	tnode_t	*build_alignof(const type_t *);
extern	tnode_t	*cast(tnode_t *, type_t *);
extern	tnode_t	*build_function_argument(tnode_t *, tnode_t *);
extern	tnode_t	*build_function_call(tnode_t *, bool, tnode_t *);
extern	val_t	*constant(tnode_t *, bool);
extern	void	expr(tnode_t *, bool, bool, bool, bool);
extern	void	check_expr_misc(const tnode_t *, bool, bool, bool,
		    bool, bool, bool);
extern	bool	constant_addr(const tnode_t *, const sym_t **, ptrdiff_t *);
extern	strg_t	*cat_strings(strg_t *, strg_t *);
extern  unsigned int type_size_in_bits(const type_t *);

void begin_statement_expr(void);
void do_statement_expr(tnode_t *);
tnode_t *end_statement_expr(void);

/*
 * func.c
 */
extern	sym_t	*funcsym;
extern	bool	reached;
extern	bool	warn_about_unreachable;
extern	bool	seen_fallthrough;
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

extern	void	begin_control_statement(control_statement_kind);
extern	void	end_control_statement(control_statement_kind);
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
extern	void	do_goto(sym_t *);
extern	void	do_continue(void);
extern	void	do_break(void);
extern	void	do_return(bool, tnode_t *);
extern	void	global_clean_up_decl(bool);
extern	void	argsused(int);
extern	void	constcond(int);
extern	void	fallthru(int);
extern	void	not_reached(int);
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
extern	void	begin_initialization(sym_t *);
extern	void	end_initialization(void);
extern	sym_t	**current_initsym(void);

extern	void	init_rbrace(void);
extern	void	init_lbrace(void);
extern	void	init_expr(tnode_t *);
extern	void	begin_designation(void);
extern	void	add_designator_member(sbuf_t *);
extern	void	add_designator_subscript(range_t);

/*
 * emit.c
 */
extern	void	outtype(const type_t *);
extern	void	outsym(const sym_t *, scl_t, def_t);
extern	void	outfdef(const sym_t *, const pos_t *, bool, bool,
		    const sym_t *);
extern	void	outcall(const tnode_t *, bool, bool);
extern	void	outusg(const sym_t *);

/*
 * lex.c
 */
extern	int	lex_name(const char *, size_t);
extern	int	lex_integer_constant(const char *, size_t, int);
extern	int	lex_floating_constant(const char *, size_t);
extern	int	lex_operator(int, op_t);
extern	int	lex_string(void);
extern	int	lex_wide_string(void);
extern	int	lex_character_constant(void);
extern	int	lex_wide_character_constant(void);
extern	void	lex_directive(const char *);
extern	void	lex_next_line(void);
extern	void	lex_comment(void);
extern	void	lex_slash_slash_comment(void);
extern	void	lex_unknown_character(int);
extern	int	lex_input(void);

/*
 * ckbool.c
 */
extern	bool	typeok_scalar_strict_bool(op_t, const mod_t *, int,
		    const tnode_t *, const tnode_t *);
extern	bool	fallback_symbol_strict_bool(sym_t *);

/*
 * ckctype.c
 */
extern	void	check_ctype_function_call(const tnode_t *, const tnode_t *);
extern	void	check_ctype_macro_invocation(const tnode_t *, const tnode_t *);

/*
 * ckgetopt.c
 */
extern	void	check_getopt_begin_while(const tnode_t *);
extern	void	check_getopt_begin_switch(void);
extern	void	check_getopt_case_label(int64_t);
extern	void	check_getopt_end_switch(void);
extern	void	check_getopt_end_while(void);
