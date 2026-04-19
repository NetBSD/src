/*	$NetBSD: msg_089.c,v 1.6 2026/04/19 16:34:11 rillig Exp $	*/
# 3 "msg_089.c"

// Test for message: typedef '%s' redeclared [89]

/* lint1-extra-flags: -r */

/* expect+1: previous definition of 'number' [261] */
typedef int number;
/* expect+2: error: typedef 'number' redeclared [89] */
/* expect+1: previous definition of 'number' [261] */
typedef int number;
/* expect+1: error: typedef 'number' redeclared [89] */
typedef double number;


// An error cannot be suppressed, only a warning can.
// So both the error message and the previous declaration are reported.

/* expect+1: previous definition of 'no_previous_definition' [261] */
typedef int no_previous_definition;
/* LINTED 89 */
/* expect+1: error: typedef 'no_previous_definition' redeclared [89] */
typedef int no_previous_definition;
