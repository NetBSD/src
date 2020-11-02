# $NetBSD: varname.mk,v 1.7 2020/11/02 22:44:29 rillig Exp $
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
#
# This is not a variable assignment since the parentheses and braces are not
# balanced.  At the end of the line, there are still 3 levels open, which
# means the variable name is not finished.
${:UVAR(((}=	try1
${:UVAR\(\(\(}=	try2
${:UVAR\(\(\(}=	try3

.MAKEFLAGS: -d0

all:
