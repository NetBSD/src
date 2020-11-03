# $NetBSD: directive.mk,v 1.3 2020/11/03 17:17:31 rillig Exp $
#
# Tests for the preprocessing directives, such as .if or .info.

# TODO: Implementation

# Unknown directives are correctly named in the error messages,
# even if they are indented.
.indented none
.  indented 2 spaces
.	indented tab

# Directives must be written directly, not indirectly via variable
# expressions.
.${:Uinfo} directives cannot be indirect

all:
	@:;
