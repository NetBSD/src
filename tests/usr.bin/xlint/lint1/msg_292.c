/*	$NetBSD: msg_292.c,v 1.3 2022/02/27 18:51:21 rillig Exp $	*/
# 3 "msg_292.c"

// Test for message: cannot concatenate wide and regular string literals [292]

const char c_c_c_w_w_w[] =
	"c2"
	"c  4"
	"c      8"
	/* expect+1: error: cannot concatenate wide and regular string literals [292] */
	L"w2"
	/* expect+1: error: cannot concatenate wide and regular string literals [292] */
	L"w  4"
	/* expect+1: error: cannot concatenate wide and regular string literals [292] */
	L"w      8";
/* The 15 results from 2 + 4 + 8 + '\0'. */
/* expect+1: error: negative array dimension (-15) [20] */
typedef int reveal_sizeof_c_c_c_w_w_w[-(int)sizeof(c_c_c_w_w_w)];

const char c_w_c_w_c_w[] =
	"c2"
	L"w2"
	/* expect+1: error: cannot concatenate wide and regular string literals [292] */
	"c  4"
	L"w  4"
	/* expect+1: error: cannot concatenate wide and regular string literals [292] */
	"c      8"
	/* expect+1: error: cannot concatenate wide and regular string literals [292] */
	L"w      8";
/*
 * Concatenating L"w2" with "c4" fails, keeping L"w2".
 * Concatenating L"w2" with L"w4" succeeds, resulting in L"w2w4".
 * Concatenating L"w2w4" with "c8" fails, keeping L"w2w4".
 * Concatenating L"w2w4" with L"w8" succeeds, resulting in L"w2w4w8".
 * Concatenating "c2" with L"w2w4w8" fails, keeping "c2".
 * The size of "c2" is 3.
 */
/* expect+1: error: negative array dimension (-3) [20] */
typedef int reveal_sizeof_c_w_c_w_c_w[-(int)sizeof(c_w_c_w_c_w)];
