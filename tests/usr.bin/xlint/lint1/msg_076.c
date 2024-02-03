/*	$NetBSD: msg_076.c,v 1.7 2024/02/03 09:36:14 rillig Exp $	*/
# 3 "msg_076.c"

// Test for message: character escape does not fit in character [76]
//
// See also:
//	msg_075.c		for hex escapes

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

char char_string_255[] = "\377";
/* expect+1: warning: character escape does not fit in character [76] */
char char_string_256[] = "\400";
/* expect+1: warning: character escape does not fit in character [76] */
char char_string_511[] = "\777";
char char_string_512[] = "\1000";

int wide_string_255[] = L"\377";
/* FIXME: A wide character can represent 256. */
/* expect+1: warning: character escape does not fit in character [76] */
int wide_string_256[] = L"\400";
/* FIXME: A wide character can represent 511. */
/* expect+1: warning: character escape does not fit in character [76] */
int wide_string_511[] = L"\777";
int wide_string_512[] = L"\1000";
