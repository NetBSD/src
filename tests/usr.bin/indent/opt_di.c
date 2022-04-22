/* $NetBSD: opt_di.c,v 1.7 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Test the option '-di', which specifies the indentation of the first
 * variable name in a declaration.
 */

#indent input
int space;
int	tab;
int		tab16;

struct long_name long_name;
#indent end

#indent run -di8
int	space;
int	tab;
int	tab16;

struct long_name long_name;
#indent end


/*
 * The declarator can be a simple variable name. It can also be prefixed by
 * asterisks, for pointer variables. These asterisks are placed to the left of
 * the indentation line, so that the variable names are aligned.
 *
 * There can be multiple declarators in a single declaration, separated by
 * commas. Only the first of them is aligned to the indentation given by
 * '-di', the others are separated with a single space.
 */
#indent input
int var;
int *ptr, *****ptr;
#indent end

#indent run -di12
int	    var;
int	   *ptr, *****ptr;
#indent end


/*
 * Test the various values for indenting.
 */
#indent input
int decl ;
#indent end

/*
 * An indentation of 0 columns uses a single space between the declaration
 * specifiers (in this case 'int') and the declarator.
 */
#indent run -di0
int decl;
#indent end

/*
 * An indentation of 7 columns uses spaces for indentation since in the
 * default configuration, the next tab stop would be at indentation 8.
 */
#indent run -di7
int    decl;
#indent end

/* The indentation consists of a single tab. */
#indent run -di8
int	decl;
#indent end

/* The indentation consists of a tab and a space. */
#indent run -di9
int	 decl;
#indent end

#indent run -di16
int		decl;
#indent end


/*
 * Ensure that all whitespace is normalized to be indented by 8 columns,
 * which in the default configuration amounts to a single tab.
 */
#indent input
int space;
int	tab;
int		tab16;
struct long_name long_name;
#indent end

#indent run -di8
int	space;
int	tab;
int	tab16;
struct long_name long_name;
#indent end


/*
 * A variable that has an ad-hoc struct/union/enum type does not need to be
 * indented to the right of the keyword 'struct', it only needs a single space
 * of indentation.
 *
 * Before NetBSD indent.c 1.151 from 2021-10-24, the indentation depended on
 * the length of the keyword 'struct', 'union' or 'enum', together with type
 * qualifiers like 'const' or the storage class like 'static'.
 */
#indent input
struct {
	int member;
} var = {
	3,
};
#indent end

#indent run-equals-input -di0

#indent run
struct {
	int		member;
}		var = {
	3,
};
#indent end
