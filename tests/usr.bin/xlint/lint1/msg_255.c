/*	$NetBSD: msg_255.c,v 1.7 2024/12/08 17:12:01 rillig Exp $	*/
# 3 "msg_255.c"

// Test for message: undefined or invalid '#' directive [255]

/* lint1-extra-flags: -X 351 */

/* expect+1: warning: undefined or invalid '#' directive [255] */
#

/* expect+1: warning: undefined or invalid '#' directive [255] */
#pragma

#pragma once

/* expect+1: warning: undefined or invalid '#' directive [255] */
#fatal_error

/* expect+1: warning: undefined or invalid '#' directive [255] */
#    ident "obsolete"

/* expect+1: warning: undefined or invalid '#' directive [255] */
#1

// Sets the line number of the current file.
# 2

// Switch back to the main file.
# 30 "msg_255.c"

/* expect+1: warning: undefined or invalid '#' directive [255] */
# 3/

/* expect+1: warning: undefined or invalid '#' directive [255] */
# 4 /

/* expect+3: error: newline in string or char constant [254] */
/* expect+2: error: unterminated string constant [258] */
# 5 "unfinished

// An empty string means standard input; tabs may be used for spacing.
#	6	""

# 45 "msg_255.c"

int dummy;
