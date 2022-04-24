/*	$NetBSD: emit.c,v 1.11 2022/04/24 19:21:01 rillig Exp $	*/
# 3 "emit.c"

/*
 * Test the symbol information that lint1 writes to a .ln file.  Using this
 * symbol information, lint2 later checks that the symbols are used
 * consistently across different translation units.
 */

/* Do not warn about unused parameters. */
/* lint1-extra-flags: -X 231 */

/*
 * Define some derived types.
 */

struct struct_tag {
	int member;
};

typedef struct {
	int member;
} struct_typedef;

union union_tag {
	int member;
};

typedef union {
	int member;
} union_typedef;

enum enum_tag {
	enum_tag_constant
};

typedef enum {
	enum_typedef_constant
} enum_typedef;

/*
 * Variable declarations using the basic types (C99 6.2.5p14).
 *
 * Last synced with function outtype from emit1.c 1.43.
 */

extern _Bool			extern__Bool;
extern float _Complex		extern__Complex_float;
extern double _Complex		extern__Complex_double;
extern long double _Complex	extern__Complex_long_double;
extern char			extern_char;
extern signed char		extern_signed_char;
extern unsigned char		extern_unsigned_char;
extern short			extern_short;
extern signed short		extern_signed_short;
extern unsigned short		extern_unsigned_short;
extern int			extern_int;
extern signed int		extern_signed_int;
extern unsigned int		extern_unsigned_int;
extern long			extern_long;
extern signed long		extern_signed_long;
extern unsigned long		extern_unsigned_long;
extern long long		extern_long_long;
extern signed long long		extern_signed_long_long;
extern unsigned long long	extern_unsigned_long_long;
extern float			extern_float;
extern double			extern_double;
extern long double		extern_long_double;

/*
 * Variable declarations using derived types (C99 6.2.5p20).
 */

extern void *			extern_pointer_to_void;
extern int			extern_array_5_of_int[5];

/*
 * Type tags are written to the .ln file as 'T kind length name', where 'kind'
 * is either 1, 2 or 3.  This is confusing at first since in 'T110struct_tag',
 * the apparent number 110 is to be read as 'tag kind 1, length 10'.
 */
extern struct struct_tag	extern_struct_tag;
extern struct_typedef		extern_struct_typedef;
extern union union_tag		extern_union_tag;
extern union_typedef		extern_union_typedef;
extern enum enum_tag		extern_enum_tag;
extern enum_typedef		extern_enum_typedef;

extern struct {
	int member;
}				extern_anonymous_struct;
extern union {
	int member;
}				extern_anonymous_union;
extern enum {
	anonymous_enum_constant
}				extern_anonymous_enum;

/*
 * Variable definitions.
 *
 * Static variables are not recorded in the .ln file.
 */

extern int			declared_int;
int				defined_int;
/* expect+1: warning: static variable static_int unused [226] */
static int			static_int;

/*
 * Type qualifiers.
 */

extern const int		extern_const_int;
extern volatile int		extern_volatile_int;
extern const volatile int	extern_const_volatile_int;

/*
 * Functions.
 */

extern void return_void_unknown_parameters();
extern /* implicit int */ return_implicit_int_unknown_parameters();
/* expect-1: error: old style declaration; add 'int' [1] */
/* For function declarations, the keyword 'extern' is optional. */
extern void extern_return_void_no_parameters(void);
/* implicit extern */ void return_void_no_parameters(void);
/* expect+1: warning: static function static_return_void_no_parameters declared but not defined [290] */
static void static_return_void_no_parameters(void);

void taking_int(int);
/* The 'const' parameter does not make a difference. */
void taking_const_int(const int);
void taking_int_double_bool(int, double, _Bool);
void taking_struct_union_enum_tags(struct struct_tag, union union_tag,
    enum enum_tag);
void taking_struct_union_enum_typedefs(struct_typedef, union_typedef,
    enum_typedef);

