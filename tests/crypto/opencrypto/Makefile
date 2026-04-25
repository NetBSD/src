# $NetBSD: Makefile,v 1.6 2026/04/25 00:37:29 christos Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/crypto/opencrypto

TESTS_SH+=	t_opencrypto

BINDIR=		${TESTSDIR}
MKMAN=		no

PROGS+=		h_aescbc
PROGS+=		h_aesctr1
PROGS+=		h_aesctr2
PROGS+=		h_arc4
PROGS+=		h_camellia
PROGS+=		h_cbcdes
PROGS+=		h_cbc3des
PROGS+=		h_comp
PROGS+=		h_comp_zlib
PROGS+=		h_comp_zlib_rnd
PROGS+=		h_gcm
PROGS+=		h_md5
PROGS+=		h_md5hmac
PROGS+=		h_null
PROGS+=		h_sha1hmac
PROGS+=		h_sha2hmac
PROGS+=		h_xcbcmac
PROGS+=		h_ioctl
PROGS+=		h_thread

LDADD.h_comp_zlib+=	-lz
DPADD.h_comp_zlib+=	${LIBZ}
LDADD.h_comp_zlib_rnd+=	-lz
DPADD.h_comp_zlib_rnd+=	${LIBZ}
LDADD.h_thread+=	-lpthread
DPADD.h_thread+=	${LIBPTHREAD}

.include <bsd.test.mk>
