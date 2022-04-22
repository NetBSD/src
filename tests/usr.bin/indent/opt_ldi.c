/* $NetBSD: opt_ldi.c,v 1.4 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for the option '-ldi', which specifies where the variable names of
 * locally declared variables are placed.
 *
 * See also:
 *	opt_di.c
 */

#indent input
int global;

void
function(void)
{
	int local;
}
#indent end

#indent run -ldi0
int		global;

void
function(void)
{
	int local;
}
#indent end

#indent run -ldi8
int		global;

void
function(void)
{
	int	local;
}
#indent end

#indent run -ldi24
int		global;

void
function(void)
{
	int			local;
}
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
{
	struct {
		int member;
	} var = {
		3,
	};
}
#indent end

/*
 * Struct members use '-di' for indentation, no matter whether they are
 * declared globally or locally.
 */
#indent run -ldi0
{
	struct {
		int		member;
	} var = {
		3,
	};
}
#indent end

#indent run -ldi16
{
	struct {
		int		member;
	}		var = {
		3,
	};
}
#indent end
