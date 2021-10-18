/* $NetBSD: token_storage_class.c,v 1.1 2021/10/18 22:30:34 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for storage classes such as 'extern', 'static', but not 'typedef'.
 */

#indent input
static int var;
extern int var;
int var;
#indent end

#indent run-equals-input -di0
