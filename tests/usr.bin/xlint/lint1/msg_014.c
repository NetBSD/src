/*	$NetBSD: msg_014.c,v 1.6 2022/05/12 00:09:44 rillig Exp $	*/
# 3 "msg_014.c"

// Test for message: compiler takes alignment of function [14]
/* This message is not used. */

typedef void function(void);

/* FIXME: '__alignof__' must be recognized. */
/* expect+2: error: function '__alignof__' implicitly declared to return int [215] */
/* expect+1: error: syntax error 'function' [249] */
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
