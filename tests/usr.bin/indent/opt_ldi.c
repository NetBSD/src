/* $NetBSD: opt_ldi.c,v 1.1 2021/10/22 20:54:36 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the option '-ldi', which specifies where the variable names of
 * locally declared variables are placed.
 */

#indent input
int global;

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
