/*	$NetBSD: msg_076.c,v 1.6 2024/02/02 19:07:58 rillig Exp $	*/
# 3 "msg_076.c"

// Test for message: character escape does not fit in character [76]

/* lint1-extra-flags: -X 351 */

char char_255 = '\377';
/* expect+1: warning: character escape does not fit in character [76] */
char char_256 = '\400';
/* expect+1: warning: character escape does not fit in character [76] */
char char_511 = '\777';
/* expect+2: warning: multi-character character constant [294] */
/* expect+1: warning: initializer does not fit [178] */
char char_512 = '\1000';

int wide_255 = L'\377';
/* expect+1: warning: character escape does not fit in character [76] */
int wide_256 = L'\400';
/* expect+1: warning: character escape does not fit in character [76] */
int wide_511 = L'\777';
/* expect+1: error: too many characters in character constant [71] */
int wide_512 = L'\1000';
