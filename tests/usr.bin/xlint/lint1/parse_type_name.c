/*	$NetBSD: parse_type_name.c,v 1.8 2022/04/01 23:16:32 rillig Exp $	*/
# 3 "parse_type_name.c"

/*
 * Test parsing of the grammar rule 'type_name', which among others appears
 * in the expression 'sizeof(type_name)'.
 */

void sink(unsigned long);

void
cover_type_name(void)
{
	/* cover 'abstract_declaration' */
	sink(sizeof(int));
}

void
cover_abstract_declaration(void)
{
	/* cover 'qualifier_list' */
	/* missing type-specifier, even in traditional C */
	/* lint doesn't care since this is caught by the compiler */
	sink(sizeof(const));

	/* cover 'specifier_qualifier_list' */
	sink(sizeof(double));

	/* cover 'qualifier_list abstract_declarator' */
	/* XXX: This is nonsense, lint should not accept it. */
	sink(sizeof(const[3]));

	/* cover 'specifier_qualifier_list abstract_declarator' */
	sink(sizeof(const int[3]));
	sink(sizeof(int const[3]));
}

void
cover_abstract_declarator(void)
{
	/* cover 'pointer' */
	sink(sizeof(int ***));

	/* cover 'direct_abstract_declarator' */
	sink(sizeof(int[3]));

	/* cover 'pointer direct_abstract_declarator' */
	sink(sizeof(int **[3]));

	/* cover 'T_TYPEOF cast_expression' */
	/* expect+1: error: cannot take size/alignment of function type 'function(int) returning int' [144] */
	sink(sizeof(int(typeof(12345))));
}

void
cover_direct_abstract_declarator(void)
{
	/* cover 'T_LPAREN abstract_declarator T_RPAREN' */
	sink(sizeof(int (*)));

	/* cover 'T_LBRACK T_RBRACK' */
	sink(sizeof(int[]));

	/* cover 'T_LBRACK array_size T_RBRACK' */
	sink(sizeof(int[3]));

	/* cover 'type_attribute direct_abstract_declarator' */
	sink(sizeof(int *__attribute__(())[3]));

	/* cover 'direct_abstract_declarator T_LBRACK T_RBRACK' */
	/* expect+1: error: null dimension [17] */
	sink(sizeof(int[3][]));

	/* cover 'direct_abstract_declarator T_LBRACK T_ASTERISK T_RBRACK' */
	/* expect+1: error: null dimension [17] */
	sink(sizeof(int[3][ *]));

	/* cover 'direct_abstract_declarator T_LBRACK array_size T_RBRACK' */
	sink(sizeof(int[3][5][8]));

	/* cover 'abstract_decl_param_list asm_or_symbolrename_opt' */
	sink(sizeof(int(double)));
	sink(sizeof(int(double) __asm("anything")));
	sink(sizeof(int(double) __symbolrename(alias)));

	/* cover 'direct_abstract_declarator abstract_decl_param_list asm_or_symbolrename_opt' */
	sink(sizeof(int (*)(double)));
	sink(sizeof(int (*)(double) __asm("anything")));
	sink(sizeof(int (*)(double)__symbolrename(alias)));

	/* cover 'direct_abstract_declarator type_attribute_list' */
	sink(sizeof(int (*) __attribute__(())));
	sink(sizeof(int (*) __attribute__(()) __attribute__(())));
}

void
cover_abstract_decl_param_list(void)
{
	/* cover 'abstract_decl_lparen T_RPAREN type_attribute_opt' */
	sink(sizeof(void (*)()));
	sink(sizeof(void (*)() __attribute__(())));
	/*
	 * XXX: The grammar allows only a single type_attribute_opt.
	 * All following __attribute__ come from direct_abstract_declarator.
	 */
	sink(sizeof(void (*)() __attribute__(()) __attribute__(())));

	/* cover 'abstract_decl_lparen vararg_parameter_type_list T_RPAREN type_attribute_opt' */
	sink(sizeof(void (*)(void) __attribute__(())));
	/*
	 * XXX: The grammar allows only a single type_attribute_opt.
	 * All following __attribute__ come from direct_abstract_declarator.
	 */
	sink(sizeof(void (*)(void) __attribute__(()) __attribute__(())));

	/* cover 'abstract_decl_lparen error T_RPAREN type_attribute_opt' */
	/* expect+1: syntax error 'goto' [249] */
	sink(sizeof(void (*)(goto)));
	/* expect+1: syntax error 'goto' [249] */
	sink(sizeof(void (*)(goto) __attribute__(())));
	/*
	 * XXX: The grammar allows only a single type_attribute_opt.
	 * All following __attribute__ come from direct_abstract_declarator.
	 */
	/* expect+1: syntax error 'goto' [249] */
	sink(sizeof(void (*)(goto) __attribute__(()) __attribute__(())));
}

void
cover_vararg_parameter_type_list(void)
{
	/* cover 'parameter_type_list' */
	sink(sizeof(void (*)(double)));

	/* cover 'parameter_type_list T_COMMA T_ELLIPSIS' */
	sink(sizeof(void (*)(double, ...)));

	/* cover 'T_ELLIPSIS' */
	/* expect+1: warning: ANSI C requires formal parameter before '...' [84] */
	sink(sizeof(void (*)(...)));
}

void
cover_parameter_type_list(void)
{
	/* cover 'parameter_declaration' */
	sink(sizeof(void (*)(double)));

	/* cover 'parameter_type_list T_COMMA parameter_declaration' */
	sink(sizeof(void (*)(double, double, double, char *)));
}

void
cover_parameter_declaration(void)
{
	/* cover 'declmods' */
	/* GCC 11 warns: type defaults to 'int' in type name */
	sink(sizeof(void (*)(int, const)));

	/* cover 'declaration_specifiers' */
	sink(sizeof(void (*)(int, double)));

	/* cover 'declmods notype_param_declarator' */
	/* GCC 11 warns: type defaults to 'int' in declaration of 'x' */
	sink(sizeof(void (*)(int, const x)));

	/* cover 'begin_type_declaration_specifiers end_type type_param_declarator' */
	sink(sizeof(void (*)(int, double x)));

	/* cover 'begin_type_declmods end_type abstract_declarator' */
	/* GCC 11 warns: type defaults to 'int' in type name */
	sink(sizeof(void (*)(int, const *)));

	/* cover 'begin_type_declaration_specifiers end_type abstract_declarator' */
	sink(sizeof(void (*)(int, double *)));
}
