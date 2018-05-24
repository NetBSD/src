# $NetBSD: varquote.mk,v 1.1 2018/05/24 00:25:44 christos Exp $
#
# Test VAR:Q modifier

.if !defined(REPROFLAGS)
REPROFLAGS+=    -fdebug-prefix-map=\$$NETBSDSRCDIR=/usr/src
REPROFLAGS+=    -fdebug-regex-map='/usr/src/(.*)/obj$$=/usr/obj/\1'
all:
	@${MAKE} -f ${MAKEFILE} REPROFLAGS=${REPROFLAGS:Q}
.else
all:
	@echo ${REPROFLAGS}
.endif
