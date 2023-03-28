/*	$NetBSD: msg_255.c,v 1.4 2023/03/28 14:44:35 rillig Exp $	*/
# 3 "msg_255.c"

// Test for message: undefined or invalid # directive [255]

/* lint1-extra-flags: -X 351 */

#pragma once

/* expect+1: warning: undefined or invalid # directive [255] */
#fatal_error

int dummy;
