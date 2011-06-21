#	$NetBSD: Makefile,v 1.1 2011/06/21 06:03:13 mrg Exp $

PROG=		cc1
SRCS=		${G_C_OBJS:Nlibcpp.a:.o=.c} main.c ${PROG}-checksum.c
CPPFLAGS+=	-DPREFIX=\"/usr\"

.include "../Makefile.backend"
.include "../Makefile.libcpp"

COPTS+=	-Wno-stack-protector

.PATH: ${DIST}/gcc ${G_out_file:H}
