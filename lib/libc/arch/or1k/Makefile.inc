#	$NetBSD: Makefile.inc,v 1.1 2014/09/03 19:34:25 matt Exp $

SRCS+=		__sigaction14_sigtramp.c __sigtramp2.S
SRCS+=		mulsi3.S

CPPFLAGS+=	-I. -isystem ${DESTDIR}/usr/include/c++

.if ${MKSOFTFLOAT} != "no"
.include <softfloat/Makefile.inc>
.endif
