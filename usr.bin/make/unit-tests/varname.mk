# $NetBSD: varname.mk,v 1.6 2020/11/02 22:29:48 rillig Exp $
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
${:UVAR(((}=	try1
${:UVAR\(\(\(}=	try2
${:UVAR\(\(\(}=	try3

.MAKEFLAGS: -d0

all:
