/* $NetBSD: lsym_storage_class.c,v 1.6 2023/06/25 19:29:57 rillig Exp $ */

/*
 * Tests for the token lsym_modifier (formerly named lsym_storage_class), which
 * represents a type modifier such as 'const', a variable modifier such as a
 * storage class, or a function modifier such as 'inline'.
 */

//indent input
static int	definition_with_internal_linkage;
extern int	declaration_with_external_linkage;
int		definition_with_external_linkage;
_Complex double	cd;
_Imaginary double id;
complex double	cd;
imaginary double id;
// The token after a modifier (in this case 'dc') is always interpreted as a
// type name, therefore it is not indented by 16 but by a single space.
double complex dc;
//indent end

//indent run-equals-input
