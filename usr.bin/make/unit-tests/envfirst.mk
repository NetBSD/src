# $NetBSD: envfirst.mk,v 1.1 2020/07/25 21:19:29 rillig Exp $
#
# The -e option makes environment variables stronger than global variables.

.if ${FROM_ENV} != value-from-env
.error ${FROM_ENV}
.endif

# Try to override the variable; this does not have any effect.
FROM_ENV=	value-from-mk
.if ${FROM_ENV} != value-from-env
.error ${FROM_ENV}
.endif

# Try to append to the variable; this also doesn't have any effect.
FROM_ENV+=	appended
.if ${FROM_ENV} != value-from-env
.error ${FROM_ENV}
.endif

# The default assignment also cannot change the variable.
FROM_ENV?=	default
.if ${FROM_ENV} != value-from-env
.error ${FROM_ENV}
.endif

# Neither can the assignment modifiers.
.if ${FROM_ENV::=from-condition}
.endif
.if ${FROM_ENV} != value-from-env
.error ${FROM_ENV}
.endif

all:
	@: nothing
