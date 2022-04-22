/* $NetBSD: token_storage_class.c,v 1.2 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for storage classes such as 'extern', 'static', but not 'typedef'.
 */

#indent input
static int var;
extern int var;
int var;
#indent end

#indent run-equals-input -di0
