#	$NetBSD: Makefile,v 1.3 2014/10/30 21:15:13 justin Exp $

.include "../../Makefile.zfs"

PROG=	ztest
MKMAN=	no

DPADD=	${LIBM} ${LIBNVPAIR} ${LIBUMEM} ${LIBZPOOL} ${LIBPTHREAD} \
	${LIBZ} ${LIBAVL}

LIBNVPAIR_OBJDIR!=  cd ${LIBNVPAIR_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBNVPAIR_OBJDIR} -lnvpair

LIBUMEM_OBJDIR!=  cd ${LIBUMEM_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBUMEM_OBJDIR} -lumem

LIBZPOOL_OBJDIR!=  cd ${LIBZPOOL_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBZPOOL_OBJDIR} -lzpool

LIBAVL_OBJDIR!=  cd ${LIBAVL_SRCDIR} && ${PRINTOBJDIR}
LDADD+=         -L${LIBAVL_OBJDIR} -lavl

LDADD+=	-lm  -lpthread -lz

.PATH: 	 ${ZFSDIR}/dist/cmd/ztest

.include <bsd.prog.mk>
