#	$NetBSD: Makefile,v 1.26 2002/02/18 09:37:11 itojun Exp $	

.include <bsd.own.mk>

SRCDIR=	${.CURDIR}/../../dist/tcpdump
.PATH:	${SRCDIR}

PROG=	tcpdump
MAN=	tcpdump.8 

SRCS=	addrtoname.c gmt2local.c machdep.c parsenfsfh.c \
	print-802_11.c print-ah.c print-arcnet.c print-arp.c \
	print-ascii.c print-atalk.c print-atm.c print-bgp.c \
	print-bootp.c print-beep.c print-cdp.c print-chdlc.c \
	print-cip.c print-cnfp.c print-decnet.c print-domain.c \
	print-dvmrp.c print-egp.c print-esp.c print-ether.c \
	print-fddi.c print-gre.c print-hsrp.c print-icmp.c \
	print-igmp.c print-igrp.c print-ip.c print-ipcomp.c \
	print-ipx.c print-isakmp.c print-isoclns.c print-krb.c \
	print-l2tp.c print-lane.c print-lcp.c print-llc.c print-lwres.c \
	print-msdp.c print-mobile.c print-mpls.c print-nfs.c \
	print-ntp.c print-null.c print-ospf.c print-pim.c \
	print-ppp.c print-pppoe.c print-pptp.c print-radius.c \
	print-raw.c print-rip.c print-rx.c print-sctp.c \
	print-sl.c print-sll.c print-snmp.c \
	print-stp.c print-sunrpc.c print-tcp.c \
	print-telnet.c print-tftp.c print-timed.c print-token.c \
	print-udp.c print-vjc.c print-vrrp.c print-wb.c print-zephyr.c \
	setsignal.c tcpdump.c util.c

SRCS+=	print-ip6.c print-ip6opts.c print-ripng.c print-icmp6.c print-frag6.c \
	print-rt6.c print-ospf6.c print-dhcp6.c

SRCS+=	bpf_dump.c

SRCS+=	version.c

# disabled for now
#SRCS+=	print-smb.c smbutil.c

version.c: VERSION
	sed -e 's/.*/char version[] = "&";/' ${.ALLSRC} > ${.TARGET}

LDADD+=	-lpcap -ll
DPADD+=	${LIBPCAP} ${LIBL}

CPPFLAGS+=	-I${.CURDIR} -I${SRCDIR}

AWKS= atime.awk packetdat.awk stime.awk send-ack.awk   

CPPFLAGS+= \
	-DHAVE___ATTRIBUTE__=1 -DHAVE_FCNTL_H=1 -DHAVE_RPC_RPCENT_H=1 \
	-DHAVE_NETINET_IF_ETHER_H=1 -DTIME_WITH_SYS_TIME=1 -DINET6=1 \
	-DHAVE_GETADDRINFO=1 -DHAVE_GETNAMEINFO=1 -DHAVE_SOCKADDR_SA_LEN=1 \
	-DSIZEOF_CHAR=1 -DSIZEOF_SHORT=2 -DSIZEOF_INT=4 -DSIZEOF_LONG=4 \
	-DHAVE_ADDRINFO=1 -DHAVE_SOCKADDR_STORAGE=1 -DHAVE_INADDRSZ=1 \
	-DHAVE_IN6ADDRSZ=1 -DHAVE_RES_USE_INET6=1 -DHAVE_VFPRINTF=1 \
	-DHAVE_STRCASECMP=1 -DHAVE_STRLCAT=1 -DHAVE_STRLCPY=1 \
	-DHAVE_STRDUP=1 -DHAVE_ETHER_NTOHOST=1 -DUSE_ETHER_NTOHOST=1 \
	-DHAVE_SETLINEBUF=1 -DHAVE_VSNPRINTF=1 -DHAVE_SNPRINTF=1 \
	-DRETSIGTYPE=void -DRETSIGVAL= -DHAVE_SIGACTION=1 \
	-DHAVE_SOCKADDR_SA_LEN=1 -DHAVE_H_ERRNO=1
#CPPFLAGS+=-DTCPDUMP_DO_SMB=1

.if (${MKCRYPTO} != "no")
CPPFLAGS+=-DHAVE_LIBCRYPTO=1 
LDADD+=	-lcrypto
DPADD+=	${LIBCRYPTO}
.endif

CLEANFILES+=	tcpdump.8 version.c

.include <bsd.prog.mk>
