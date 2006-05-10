#	$NetBSD: Makefile,v 1.1 2006/05/10 22:48:12 mrg Exp $

MYOBJS=		${G_OBJC_OBJS} ${G_C_AND_OBJC_OBJS} 
PROG=		cc1obj
SRCS=		${MYOBJS:S/objc\///:Nlibcpp.a:.o=.c} main.c dummy-checksum.c

CPPFLAGS+=	-I${DIST}/gcc/objc
CPPFLAGS.prefix.c+=	-DPREFIX=\"/usr\"

.include "../Makefile.backend"
.include "../Makefile.libcpp"

.PATH: ${DIST}/gcc/objc ${DIST}/gcc ${G_out_file:H}
