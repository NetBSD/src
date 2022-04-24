/* $NetBSD: lsym_storage_class.c,v 1.4 2022/04/24 10:36:37 rillig Exp $ */

/*
 * Tests for the token lsym_storage_class, which represents a storage class as
 * part of a declaration.
 */

//indent input
static int definition_with_internal_linkage;
extern int declaration_with_external_linkage;
int definition_with_external_linkage;
//indent end

//indent run-equals-input -di0
