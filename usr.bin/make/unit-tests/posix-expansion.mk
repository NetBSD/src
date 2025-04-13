# $NetBSD: posix-expansion.mk,v 1.1 2025/04/13 09:29:33 rillig Exp $
#
# https://pubs.opengroup.org/onlinepubs/9799919799/utilities/make.html#tag_20_76_13_05
#
# In POSIX mode, when expanding an expression containing modifiers, the
# modifiers specified in POSIX take precedence over the BSD-style modifiers.

.POSIX:


MOD_SUBST=	S s from to
# The modifier contains a "=" and is thus the POSIX modifier.
.if ${MOD_SUBST:S=from=to=} != "from=to= s from to"
# FIXME: POSIX mode must be active here.
# expect+1: Missing argument for ".error"
.  error
.endif
# The modifier does not contain a "=" and thus falls back to the BSD modifier.
.if ${MOD_SUBST:S,from,to,} != "S s to to"
.  error
.endif


all:
