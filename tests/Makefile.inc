# $NetBSD: Makefile.inc,v 1.11 2023/04/24 20:35:49 uwe Exp $
WARNS ?= 4
CWARNFLAGS+= -Wno-missing-noreturn
TOOLSSRCDIR:= ${.PARSEDIR}
CPPFLAGS+= -I${TOOLSSRCDIR}
.if ${RUMPFIFO:Uno} == "yes"
LIBRUMPFIFO_PRE = -lrumpvfs_fifofs -lrumpnet_local -lrumpnet_net -lrumpnet
.endif
LIBRUMPFIFO = -lrumpvfs_nofifofs 
LIBRUMPBASE = ${LIBRUMPFIFO_PRE} -lrumpvfs -lrumpvfs_nofifofs -lrump -lrumpuser -lpthread

.if ${RUMP_SANITIZE:Uno} != "no"
LIBRUMPBASE+=	-fsanitize=${RUMP_SANITIZE}
.endif
