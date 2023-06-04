/* $NetBSD: lsym_storage_class.c,v 1.5 2023/06/04 11:45:00 rillig Exp $ */

/*
 * Tests for the token lsym_modifier (formerly named lsym_storage_class), which
 * represents a type modifier such as 'const', a variable modifier such as a
 * storage class, or a function modifier such as 'inline'.
 */

//indent input
static int definition_with_internal_linkage;
extern int declaration_with_external_linkage;
int definition_with_external_linkage;
//indent end

//indent run-equals-input -di0
