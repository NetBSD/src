/*	$NetBSD: externs1.h,v 1.221 2024/03/29 08:35:32 rillig Exp $	*/

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
 *	This product includes software developed by Jochen Pohl for
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

#include <signal.h>

/*
 * main1.c
 */
extern bool Fflag;
extern bool Pflag;
extern bool Tflag;
extern int aflag;
extern bool bflag;
extern bool cflag;
extern bool eflag;
extern bool hflag;
extern bool pflag;
extern bool rflag;
extern bool uflag;
extern bool vflag;
extern bool wflag;
extern bool yflag;
extern bool zflag;

extern bool allow_trad;
extern bool allow_c90;
extern bool allow_c99;
extern bool allow_c11;
extern bool allow_c23;
extern bool allow_gcc;

extern sig_atomic_t fpe;

void norecover(void);

/*
 * cgram.y
 */
extern int block_level;
extern size_t mem_block_level;
extern int yydebug;

int yyerror(const char *);
int yyparse(void);

/*
 * lex.c
 */
extern bool in_gcc_attribute;
extern pos_t curr_pos;
extern pos_t csrc_pos;
extern bool in_system_header;
extern symbol_kind sym_kind;
extern FILE *yyin;

void init_lex(void);
int64_t convert_integer(int64_t, tspec_t, unsigned int);
void reset_suppressions(void);
sym_t *getsym(sbuf_t *);
void clean_up_after_error(void);
sym_t *pushdown(const sym_t *);
sym_t *mktempsym(type_t *);
void symtab_remove_forever(sym_t *);
void symtab_remove_level(sym_t *);
void inssym(int, sym_t *);
void freeyyv(void *, int);
int yylex(void);

/*
 * mem1.c
 */
const char *record_filename(const char *, size_t);
int get_filename_id(const char *);
void add_directory_replacement(char *);
const char *transform_filename(const char *, size_t);

#ifdef DEBUG_MEM
void *block_zero_alloc(size_t, const char *);
void *level_zero_alloc(size_t, size_t, const char *);
#else
void *block_zero_alloc(size_t);
void *level_zero_alloc(size_t, size_t);
#define block_zero_alloc(size, descr) (block_zero_alloc)(size)
#define level_zero_alloc(level, size, descr) (level_zero_alloc)(level, size)
#endif
void level_free_all(size_t);

#ifdef DEBUG_MEM
void *expr_zero_alloc(size_t, const char *);
#else
void *expr_zero_alloc(size_t);
#define expr_zero_alloc(size, descr) (expr_zero_alloc)(size)
#endif
tnode_t *expr_alloc_tnode(void);
void expr_free_all(void);
memory_pool expr_save_memory(void);
void expr_restore_memory(memory_pool);

/*
 * debug.c
 */

#ifdef DEBUG
const char *decl_level_kind_name(decl_level_kind);
const char *scl_name(scl_t);
const char *symbol_kind_name(symbol_kind);
const char *type_qualifiers_string(type_qualifiers);
const char *function_specifier_name(function_specifier);
void debug_dcs(void);
void debug_dcs_all(void);
void debug_node(const tnode_t *);
void debug_type(const type_t *);
void debug_sym(const char *, const sym_t *, const char *);
void debug_symtab(void);
void debug_printf(const char *fmt, ...) __printflike(1, 2);
void debug_skip_indent(void);
void debug_indent_inc(void);
void debug_indent_dec(void);
bool debug_push_indented(bool);
void debug_pop_indented(bool);
void debug_enter_func(const char *);
void debug_step(const char *fmt, ...) __printflike(1, 2);
void debug_leave_func(const char *);
#define	debug_enter()		debug_enter_func(__func__)
#define	debug_leave()		debug_leave_func(__func__)
#else
#define	debug_noop()		do { } while (false)
#define	debug_dcs()		debug_noop()
#define	debug_dcs_all()		debug_noop()
#define	debug_sym(p, sym, s)	debug_noop()
#define	debug_symtab()		debug_noop()
#define	debug_node(tn)		debug_noop()
#define	debug_type(tp)		debug_noop()
#define	debug_printf(...)	debug_noop()
#define	debug_skip_indent()	debug_noop()
#define	debug_indent_inc()	debug_noop()
#define	debug_indent_dec()	debug_noop()
#define debug_push_indented(c)	true
#define debug_pop_indented(c)	(void)(c)
#define	debug_enter()		debug_noop()
#define	debug_step(...)		debug_noop()
#define	debug_leave()		debug_noop()
#endif

/*
 * err.c
 */
