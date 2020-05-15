# $NetBSD: Makefile.inc,v 1.7 2020/05/15 23:32:28 christos Exp $
WARNS ?= 4
CWARNFLAGS+= -Wno-missing-noreturn
TOOLSSRCDIR:= ${.PARSEDIR}
CPPFLAGS+= -I${TOOLSSRCDIR}
.if ${RUMPFIFO:Uno} == "yes"
LIBRUMPFIFO = -lrumpvfs_fifofs -lrumpnet_local -lrumpnet_net -lrumpnet
.else
LIBRUMPFIFO = -lrumpvfs_nofifofs 
.endif
LIBRUMPBASE = -lrumpvfs -lrump ${LIBRUMPFIFO} -lrumpvfs -lrumpuser -lrump -lpthread

