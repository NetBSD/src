#	$NetBSD: Makefile,v 1.1 2006/05/10 22:48:12 mrg Exp $

PROG=		cc1
SRCS=		${G_C_OBJS:Nlibcpp.a:.o=.c} main.c dummy-checksum.c
CPPFLAGS+=	-DPREFIX=\"/usr\"

.include "../Makefile.backend"
.include "../Makefile.libcpp"

.PATH: ${DIST}/gcc ${G_out_file:H}
