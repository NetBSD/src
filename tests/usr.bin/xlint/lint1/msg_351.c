/*	$NetBSD: msg_351.c,v 1.7 2023/07/07 19:45:22 rillig Exp $	*/
# 3 "msg_351.c"

// Test for message 351: missing%s header declaration for '%s' [351]

/*
 * Warn about declarations or definitions for functions or objects that are
 * visible outside the current translation unit but do not have a previous
 * declaration in a header file.
 *
 * All symbols that are used across translation units should be declared in a
 * header file, to ensure consistent types.
 *
 * Since the storage class 'extern' is redundant for functions but not for
 * objects, the diagnostic omits it for functions.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html#index-Wmissing-declarations
 */

/* expect+1: warning: missing header declaration for 'func_decl' [351] */
void func_decl(void);
/* expect+1: warning: missing header declaration for 'extern_func_decl' [351] */
extern void extern_func_decl(void);
static int static_func_decl(void);

/* expect+3: warning: missing header declaration for 'func_def' [351] */
void
func_def(void)
{
}

/* expect+3: warning: missing header declaration for 'extern_func_def' [351] */
extern void
extern_func_def(void)
{
}

/* expect+2: warning: static function 'static_func_def' unused [236] */
static void
static_func_def(void)
{
}

/* expect+1: warning: missing 'extern' header declaration for 'obj_decl' [351] */
extern int obj_decl;
/* expect+1: warning: missing 'extern' header declaration for 'obj_def' [351] */
int obj_def;
static int static_obj_def;


# 18 "header.h" 1 3 4

void func_decl(void);
extern void extern_func_decl(void);
static int static_func_decl(void);

void func_def(void);
extern void extern_func_def(void);
static void static_func_def(void);

void func_def_ok(void);
extern void extern_func_def_ok(void);
static void static_func_def_ok(void);

extern int obj_decl;
int obj_def;
static int static_obj_def;

# 70 "msg_351.c" 2

void func_decl(void);
extern void extern_func_decl(void);
/* expect+1: warning: static function 'static_func_decl' declared but not defined [290] */
static int static_func_decl(void);

void
func_def_ok(void)
{
}

extern void
extern_func_def_ok(void)
{
}

/* expect+2: warning: static function 'static_func_def_ok' unused [236] */
static void
static_func_def_ok(void)
{
}

extern int obj_decl;
int obj_def;
/* expect+1: warning: static variable 'static_obj_def' unused [226] */
static int static_obj_def;


/*
 * Do not warn about the temporary identifier generated for the object from the
 * compound literal.
 */
/* expect+1: warning: missing 'extern' header declaration for 'dbl_ptr' [351] */
double *dbl_ptr = &(double) { 0.0 };
