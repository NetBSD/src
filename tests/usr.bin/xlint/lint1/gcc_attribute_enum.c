/*	$NetBSD: gcc_attribute_enum.c,v 1.1 2021/07/06 17:33:07 rillig Exp $	*/
# 3 "gcc_attribute_enum.c"

/*
 * Tests for the GCC __attribute__ for enumerators.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Enumerator-Attributes.html
 */

enum Planet {
	Mercury,
	Venus,
	Earth,
	Mars,
	Jupiter,
	Saturn,
	Uranus,
	Neptune,
	/* https://en.wikipedia.org/wiki/Pluto_(planet) */
	/*FIXME*//* expect+1: error: syntax error '__attribute__' [249] */
	Pluto __attribute__((__deprecated__ /* since August 2006 */))
};
