# $NetBSD: Makefile.inc,v 1.8 2020/05/17 18:25:11 christos Exp $
WARNS ?= 4
CWARNFLAGS+= -Wno-missing-noreturn
TOOLSSRCDIR:= ${.PARSEDIR}
CPPFLAGS+= -I${TOOLSSRCDIR}
.if ${RUMPFIFO:Uno} == "yes"
LIBRUMPFIFO = -lrumpvfs_fifofs -lrumpnet_local -lrumpnet_net -lrumpnet
.else
LIBRUMPFIFO = -lrumpvfs_nofifofs 
.endif
LIBRUMPBASE = -lrumpvfs -lrump -lrumpvfs ${LIBRUMPFIFO} -lrumpuser -lrump -lpthread

