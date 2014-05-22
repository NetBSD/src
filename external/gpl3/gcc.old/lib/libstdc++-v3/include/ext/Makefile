#	$NetBSD: Makefile,v 1.1.1.2.4.2 2014/05/22 15:58:51 yamt Exp $

.include <bsd.init.mk>

.include "${.CURDIR}/../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

INCS=		${G_ext_headers}
INCSDIR=	/usr/include/g++/ext

# Get default targets including <bsd.inc.mk>.
.include <bsd.prog.mk>

# XXX Not yet
#SUBDIR=	pb_assoc

#.include <bsd.subdir.mk>

.PATH: ${DIST}/libstdc++-v3/include/ext
# for ext/hash_set
.PATH: ${DIST}/libstdc++-v3/include/backward
