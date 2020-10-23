# $NetBSD: varname-make_print_var_on_error.mk,v 1.3 2020/10/23 05:44:02 rillig Exp $
#
# Tests for the special MAKE_PRINT_VAR_ON_ERROR variable, which prints the
# values of selected variables on error.

MAKE_PRINT_VAR_ON_ERROR=	.ERROR_TARGET .ERROR_CMD

all:
	@: command before
	@echo fail; false
	@: command after

# XXX: As of 2020-10-23, the .ERROR_CMD variable seems pointless since at
# that point, the first command in gn->commands has been set to NULL already.
# And because of this, .ERROR_CMD stays an empty list.
