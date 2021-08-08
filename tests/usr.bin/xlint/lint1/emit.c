/*	$NetBSD: emit.c,v 1.2 2021/08/08 11:07:19 rillig Exp $	*/
# 3 "emit.c"

/*
 * Test the symbol information that lint1 writes to a .ln file.  Using this
 * symbol information, lint2 later checks that the symbols are used
 * consistently across different translation units.
 */

// omit the option '-g' to avoid having the GCC builtins in the .ln file.
/* lint1-flags: -Sw */

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
extern float _Complex 		extern__Complex_float;
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
static int			static_int;		/* expect: unused */

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

/* For function declarations, the keyword 'extern' is optional. */
extern void extern_return_void_no_parameters(void);
/* implicit extern */ void return_void_no_parameters(void);
static void static_return_void_no_parameters(void);	/* expect: declared */

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
static int static_function(void);			/* expect: declared */

void my_printf(const char *, ...);

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
	my_printf("%s", "%\a %\b %\f %\n %\r %\t %\v %\177");
}
