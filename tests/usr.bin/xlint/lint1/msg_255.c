/*	$NetBSD: msg_255.c,v 1.6 2023/08/11 04:27:49 rillig Exp $	*/
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

/* expect+1: warning: undefined or invalid '#' directive [255] */
# 5 "unfinished

// An empty string means standard input; tabs may be used for spacing.
#	6	""

# 44 "msg_255.c"

int dummy;
