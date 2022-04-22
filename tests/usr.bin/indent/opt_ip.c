/* $NetBSD: opt_ip.c,v 1.6 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for the options '-ip' and '-nip'.
 *
 * The option '-ip' indents parameter declarations from the left margin, for
 * traditional function definitions.
 *
 * The option '-nip' places the parameter declarations in column 1.
 */

#indent input
double
plus3(a, b, c)
	double a, b, c;
{
	return a + b + c;
}
#indent end

#indent run -ip
double
plus3(a, b, c)
	double		a, b, c;
{
	return a + b + c;
}
#indent end

#indent run -nip
double
plus3(a, b, c)
double		a, b, c;
{
	return a + b + c;
}
#indent end


#indent input
int
first_parameter_in_same_line(int a,
int b,
const char *cp);

int
parameters_in_separate_lines(
int a,
int b,
const char *cp);

int
multiple_parameters_per_line(
int a1, int a2,
int b1, int b2,
const char *cp);
#indent end

#indent run -ip
int
first_parameter_in_same_line(int a,
			     int b,
			     const char *cp);

int
parameters_in_separate_lines(
			     int a,
			     int b,
			     const char *cp);

int
multiple_parameters_per_line(
			     int a1, int a2,
			     int b1, int b2,
			     const char *cp);
#indent end

#indent run-equals-prev-output -nip
