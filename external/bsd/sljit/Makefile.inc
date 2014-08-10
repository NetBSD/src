# $NetBSD: Makefile.inc,v 1.1.10.1 2014/08/10 07:10:11 tls Exp $

.include <bsd.own.mk>

SLJITDIST=	${NETBSDSRCDIR}/sys/external/bsd/sljit/dist
LIBSLJITDIR!=	cd ${NETBSDSRCDIR}/external/bsd/sljit/lib && ${PRINTOBJDIR}

CPPFLAGS+=	-I${SLJITDIST}/sljit_src

# Link to libarm to get arm_sync_icache(2), for mips it's not
# required because _cacheflush() is in libc.
.if !empty(MACHINE_ARCH:Marm*) || !empty(MACHINE_ARCH:Mearm*)
LDADD+=	-larm
.endif
