/*	$NetBSD: msg_351.c,v 1.4 2023/04/25 19:00:57 rillig Exp $	*/
# 3 "msg_351.c"

// Test for message 351: missing%s header declaration for '%s' [351]

/*
 * Warn about variable definitions or function definitions that are visible
 * outside the current translation unit but do not have a previous
 * declaration in a header file.
 *
 * All symbols that are used across translation units should be declared in a
 * header file, to ensure consistent types.
 *
 * Since the storage class 'extern' is redundant for functions but not for
 * objects, omit it for functions.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html#index-Wmissing-declarations
 */

/* expect+1: warning: missing header declaration for 'implicitly_extern_function' [351] */
void implicitly_extern_function(void);
/* expect+1: warning: missing header declaration for 'explicitly_extern_function' [351] */
extern void explicitly_extern_function(void);

/* expect+1: warning: missing 'extern' header declaration for 'definition' [351] */
int definition;
/* expect+1: warning: missing 'extern' header declaration for 'reference' [351] */
extern int reference;
/* expect+1: warning: static variable 'file_scoped_definition' unused [226] */
static int file_scoped_definition;


# 18 "header.h" 1 3 4
static int static_def;
int external_def;
extern int external_ref;

static int static_func_def(void);
int extern_func_decl(void);
extern int extern_func_decl_verbose(void);

# 29 "msg_351.c" 2
/* expect+1: warning: static variable 'static_def' unused [226] */
static int static_def;
int external_def;
extern int external_ref;

/* expect+1: warning: static function 'static_func_def' declared but not defined [290] */
static int static_func_def(void);
int extern_func_decl(void);
extern int extern_func_decl_verbose(void);

/* expect+1: warning: missing 'extern' header declaration for 'dbl_ptr' [351] */
double *dbl_ptr = &(double) { 0.0 };
