#	$NetBSD: Makefile,v 1.2.12.2 2014/08/20 00:04:53 tls Exp $

NOMAN=	# defined

.include <bsd.own.mk>

HOSTPROGNAME=   ${_TOOL_PREFIX}ctfmerge
HOST_SRCDIR=    external/cddl/osnet/usr.bin/ctfmerge

LIBELFTOOLCHAIN_DIR=	${.CURDIR}/../../external/bsd/elftoolchain/dist
LIBELF_DIR=		${LIBELFTOOLCHAIN_DIR}/libelf
LIBDWARF_DIR=		${LIBELFTOOLCHAIN_DIR}libdwarf

HOST_CPPFLAGS+=	-I${OSNETDIR}/sys \
		-I${OSNETDIR}/include \
		-I${OPENSOLARIS_DISTDIR} \
		-I${OPENSOLARIS_DISTDIR}/head \
		-I${OPENSOLARIS_DISTDIR}/tools/ctf/common \
		-I${OPENSOLARIS_DISTDIR}/tools/ctf/cvt \
		-I${OPENSOLARIS_DISTDIR}/uts/common \
		-I${LIBELF_DIR} \
		-I${LIBDWARF_DIR}

HOST_CPPFLAGS+=	-DHAVE_NBTOOL_CONFIG_H=1

OSNETDIR=	${.CURDIR}/../../external/cddl/osnet
OPENSOLARIS_DISTDIR= ${OSNETDIR}/dist

CTFOBJ!=	cd ${.CURDIR}/../libctf && ${PRINTOBJDIR}
DWARFOBJ!=	cd ${.CURDIR}/../libdwarf && ${PRINTOBJDIR}
ELFOBJ!=	cd ${.CURDIR}/../libelf && ${PRINTOBJDIR}
LDADD+=		-L${CTFOBJ} -lctf
LDADD+=		-L${DWARFOBJ} -ldwarf
LDADD+=		-L${ELFOBJ} -lelf

# need native pthread support
LDADD+=		-lpthread

.PATH:		${OPENSOLARIS_DISTDIR}/tools/ctf/common
.PATH:		${OPENSOLARIS_DISTDIR}/tools/ctf/cvt

.include "${.CURDIR}/../Makefile.nbincludes"
.include "${.CURDIR}/../Makefile.host"
