/* $NetBSD: token_storage_class.c,v 1.3 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for storage classes such as 'extern', 'static', but not 'typedef'.
 */

//indent input
static int var;
extern int var;
int var;
//indent end

//indent run-equals-input -di0
