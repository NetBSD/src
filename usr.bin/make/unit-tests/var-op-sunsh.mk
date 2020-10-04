# $NetBSD: var-op-sunsh.mk,v 1.1 2020/10/04 06:53:15 rillig Exp $
#
# Tests for the :sh= variable assignment operator, which runs its right-hand
# side through the shell.  It is a seldom-used alternative to the !=
# assignment operator.

.MAKEFLAGS: -dL			# Enable sane error messages

# This is the idiomatic form of the Sun shell assignment operator.
# The assignment operator is directly preceded by the ':sh'.
VAR:sh=		echo colon-sh
.if ${VAR} != "colon-sh"
.  error
.endif

# XXX: As of 2020-10-04, the ':sh' can even be followed by other characters.
# This is neither documented by NetBSD make nor by Solaris make.
VAR:shell=	echo colon-shell
.if ${VAR} != "colon-shell"
.  error
.endif

# XXX: Several colons can syntactically appear in a variable name.
# Neither of these should be interpreted as the ':sh' assignment operator
# modifier.
VAR:shoe:shore=	echo two-colons
.if ${VAR${:U\:}shoe} != "two-colons"
.  error
.endif

#.MAKEFLAGS: -dcpv

# XXX: As of 2020-10-04, the following expression is wrongly marked as
# a parse error.  This is caused by the ':sh' modifier.
#
# There are two different syntactical elements that look exactly the same:
# The variable modifier ':sh' and the assignment operator modifier ':sh'.
# Intuitively this variable name contains the variable modifier, but the
# parser sees it as operator modifier, in Parse_DoVar.
#
VAR.${:Uecho 123:sh}=	echo oops
.if ${VAR.echo 123} != "oops"
.  error
.endif

# XXX: Same pattern here. The ':sh' inside the nested expression is taken
# for the assignment operator, even though it is escaped by a backslash.
#
VAR.${:U echo\:shell}=	echo oops
.if ${VAR.${:U echo\\}} != "oops"
.  error
.endif

# XXX: The word 'shift' is also affected since it starts with ':sh'.
#
VAR.key:shift=		echo Shift
.if ${VAR.key} != "Shift"
.  error
.endif

.MAKEFLAGS: -d0

# XXX: Despite the error messages the exit status is still 0.

all:
	@:;