void taking_varargs(const char *, ...);

/*
 * This function does not affect anything outside this translation unit.
 * Naively there is no need to record this function in the .ln file, but it
 * is nevertheless recorded.  There's probably a good reason for recording
 * it.
 */
/* expect+1: warning: static function static_function declared but not defined [290] */
static int static_function(void);

void my_printf(const char *, ...);
void my_scanf(const char *, ...);

/*
 * String literals that occur in function calls are written to the .ln file,
 * just in case they are related to a printf-like or scanf-like function.
 *
 * In this example, the various strings are not format strings, they just
 * serve to cover the code that escapes character literals (outqchar in
 * lint1) and reads them back into characters (inpqstrg in lint2).
 */
void
cover_outqchar(void)
{
	my_printf("%s", "%");
	my_printf("%s", "%s");
	my_printf("%s", "%%");
	my_printf("%s", "%\\ %\" %' %\a %\b %\f %\n %\r %\t %\v %\177");
}

void
cover_outfstrg(void)
{
	my_printf("%s", "%-3d %+3d % d %#x %03d %*.*s %6.2f %hd %ld %Ld %qd");
	my_scanf("%s", "%[0-9]s %[^A-Za-z]s %[][A-Za-z0-9]s %[+-]s");
}

/*
 * Calls to GCC builtin functions should not be emitted since GCC already
 * guarantees a consistent definition of these function and checks the
 * arguments, so there is nothing left to do for lint.
 */
void
call_gcc_builtins(int x, long *ptr)
{
	long value;

	__builtin_expect(x > 0, 1);
	__builtin_bswap32(0x12345678);

	__atomic_load(ptr, &value, 0);
}

/*
 * XXX: It's strange that a function can be annotated with VARARGS even
 * though it does not take varargs at all.
 *
 * This feature is not useful for modern code anyway, it focused on pre-C90
 * code that did not have function prototypes.
 */

/* VARARGS */
void
varargs_comment(const char *fmt)
{
}

/* VARARGS 0 */
void
varargs_0_comment(const char *fmt)
{
}

/* VARARGS 3 */
void
varargs_3_comment(int a, int b, int c, const char *fmt)
{
}

/* PRINTFLIKE */
void
printflike_comment(const char *fmt)
{
}

/* PRINTFLIKE 0 */
void
printflike_0_comment(const char *fmt)
{
}

/* PRINTFLIKE 3 */
void
printflike_3_comment(int a, int b, const char *fmt)
{
}

/* PRINTFLIKE 10 */
void
printflike_10_comment(int a1, int a2, int a3, int a4, int a5,
		      int a6, int a7, int a8, int a9,
		      const char *fmt)
{
}

/* SCANFLIKE */
void
scanflike_comment(const char *fmt)
{
}

/* SCANFLIKE 0 */
void
scanflike_0_comment(const char *fmt)
{
}

/* SCANFLIKE 3 */
void
scanflike_3_comment(int a, int b, const char *fmt)
{
}

int
used_function(void)
{
	return 4;
}

inline int
inline_function(void)
{
	used_function();
	(void)used_function();
	return used_function();
}

extern int declared_used_var;
int defined_used_var;

/*
 * When a function is used, that usage is output as a 'c' record.
 * When a variable is used, that usage is output as a 'u' record.
 */
void
use_vars(void)
{
	declared_used_var++;
	defined_used_var++;
}

/*
 * Since C99, an initializer may contain a compound expression. This allows
 * to create trees of pointer data structures at compile time.
 *
 * The objects that are created for these compound literals are unnamed,
 * therefore there is no point in exporting them to the .ln file.
 *
 * Before emit1.c 1.60 from 2021-11-28, lint exported them.
 */
struct compound_expression_in_initializer {
	const char * const *info;
};

struct compound_expression_in_initializer compound = {
	.info = (const char *[16]){
		[0] = "zero",
	},
};
