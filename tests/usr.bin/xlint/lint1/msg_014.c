/*	$NetBSD: msg_014.c,v 1.8.2.1 2025/08/02 05:58:15 perseant Exp $	*/
# 3 "msg_014.c"

// Test for message: compiler takes alignment of function [14]
/* This message is not used. */

/* lint1-extra-flags: -X 351 */

typedef void function(void);

/* expect+1: error: cannot take size/alignment of function type 'function(void) returning void' [144] */
unsigned long alignof_function = __alignof__(function);

struct invalid_bit_field {
	/* expect+1: warning: invalid bit-field type 'function(void) returning void' [35] */
	function bit_field:1;
	/* expect+1: error: function invalid in structure or union [38] */
	function member;
};

struct s {
	/* expect+1: error: array of function is invalid [16] */
	function member[5];
};
