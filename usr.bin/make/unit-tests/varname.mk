# $NetBSD: varname.mk,v 1.5 2020/11/02 22:16:24 rillig Exp $
#
# Tests for special variables, such as .MAKE or .PARSEDIR.
# And for variable names in general.

.MAKEFLAGS: -dv

# In variable names, braces are allowed, but they must be balanced.
# Parentheses and braces may be mixed.
VAR{{{}}}=	3 braces
.if "${VAR{{{}}}}" != "3 braces"
.  error
.endif

# In variable expressions, the parser works differently.  It doesn't treat
# braces and parentheses equally, therefore the first closing brace already
# marks the end of the variable name.
VARNAME=	VAR(((
${VARNAME}=	3 open parentheses
.if "${VAR(((}}}}" != "3 open parentheses}}}"
.  error
.endif

# In the above test, the variable name is constructed indirectly.  Neither
# of the following expressions produces the intended effect.
# TODO: explain what happens in the parser here.
${VAR(((}=	try1
${VAR\(\(\(}=	try2
${VAR\(\(\(}=	try3

.MAKEFLAGS: -d0

all:
