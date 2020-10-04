# $NetBSD: var-op-sunsh.mk,v 1.2 2020/10/04 07:49:45 rillig Exp $
#
# Tests for the :sh= variable assignment operator, which runs its right-hand
# side through the shell.  It is a seldom-used alternative to the !=
# assignment operator, adopted from Sun make.

.MAKEFLAGS: -dL			# Enable sane error messages

# This is the idiomatic form of the Sun shell assignment operator.
# The assignment operator is directly preceded by the ':sh'.
VAR:sh=		echo colon-sh
.if ${VAR} != "colon-sh"
.  error
.endif

# It is also possible to have whitespace around the :sh assignment
# operator modifier.
VAR :sh =	echo colon-sh-spaced
.if ${VAR} != "colon-sh-spaced"
.  error
.endif

# Until 2020-10-04, the ':sh' could even be followed by other characters.
# This was neither documented by NetBSD make nor by Solaris make and was
# an implementation error.
#
# Since 2020-10-04, this is a normal variable assignment using the '='
# assignment operator.
VAR:shell=	echo colon-shell
.if ${${:UVAR\:shell}} != "echo colon-shell"
.  error
.endif

# Several colons can syntactically appear in a variable name.
# Until 2020-10-04, the last of them was interpreted as the ':sh'
# assignment operator.
#
# Since 2020-10-04, the colons are part of the variable name.
VAR:shoe:shore=	echo two-colons
.if ${${:UVAR\:shoe\:shore}} != "echo two-colons"
.  error
.endif

# Until 2020-10-04, the following expression was wrongly marked as
# a parse error.  This was because the parser for variable assignments
# just looked for the previous ":sh", without taking any contextual
# information into account.
#
# There are two different syntactical elements that look exactly the same:
# The variable modifier ':sh' and the assignment operator modifier ':sh'.
# Intuitively this variable name contains the variable modifier, but until
# 2020-10-04, the parser regarded it as an assignment operator modifier, in
# Parse_DoVar.
VAR.${:Uecho 123:sh}=	ok-123
.if ${VAR.123} != "ok-123"
.  error
.endif

# Same pattern here. Until 2020-10-04, the ':sh' inside the nested expression
# was taken for the :sh assignment operator modifier, even though it was
# escaped by a backslash.
VAR.${:U echo\:shell}=	ok-shell
.if ${VAR.${:U echo\:shell}} != "ok-shell"
.  error
.endif

# Until 2020-10-04, the word 'shift' was also affected since it starts with
# ':sh'.
VAR.key:shift=		Shift
.if ${${:UVAR.key\:shift}} != "Shift"
.  error
.endif

all:
	@:;
