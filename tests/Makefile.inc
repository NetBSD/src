# $NetBSD: Makefile.inc,v 1.10 2021/07/07 11:51:45 martin Exp $
WARNS ?= 4
CWARNFLAGS+= -Wno-missing-noreturn
TOOLSSRCDIR:= ${.PARSEDIR}
CPPFLAGS+= -I${TOOLSSRCDIR}
.if ${RUMPFIFO:Uno} == "yes"
LIBRUMPFIFO_PRE = -lrumpvfs_fifofs -lrumpnet_local -lrumpnet_net -lrumpnet
.endif
LIBRUMPFIFO = -lrumpvfs_nofifofs 
LIBRUMPBASE = ${LIBRUMPFIFO_PRE} -lrumpvfs -lrump -lrumpvfs -lrumpvfs_nofifofs -lrumpuser -lrump -lpthread

.if ${RUMP_SANITIZE:Uno} != "no"
LIBRUMPBASE+=	-fsanitize=${RUMP_SANITIZE}
.endif
