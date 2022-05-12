/*	$NetBSD: msg_014.c,v 1.7 2022/05/12 00:18:35 rillig Exp $	*/
# 3 "msg_014.c"

// Test for message: compiler takes alignment of function [14]
/* This message is not used. */

typedef void function(void);

/* expect+1: error: cannot take size/alignment of function type 'function(void) returning void' [144] */
unsigned long alignof_function = __alignof__(function);

struct illegal_bit_field {
	/* expect+1: warning: illegal bit-field type 'function(void) returning void' [35] */
	function bit_field:1;
	/* expect+1: error: function illegal in structure or union [38] */
	function member;
};

struct s {
	/* expect+1: error: array of function is illegal [16] */
	function member[5];
};
