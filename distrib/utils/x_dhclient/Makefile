#	$NetBSD: Makefile,v 1.17 2011/06/20 07:43:57 mrg Exp $

NOMAN=
.include <bsd.own.mk>

RSRCDIR=	${NETBSDSRCDIR}/dist/dhcp

.PATH:		${RSRCDIR}/client

PROG=		dhclient
SRCS=		dhclient.c clparse.c
CPPFLAGS+=	-DCLIENT_PATH='"PATH=/usr/bin:/usr/sbin:/bin:/sbin"' \
		-DSMALL -I${RSRCDIR} -I${RSRCDIR}/includes

.if defined(HAVE_GCC)
CPPFLAGS+=	-fno-strict-aliasing
.endif

#	common source
#
.PATH:	${RSRCDIR}/common
SRCS+=	alloc.c bpf.c comapi.c conflex.c discover.c dispatch.c dns.c \
	ethernet.c execute.c fddi.c inet.c memory.c options.c packet.c \
	parse.c print.c socket.c tables.c tree.c 

#	omapip source
#
.PATH:	${RSRCDIR}/omapip
SRCS+=	alloc.OM.c convert.c dispatch.OM.c errwarn.c handle.c hash.c \
	result.c support.c


.SUFFIXES: .OM.c

.c.OM.c:
	rm -f ${.TARGET}
	cp ${RSRCDIR}/omapip/${.IMPSRC:T} ${.TARGET}

DPSRCS+=	${SRCS:M*.OM.c}
CLEANFILES+=	${SRCS:M*.OM.c}

.include <bsd.prog.mk>
