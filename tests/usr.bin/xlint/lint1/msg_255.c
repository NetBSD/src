/*	$NetBSD: msg_255.c,v 1.3 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_255.c"

// Test for message: undefined or invalid # directive [255]

#pragma once

/* expect+1: warning: undefined or invalid # directive [255] */
#fatal_error

int dummy;
