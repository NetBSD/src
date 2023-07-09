/*	$NetBSD: decl_arg.c,v 1.10 2023/07/09 11:18:55 rillig Exp $	*/
# 3 "decl_arg.c"

/*
 * Tests for declarations of function arguments.
 *
 * See arg_declaration in cgram.y.
 */

/* lint1-extra-flags: -X 351 */

typedef double number;

void no_args(void);
void type_unnamed(double );
void type_named(double arg);
void typedef_unnamed_prototype(number);
void typedef_named(number arg);

void type_qualifier(const number);
void type_qualifier_pointer(const number *const);

/*
 * Just some unrealistic coverage for the grammar rule 'arg_declaration'.
 */
/* expect+6: warning: parameter 'an_int' unused in function 'old_style' [231] */
/* expect+5: warning: parameter 'a_const_int' unused in function 'old_style' [231] */
/* expect+4: warning: parameter 'a_number' unused in function 'old_style' [231] */
/* expect+3: warning: parameter 'a_function' unused in function 'old_style' [231] */
/* expect+2: warning: parameter 'a_struct' unused in function 'old_style' [231] */
extern void
old_style(an_int, a_const_int, a_number, a_function, a_struct)
/* expect+2: warning: empty declaration [2] */
/* expect+1: error: only 'register' is valid as storage class in parameter [9] */
static;
/* expect+1: error: syntax error '"' [249] */
static "error";
/* expect+1: warning: empty declaration [2] */
const;
/* expect+1: error: declared argument 'undeclared' is missing [53] */
const undeclared;
/* expect+2: error: declared argument 'undeclared_initialized' is missing [53] */
/* expect+1: error: cannot initialize parameter 'undeclared_initialized' [52] */
const undeclared_initialized = 12345;
/* expect+1: warning: empty declaration [2] */
int;
/* expect+1: warning: 'struct arg_struct' declared in argument declaration list [3] */
struct arg_struct { int member; };
/* expect+1: error: cannot initialize parameter 'an_int' [52] */
int an_int = 12345;
const int a_const_int;
number a_number;
void (a_function) (number);
/* expect+1: warning: 'struct a_struct' declared in argument declaration list [3] */
struct a_struct { int member; } a_struct;
{
}

/*
 * Just some unrealistic coverage for the grammar rule
 * 'notype_direct_declarator'.
 */
extern int
cover_notype_direct_decl(arg)
int arg;
/* expect+1: error: declared argument 'name' is missing [53] */
const name;
/* expect+1: error: declared argument 'parenthesized_name' is missing [53] */
const (parenthesized_name);
/* expect+1: error: declared argument 'array' is missing [53] */
const array[];
/* expect+1: error: declared argument 'array_size' is missing [53] */
const array_size[1+1+1];
/* expect+2: error: declared argument 'multi_array' is missing [53] */
/* expect+1: error: null dimension [17] */
const multi_array[][][][][][];
/* expect+1: error: declared argument 'function' is missing [53] */
const function(void);
/* expect+1: error: declared argument 'prefix_attribute' is missing [53] */
const __attribute__((deprecated)) prefix_attribute;
/* expect+1: error: declared argument 'postfix_attribute' is missing [53] */
const postfix_attribute __attribute__((deprecated));
/* expect+1: error: declared argument 'infix_attribute' is missing [53] */
const __attribute__((deprecated)) infix_attribute __attribute__((deprecated));
/* The __attribute__ before the '*' is consumed by some other grammar rule. */
/* expect+7: error: declared argument 'pointer_prefix_attribute' is missing [53] */
const
    __attribute__((deprecated))
    *
    __attribute__((deprecated))
    __attribute__((deprecated))
    __attribute__((deprecated))
    pointer_prefix_attribute;
{
	return arg;
}

void test_varargs_attribute(
    void (*pr)(const char *, ...)
	__attribute__((__format__(__printf__, 1, 2)))
);

/*
 * XXX: To cover the grammar rule 'direct_notype_param_decl', the parameters
 *  need to be enclosed by one more pair of parentheses than usual.
 */
void cover_direct_notype_param_decl(
    double (identifier),
    double ((parenthesized)),
    double (array[]),
    double (array_size[3]),
    double (*)(void (function()))
);

/*
 * Just some unrealistic code to cover the grammar rule parameter_declaration.
 */
/* expect+4: error: only 'register' is valid as storage class in parameter [9] */
void cover_parameter_declaration(
    volatile,			/* 1 */
    double,			/* 2 */
    static storage_class,	/* 3.1 */
    const type_qualifier,	/* 3.2 */
    double (identifier),	/* 4 */
    const (*),			/* 5 */
    double *const,		/* 6 */
    ...
);

void cover_asm_or_symbolrename_asm(void)
    __asm("assembly code");

void cover_asm_or_symbolrename_symbolrename(void)
    __symbolrename(alternate_name);

// FIXME: internal error in decl.c:906 near decl_arg.c:134: length(0)
//void cover_abstract_declarator_typeof(void (*)(typeof(no_args)));
