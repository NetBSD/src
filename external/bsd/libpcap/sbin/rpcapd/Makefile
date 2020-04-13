# $NetBSD: Makefile,v 1.1.2.2 2020/04/13 07:46:12 martin Exp $

USE_FORT?=	yes	# network server

.include <bsd.init.mk>

BINDIR=	/usr/sbin
.PATH:          ${SRCDIR}/rpcapd

PROG=		rpcapd
SRCS+=		daemon.c fileconf.c log.c rpcapd.c    

MANADMININ+= rpcapd.manadmin.in 
MANFILEIN+=  rpcapd-config.manfile.in

MAN=    ${MANADMININ:S/manadmin.in/8/g} ${MANFILEIN:S/manfile.in/5/g}
CLEANFILES+= ${MAN}

.for i in ${MANADMININ}
${i:S/manadmin.in/8/}: ${i} __sed
.endfor

.for i in ${MANFILEIN}  
${i:S/manfile.in/5/}: ${i} __sed
.endfor

FILES=		rpcapd.socket rpcapd.inetd.conf rpcapd@.service
FILESDIR=	/usr/share/examples/rpcapd

CPPFLAGS+=	-pthread
LDFLAGS+=	-pthread
CPPFLAGS+=	-I${SRCDIR} -I${.CURDIR}/../../include
CPPFLAGS+=	-DHAVE_CONFIG_H -D_OPENBSD_SOURCE
CPPFLAGS+=	-DPCAP_DONT_INCLUDE_PCAP_BPF_H

PROGDPLIBS+=	pcap ${.CURDIR}/../../lib \
		crypt ${NETBSDSRCDIR}/lib/libcrypt

.include <bsd.prog.mk>
