#	$NetBSD: Makefile,v 1.1 2009/08/07 20:57:59 haad Exp $

.include "../../Makefile.zfs"

PROG=	ztest
MKMAN=	no

DPADD=	${LIBM} ${LIBNVPAIR} ${LIBUMEM} ${LIBZPOOL} ${LIBPTHREAD} \
	${LIBZ} ${LIBAVL}

LDADD+=-Lhehehe

LIBAVL_OBJDIR!=  cd ${LIBAVL_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBAVL_OBJDIR} -lavl

LIBNVPAIR_OBJDIR!=  cd ${LIBNVPAIR_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBNVPAIR_OBJDIR} -lnvpair

LIBUMEM_OBJDIR!=  cd ${LIBUMEM_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBUMEM_OBJDIR} -lumem

LIBZPOOL_OBJDIR!=  cd ${LIBZPOOL_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBZPOOL_OBJDIR} -lzpool

LDADD+=	-lm  -lpthread -lz

.PATH: 	 ${ZFSDIR}/dist/cmd/ztest

.include <bsd.prog.mk>
