# $NetBSD: varmod-match-escape.mk,v 1.4 2020/11/01 19:25:23 rillig Exp $
#
# As of 2020-08-01, the :M and :N modifiers interpret backslashes differently,
# depending on whether there was a variable expression somewhere before the
# first backslash or not.  See ApplyModifier_Match, "copy = TRUE".
#
# Apart from the different and possibly confusing debug output, there is no
# difference in behavior.  When parsing the modifier text, only \{, \} and \:
# are unescaped, and in the pattern matching these have the same meaning as
# their plain variants '{', '}' and ':'.  In the pattern matching from
# Str_Match, only \*, \? or \[ would make a noticeable difference.

.MAKEFLAGS: -dcv

SPECIALS=	\: : \\ * \*
.if ${SPECIALS:M${:U}\:} != ${SPECIALS:M\:${:U}}
.  warning unexpected
.endif

# And now both cases combined: A single modifier with both an escaped ':'
# as well as a variable expression that expands to a ':'.
#
# XXX: As of 2020-11-01, when an escaped ':' occurs before the variable
# expression, the whole modifier text is subject to unescaping '\:' to ':',
# before the variable expression is expanded.  This means that the '\:' in
# the variable expression is expanded as well, turning ${:U\:} into a simple
# ${:U:}, which silently expands to an empty string, instead of generating
# an error message.
#
# XXX: As of 2020-11-01, the modifier on the right-hand side of the
# comparison is parsed differently though.  First, the variable expression
# is parsed, resulting in ':' and needSubst=TRUE.  After that, the escaped
# ':' is seen, and this time, copy=TRUE is not executed but stays copy=FALSE.
# Therefore the escaped ':' is kept as-is, and the final pattern becomes
# ':\:'.
#
# If ApplyModifier_Match had used the same parsing algorithm as Var_Subst,
# both patterns would end up as '::'.
#
VALUES=		: :: :\:
.if ${VALUES:M\:${:U\:}} != ${VALUES:M${:U\:}\:}
.  warning XXX: Oops
.endif

.MAKEFLAGS: -d0

all:
	@:;
