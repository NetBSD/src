/*	$NetBSD: msg_068.c,v 1.6 2024/11/30 11:27:20 rillig Exp $	*/
# 3 "msg_068.c"

// Test for message: typedef already qualified with '%s' [68]

/* lint1-extra-flags: -X 351 */

typedef const char const_char;

/* expect+1: warning: typedef already qualified with 'const' [68] */
const const_char twice_const;

typedef volatile char volatile_char;

/* expect+1: warning: typedef already qualified with 'volatile' [68] */
volatile volatile_char twice_volatile;

typedef const volatile char const_volatile_char;

/* expect+2: warning: typedef already qualified with 'const' [68] */
/* expect+1: warning: typedef already qualified with 'volatile' [68] */
const volatile const_volatile_char twice_const_volatile;
