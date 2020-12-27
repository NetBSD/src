# $NetBSD: var-op-expand.mk,v 1.5 2020/12/27 20:45:52 rillig Exp $
#
# Tests for the := variable assignment operator, which expands its
# right-hand side.


# If the right-hand side does not contain a dollar sign, the ':=' assignment
# operator has the same effect as the '=' assignment operator.
VAR:=			value
.if ${VAR} != "value"
.  error
.endif

# When a ':=' assignment is performed, its right-hand side is evaluated and
# expanded as far as possible.  Contrary to other situations, '$$' and
# variable expressions based on undefined variables are preserved though.
#
# Whether a variable expression is undefined or not is determined at the end
# of evaluating the expression.  The consequence is that ${:Ufallback} expands
# to "fallback"; initially this expression is undefined since it is based on
# the variable named "", which is guaranteed to be never defined, but at the
# end of evaluating the expression ${:Ufallback}, the modifier ':U' has turned
# the expression into a defined expression.


# literal dollar signs
VAR:=		$$ $$$$ $$$$$$$$
.if ${VAR} != "\$ \$\$ \$\$\$\$"
.  error
.endif


# reference to a variable containing a literal dollar sign
REF=		$$ $$$$ $$$$$$$$
VAR:=		${REF}
REF=		too late
.if ${VAR} != "\$ \$\$ \$\$\$\$"
.  error
.endif


# reference to an undefined variable
.undef UNDEF
VAR:=		<${UNDEF}>
UNDEF=		after
.if ${VAR} != "<after>"
.  error
.endif


# reference to a variable whose name is computed from another variable
REF2=		referred to
REF=		REF2
VAR:=		${${REF}}
REF=		too late
.if ${VAR} != "referred to"
.  error
.endif


# expression with an indirect modifier referring to an undefined variable
.undef UNDEF
VAR:=		${:${UNDEF}}
UNDEF=		Uwas undefined
.if ${VAR} != "was undefined"
.  error
.endif


# expression with an indirect modifier referring to another variable that
# in turn refers to an undefined variable
#
# XXX: Even though this is a ':=' assignment, the '${UNDEF}' in the part of
# the variable modifier is not preserved.  To preserve it, ParseModifierPart
# would have to call VarSubstExpr somehow since this is the only piece of
# code that takes care of this global variable.
.undef UNDEF
REF=		U${UNDEF}
#.MAKEFLAGS: -dv
VAR:=		${:${REF}}
#.MAKEFLAGS: -d0
REF=		too late
UNDEF=		Uwas undefined
.if ${VAR} != ""
.  error
.endif


# XXX: edge case: When a variable name refers to an undefined variable, the
# behavior differs between the '=' and the ':=' assignment operators.
# This bug exists since var.c 1.42 from 2000-05-11.
#
# The '=' operator expands the undefined variable to an empty string, thus
# assigning to VAR_ASSIGN_.  In the name of variables to be set, it should
# really be forbidden to refer to undefined variables.
#
# The ':=' operator expands the variable name twice.  In one of these
# expansions, the undefined variable expression is preserved (controlled by
# preserveUndefined in VarAssign_EvalSubst), in the other expansion it expands
# to an empty string.  This way, 2 variables are created using a single
# variable assignment.  It's magic. :-/
.undef UNDEF
.MAKEFLAGS: -dv
VAR_ASSIGN_${UNDEF}=	undef value
VAR_SUBST_${UNDEF}:=	undef value
.MAKEFLAGS: -d0

all:
	@:;
