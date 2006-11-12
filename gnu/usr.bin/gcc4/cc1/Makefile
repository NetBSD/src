#	$NetBSD: Makefile,v 1.3 2006/11/12 01:24:44 christos Exp $

PROG=		cc1
SRCS=		${G_C_OBJS:Nlibcpp.a:.o=.c} main.c ${PROG}-checksum.c
CPPFLAGS+=	-DPREFIX=\"/usr\"

.include "../Makefile.backend"
.include "../Makefile.libcpp"

COPTS+=	-Wno-stack-protector

.PATH: ${DIST}/gcc ${G_out_file:H}