extern bool seen_error;
extern bool seen_warning;
extern int sytxerr;
extern bool any_query_enabled;
extern bool is_query_enabled[];

void msglist(void);
void error_at(int, const pos_t *, ...);
void warning_at(int, const pos_t *, ...);
void message_at(int, const pos_t *, ...);
void error(int, ...);
void warning(int, ...);
bool gnuism(int, ...);
void c99ism(int, ...);
void c11ism(int, ...);
void c23ism(int, ...);
void assert_failed(const char *, int, const char *, const char *) __dead;
void update_location(const char *, int, bool, bool);
void suppress_messages(const char *);

void query_message(int, ...);
void enable_queries(const char *);

/*
 * decl.c
 */
extern decl_level *dcs;
extern const char unnamed[];
extern int enumval;

void init_decl(void);
type_t *gettyp(tspec_t);
type_t *block_dup_type(const type_t *);
type_t *expr_dup_type(const type_t *);
type_t *expr_unqualified_type(const type_t *);
bool is_incomplete(const type_t *);
void dcs_add_function_specifier(function_specifier);
void dcs_add_storage_class(scl_t);
void dcs_add_type(type_t *);
void dcs_add_qualifiers(type_qualifiers);
void dcs_add_packed(void);
void dcs_set_used(void);
void begin_declaration_level(decl_level_kind);
void end_declaration_level(void);
void dcs_set_asm(void);
void dcs_begin_type(void);
void dcs_end_type(void);
int length_in_bits(const type_t *, const char *);
unsigned int alignment_in_bits(const type_t *);
sym_t *concat_symbols(sym_t *, sym_t *);
void check_type(sym_t *);
sym_t *declare_unnamed_member(void);
sym_t *declare_member(sym_t *);
sym_t *set_bit_field_width(sym_t *, int);
void add_type_qualifiers(type_qualifiers *, type_qualifiers);
qual_ptr *append_qualified_pointer(qual_ptr *, qual_ptr *);
sym_t *add_pointer(sym_t *, qual_ptr *);
sym_t *add_array(sym_t *, bool, int);
sym_t *add_function(sym_t *, parameter_list);
void check_extern_declaration(const sym_t *);
void check_function_definition(sym_t *, bool);
sym_t *declarator_name(sym_t *);
sym_t *old_style_function_parameter_name(sym_t *);
type_t *make_tag_type(sym_t *, tspec_t, bool, bool);
type_t *complete_struct_or_union(sym_t *);
type_t *complete_enum(sym_t *);
sym_t *enumeration_constant(sym_t *, int, bool);
void declare(sym_t *, bool, sbuf_t *);
void copy_usage_info(sym_t *, sym_t *);
bool check_redeclaration(sym_t *, bool *);
bool pointer_types_are_compatible(const type_t *, const type_t *, bool);
bool types_compatible(const type_t *, const type_t *, bool, bool, bool *);
void complete_type(sym_t *, sym_t *);
sym_t *declare_parameter(sym_t *, bool);
void check_func_lint_directives(void);
void check_func_old_style_parameters(void);

void declare_local(sym_t *, bool);
sym_t *abstract_name(void);
sym_t *abstract_enclosing_name(void);
void global_clean_up(void);
sym_t *declare_abstract_type(sym_t *);
void check_size(const sym_t *);
void mark_as_set(sym_t *);
void mark_as_used(sym_t *, bool, bool);
void check_usage(const decl_level *);
void check_usage_sym(bool, const sym_t *);
void end_translation_unit(void);
void print_previous_declaration(const sym_t *);
int to_int_constant(tnode_t *, bool);

/*
 * tree.c
 */
const tnode_t *before_conversion(const tnode_t *);
type_t *block_derive_type(type_t *, tspec_t);
type_t *expr_derive_type(type_t *, tspec_t);
bool is_compiler_builtin(const char *);
tnode_t *build_constant(type_t *, val_t *);
tnode_t *build_name(sym_t *, bool);
tnode_t *build_string(buffer *);
tnode_t *build_generic_selection(const tnode_t *,
		    struct generic_association *);

