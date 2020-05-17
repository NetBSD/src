# $NetBSD: Makefile.inc,v 1.9 2020/05/17 18:52:43 christos Exp $
WARNS ?= 4
CWARNFLAGS+= -Wno-missing-noreturn
TOOLSSRCDIR:= ${.PARSEDIR}
CPPFLAGS+= -I${TOOLSSRCDIR}
.if ${RUMPFIFO:Uno} == "yes"
LIBRUMPFIFO_PRE = -lrumpvfs_fifofs -lrumpnet_local -lrumpnet_net -lrumpnet
.endif
LIBRUMPFIFO = -lrumpvfs_nofifofs 
LIBRUMPBASE = ${LIBRUMPFIFO_PRE} -lrumpvfs -lrump -lrumpvfs -lrumpvfs_nofifofs -lrumpuser -lrump -lpthread

