/*	$NetBSD: msg_005.c,v 1.5 2022/06/17 18:54:53 rillig Exp $	*/
# 3 "msg_005.c"

// Test for message: modifying typedef with '%s'; only qualifiers allowed [5]

typedef int number;

/* expect+1: warning: modifying typedef with 'signed'; only qualifiers allowed [5] */
typedef number signed signed_number;

/* expect+1: warning: modifying typedef with 'unsigned'; only qualifiers allowed [5] */
typedef number unsigned unsigned_number;

/* expect+1: warning: modifying typedef with 'short'; only qualifiers allowed [5] */
typedef number short short_number;

/* expect+1: warning: modifying typedef with 'long'; only qualifiers allowed [5] */
typedef number long long_number;

/*
 * If the type qualifier comes first, the following name is interpreted as a
 * new name, not as the one referring to the typedef.  This makes the above
 * type modifications even more obscure.
 */
/* expect+1: error: syntax error 'prefix_long_number' [249] */
typedef long number prefix_long_number;

/* Type qualifiers are OK. */
typedef number const const_number;
