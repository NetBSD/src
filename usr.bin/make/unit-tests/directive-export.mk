# $NetBSD: directive-export.mk,v 1.5 2020/12/12 19:31:18 rillig Exp $
#
# Tests for the .export directive.

# TODO: Implementation

INDIRECT=	indirect
VAR=		value $$ ${INDIRECT}

# Before 2020-12-13, this unusual expression invoked undefined behavior since
# it accessed out-of-bounds memory via Var_Export -> ExportVar -> MayExport.
.export ${:U }

# A variable is exported using the .export directive.
# During that, its value is expanded, just like almost everywhere else.
.export VAR
.if ${:!env | grep '^VAR'!} != "VAR=value \$ indirect"
.  error
.endif

# Undefining a variable that has been exported implicitly removes it from
# the environment of all child processes.
.undef VAR
.if ${:!env | grep '^VAR' || true!} != ""
.  error
.endif

# Tests for parsing the .export directive.
.expor				# misspelled
.export				# oops: missing argument
.export VARNAME
.exporting works		# oops: misspelled

all:
	@:;
