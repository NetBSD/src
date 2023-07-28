/*	$NetBSD: c11.c,v 1.4 2023/07/28 22:05:44 rillig Exp $	*/
# 3 "c11.c"

/*
 * Test the language level C11, which adds _Generic expressions, _Noreturn
 * functions, anonymous struct/union members, and several more.
 */

/* lint1-flags: -Ac11 -w -X 192,231,236,351 */

_Noreturn void exit(int);
void _Noreturn exit(int);

_Noreturn void
noreturn_before_type(void)
{
	exit(0);
}

void _Noreturn
noreturn_after_type(void)
{
	exit(0);
}

static _Noreturn void
noreturn_after_storage_class(void)
{
	exit(0);
}

_Noreturn static void
noreturn_before_storage_class(void)
{
	exit(0);
}

/* C11 6.7.4p5: A function specifier may appear more than once. */
_Noreturn _Noreturn _Noreturn void
three_times(void)
{
	exit(0);
}


_Static_assert(1 > 0, "string");
/* XXX: requires C23 or later */
_Static_assert(1 > 0);


// C11 6.7.6.1p3
const int *ptr_to_constant;
int *const constant_ptr;

// C11 6.7.6.1p4
typedef int *int_ptr;
const int_ptr constant_ptr;

// C11 6.7.6.2p7
float fa[11], *afp[17];

// C11 6.7.6.2p8
extern int *x;
extern int y[];

// C11 6.7.6.2p9
extern int n;
extern int m;
void fcompat(void)
{
	int a[n][6][m];
	int (*p)[4][n+1];
	int c[n][n][6][m];
	int (*r)[n][n][n+1];
	/* expect+2: warning: 'p' set but not used in function 'fcompat' [191] */
	/* expect+1: warning: illegal combination of 'pointer to array[4] of array[1] of int' and 'pointer to array[6] of array[1] of int', op '=' [124] */
	p = a;
	/* expect+2: warning: 'r' set but not used in function 'fcompat' [191] */
	/* expect+1: warning: illegal combination of 'pointer to array[1] of array[1] of array[1] of int' and 'pointer to array[1] of array[6] of array[1] of int', op '=' [124] */
	r = c;
}

// C11 6.7.6.2p10
extern int n;
int A[n];
extern int (*p2)[n];
int B[100];
void fvla(int m, int C[m][m]);
void fvla(int m, int C[m][m])
{
	typedef int VLA[m][m];
	struct tag {
		int (*y)[n];
		int z[n];
	};
	int D[m];
	static int E[m];
	/* expect+1: warning: nested 'extern' declaration of 'F' [352] */
	extern int F[m];
	int (*s)[m];
	/* expect+1: warning: nested 'extern' declaration of 'r' [352] */
	extern int (*r)[m];
	/* expect+2: warning: illegal combination of 'pointer to array[1] of int' and 'pointer to int', op 'init' [124] */
	/* expect+1: warning: 'q' set but not used in function 'fvla' [191] */
	static int (*q)[m] = &B;
}

// C11 6.7.6.3p15
int f(void), *fip(), (*pfi)();

// C11 6.7.6.3p17
int (*apfi[3])(int *x, int *y);

// C11 6.7.6.3p18
int (*fpfi(int (*)(long), int))(int, ...);

// C11 6.7.6.3p19
void addscalar(int n, int m, double a[n][n*m+300], double x);
int main(void)
{
	double b[4][308];
	/* expect+1: warning: converting 'pointer to array[308] of double' to incompatible 'pointer to array[1] of double' for argument 3 [153] */
	addscalar(4, 2, b, 2.17);
	return 0;
}
void addscalar(int n, int m, double a[n][n*m+300], double x)
{
	for (int i = 0; i < n; i++)
		for (int j = 0, k = n*m+300; j < k; j++)
			a[i][j] += x;
}

// C11 6.7.6.3p20
double maximum(int n, int m, double a[n][m]);
/* expect+1: error: null dimension [17] */
double maximum(int n, int m, double a[*][*]);
/* expect+1: error: null dimension [17] */
double maximum(int n, int m, double a[ ][*]);
double maximum(int n, int m, double a[ ][m]);

void f1(double (* restrict a)[5]);
void f2(double a[restrict][5]);
/* expect+1: error: syntax error '3' [249] */
void f3(double a[restrict 3][5]);
void f4(double a[restrict static 3][5]);


// In C11 mode, 'thread_local' is not yet known, but '_Thread_local' is.
/* expect+2: error: old-style declaration; add 'int' [1] */
/* expect+1: error: syntax error 'int' [249] */
thread_local int thread_local_variable_c23;
_Thread_local int thread_local_variable_c11;

/* The '_Noreturn' must not appear after the declarator. */
void _Noreturn exit(int) _Noreturn;
/* expect-1: error: formal parameter #1 lacks name [59] */
/* expect-2: warning: empty declaration [2] */
/* expect+2: error: syntax error '' [249] */
/* expect+1: error: cannot recover from previous errors [224] */
