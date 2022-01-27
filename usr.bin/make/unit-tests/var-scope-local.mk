# $NetBSD: var-scope-local.mk,v 1.2 2022/01/27 06:56:27 sjg Exp $
#
# Tests for target-local variables, such as ${.TARGET} or $@.

# TODO: Implementation

# Ensure that the name of the variable is exactly the given one.
# The variable "@" is an alias for ".TARGET", so the implementation might
# canonicalize these aliases at some point, and that might be surprising.
# This aliasing happens for single-character variable names like $@ or $<
# (see VarFind, CanonicalVarname), but not for braced or parenthesized
# expressions like ${@}, ${.TARGET} ${VAR:Mpattern} (see Var_Parse,
# ParseVarname).
.if ${@:L} != "@"
.  error
.endif
.if ${.TARGET:L} != ".TARGET"
.  error
.endif
.if ${@F:L} != "@F"
.  error
.endif
.if ${@D:L} != "@D"
.  error
.endif

all:

.SUFFIXES: .c .o

var-scope-local.c var-scope-local2.c var-scope-local3.c:
	: Making ${.TARGET} out of nothing.

.c.o:
	: Making ${.TARGET} from ${.IMPSRC}.

	# The local variables @F, @D, <F, <D are legacy forms.
	# See the manual page for details.
	: Making basename "${@F}" in "${@D}" from "${<F}" in "${<D}" VAR="${VAR}".

all: var-scope-local.o var-scope-local2.o var-scope-local3.o
	# The ::= modifier overwrites the .TARGET variable in the node
	# 'all', not in the global scope.  This can be seen with the -dv
	# option, looking for "all:@ = overwritten".
	: ${.TARGET} ${.TARGET::=overwritten}${.TARGET} VAR="${VAR}"

# we can set variables per target with some limitations
.MAKE.TARGET_LOCAL_VARIABLES= yes
VAR= global
# the rest of the line is the value
var-scope-local.o: VAR= local
# += will *not* take global value and add to it in local context
var-scope-local2.o: VAR+= local
# but once defined in local context += behaves as expected.
var-scope-local2.o: VAR += to ${.TARGET}
# we can get the global value though
# so complex values can always be set via global variable and then
# assigned to local context
# Note: that the global ${VAR} is expanded at this point
# just as with any dependency line.
var-scope-local3.o: VAR= ${VAR}+local

# while VAR=use will be set for a .USE node, it will never be seen
# since only the ultimate target's context is searched
a_use: .USE VAR=use
	: ${.TARGET} uses .USE VAR="${VAR}"

var-scope-local3.o: a_use
