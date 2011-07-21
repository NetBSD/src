#	$NetBSD: Makefile,v 1.4 2011/07/21 03:13:32 mrg Exp $

MYOBJS=		${G_OBJC_OBJS} ${G_C_AND_OBJC_OBJS} 
PROG=		cc1obj
SRCS=		${MYOBJS:S/objc\///:Nlibcpp.a:.o=.c} main.c ${PROG}-checksum.c

CPPFLAGS+=	-I${DIST}/gcc/objc
CPPFLAGS.prefix.c+=	-DPREFIX=\"/usr\"

.include "../Makefile.backend"
.include "../Makefile.libcpp"
.include "../Makefile.libdecnumber"

# VER_CPPFLAGS from Makefile.inc
CPPFLAGS.c-cppbuiltin.c=	${VER_CPPFLAGS}

COPTS+=	-Wno-stack-protector

LDADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} -lintl -lz
DPADD+=	${LIBMPC} ${LIBMPFR} ${LIBGMP} ${LIBINTL} ${LIBZ}

.PATH: ${DIST}/gcc/objc ${DIST}/gcc ${G_out_file:H}
