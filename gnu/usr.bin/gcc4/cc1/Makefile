#	$NetBSD: Makefile,v 1.2 2006/07/01 19:06:51 mrg Exp $

PROG=		cc1
SRCS=		${G_C_OBJS:Nlibcpp.a:.o=.c} main.c ${PROG}-checksum.c
CPPFLAGS+=	-DPREFIX=\"/usr\"

.include "../Makefile.backend"
.include "../Makefile.libcpp"

.PATH: ${DIST}/gcc ${G_out_file:H}
