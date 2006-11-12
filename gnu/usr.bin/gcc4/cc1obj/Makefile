#	$NetBSD: Makefile,v 1.3 2006/11/12 01:24:44 christos Exp $

MYOBJS=		${G_OBJC_OBJS} ${G_C_AND_OBJC_OBJS} 
PROG=		cc1obj
SRCS=		${MYOBJS:S/objc\///:Nlibcpp.a:.o=.c} main.c ${PROG}-checksum.c

CPPFLAGS+=	-I${DIST}/gcc/objc
CPPFLAGS.prefix.c+=	-DPREFIX=\"/usr\"

.include "../Makefile.backend"
.include "../Makefile.libcpp"

COPTS+=	-Wno-stack-protector

.PATH: ${DIST}/gcc/objc ${DIST}/gcc ${G_out_file:H}
