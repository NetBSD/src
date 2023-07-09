/*	$NetBSD: msg_255.c,v 1.5 2023/07/09 11:01:27 rillig Exp $	*/
# 3 "msg_255.c"

// Test for message: undefined or invalid '#' directive [255]

/* lint1-extra-flags: -X 351 */

#pragma once

/* expect+1: warning: undefined or invalid '#' directive [255] */
#fatal_error

int dummy;