tnode_t *build_binary(tnode_t *, op_t, bool, tnode_t *);
tnode_t *build_unary(op_t, bool, tnode_t *);
tnode_t *build_member_access(tnode_t *, op_t, bool, sbuf_t *);
tnode_t *cconv(tnode_t *);
bool is_typeok_bool_compares_with_zero(const tnode_t *);
bool typeok(op_t, int, const tnode_t *, const tnode_t *);
tnode_t *promote(op_t, bool, tnode_t *);
tnode_t *convert(op_t, int, type_t *, tnode_t *);
void convert_constant(op_t, int, const type_t *, val_t *, val_t *);
tnode_t *build_sizeof(const type_t *);
tnode_t *build_offsetof(const type_t *, designation);
tnode_t *build_alignof(const type_t *);
tnode_t *cast(tnode_t *, bool, type_t *);
void add_function_argument(function_call *, tnode_t *);
tnode_t *build_function_call(tnode_t *, bool, function_call *);
val_t *integer_constant(tnode_t *, bool);
void expr(tnode_t *, bool, bool, bool, bool);
void check_expr_misc(const tnode_t *, bool, bool, bool, bool, bool, bool);
bool constant_addr(const tnode_t *, const sym_t **, ptrdiff_t *);
buffer *cat_strings(buffer *, buffer *);
unsigned int type_size_in_bits(const type_t *);
sym_t *find_member(const struct_or_union *, const char *);
uint64_t possible_bits(const tnode_t *);

void begin_statement_expr(void);
void do_statement_expr(tnode_t *);
tnode_t *end_statement_expr(void);
bool in_statement_expr(void);

/*
 * func.c
 */
extern sym_t *funcsym;
extern bool reached;
extern bool warn_about_unreachable;
extern bool suppress_fallthrough;
extern int nargusg;
extern pos_t argsused_pos;
extern int nvararg;
extern pos_t vapos;
extern int printflike_argnum;
extern pos_t printflike_pos;
extern int scanflike_argnum;
extern pos_t scanflike_pos;
extern bool suppress_constcond;
extern bool llibflg;
extern int lwarn;
extern bool suppress_bitfieldtype;
extern bool plibflg;
extern bool suppress_longlong;

void begin_control_statement(control_statement_kind);
void end_control_statement(control_statement_kind);
void check_statement_reachable(void);
void begin_function(sym_t *);
void end_function(void);
void named_label(sym_t *);
void case_label(tnode_t *);
void default_label(void);
void stmt_if_expr(tnode_t *);
void stmt_if_then_stmt(void);
void stmt_if_else_stmt(bool);
void stmt_switch_expr(tnode_t *);
void stmt_switch_expr_stmt(void);
void stmt_while_expr(tnode_t *);
void stmt_while_expr_stmt(void);
void stmt_do(void);
void stmt_do_while_expr(tnode_t *);
void stmt_for_exprs(tnode_t *, tnode_t *, tnode_t *);
void stmt_for_exprs_stmt(void);
void stmt_goto(sym_t *);
void stmt_continue(void);
void stmt_break(void);
void stmt_return(bool, tnode_t *);
void global_clean_up_decl(bool);
void handle_lint_comment(lint_comment, int);

/*
 * init.c
 */
void begin_initialization(sym_t *);
void end_initialization(void);
sym_t *current_initsym(void);

void init_rbrace(void);
void init_lbrace(void);
void init_expr(tnode_t *);
void begin_designation(void);
void add_designator_member(sbuf_t *);
void add_designator_subscript(range_t);
void designation_push(designation *, designator_kind, const sym_t *, size_t);

/*
 * emit.c
 */
void outtype(const type_t *);
void outsym(const sym_t *, scl_t, def_t);
void outfdef(const sym_t *, const pos_t *, bool, bool, const sym_t *);
void outcall(const tnode_t *, bool, bool);
void outusg(const sym_t *);

/*
 * lex.c
 */
int lex_name(const char *, size_t);
int lex_integer_constant(const char *, size_t, int);
int lex_floating_constant(const char *, size_t);
int lex_operator(int, op_t);
int lex_string(void);
int lex_wide_string(void);
int lex_character_constant(void);
int lex_wide_character_constant(void);
void lex_directive(const char *);
void lex_next_line(void);
void lex_comment(void);
void lex_slash_slash_comment(void);
void lex_unknown_character(int);
int lex_input(void);
bool quoted_next(const buffer *, quoted_iterator *);

/*
 * ckbool.c
 */
bool typeok_scalar_strict_bool(op_t, const mod_t *, int,
		    const tnode_t *, const tnode_t *);
bool fallback_symbol_strict_bool(sym_t *);

/*
 * ckctype.c
 */
void check_ctype_function_call(const function_call *);
void check_ctype_macro_invocation(const tnode_t *, const tnode_t *);

/*
 * ckgetopt.c
 */
void check_getopt_begin_while(const tnode_t *);
void check_getopt_begin_switch(void);
void check_getopt_case_label(int64_t);
void check_getopt_end_switch(void);
void check_getopt_end_while(void);

/* cksnprintb.c */
void check_snprintb(const tnode_t *);
