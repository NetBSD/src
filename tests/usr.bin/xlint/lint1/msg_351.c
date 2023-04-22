/*	$NetBSD: msg_351.c,v 1.2 2023/04/22 20:21:13 rillig Exp $	*/
# 3 "msg_351.c"

// Test for message 351: 'extern' declaration of '%s' outside a header [351]

/* expect+1: warning: 'extern' declaration of 'implicitly_extern_function' outside a header [351] */
void implicitly_extern_function(void);
/* expect+1: warning: 'extern' declaration of 'explicitly_extern_function' outside a header [351] */
extern void explicitly_extern_function(void);

/* expect+1: warning: 'extern' declaration of 'definition' outside a header [351] */
int definition;
/* expect+1: warning: 'extern' declaration of 'reference' outside a header [351] */
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

/* FIXME: Don't warn about the identifier starting with '00000'. */
/* expect+2: warning: 'extern' declaration of 'dbl_ptr' outside a header [351] */
/* expect+1: warning: 'extern' declaration of '00000000_tmp' outside a header [351] */
double *dbl_ptr = &(double) { 0.0 };
