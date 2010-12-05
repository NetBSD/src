#	$NetBSD: Makefile,v 1.1 2010/12/05 05:14:00 christos Exp $	

WARNS?=	1	# XXX: need to cleanup later

.include <bsd.own.mk>

USE_FORT?= yes	# network client

SRCDIR=	${.CURDIR}/../dist
.PATH:	${SRCDIR}

PROG=	tcpdump
MAN=	tcpdump.8 

SRCS=	addrtoname.c cpack.c gmpls.c gmt2local.c machdep.c oui.c parsenfsfh.c \
		pf_print_state.c \
	print-802_11.c print-ah.c print-aodv.c print-ap1394.c print-arcnet.c \
	print-arp.c \
	print-ascii.c print-atalk.c print-atm.c print-bfd.c print-bgp.c \
	print-bootp.c print-beep.c print-cdp.c print-chdlc.c \
	print-cip.c print-cnfp.c print-decnet.c print-domain.c \
	print-dvmrp.c print-egp.c print-enc.c print-esp.c print-ether.c \
	print-fddi.c print-fr.c print-gre.c print-hsrp.c print-icmp.c \
	print-igmp.c print-igrp.c print-ip.c print-ipcomp.c print-ipfc.c \
	print-ipx.c print-isakmp.c print-isoclns.c print-krb.c \
	print-l2tp.c print-lane.c print-ldp.c print-llc.c print-lwres.c \
	print-msdp.c print-mobile.c print-mobility.c print-mpls.c print-nfs.c \
	print-ntp.c print-null.c print-ospf.c print-pflog.c print-pfsync.c \
	print-pim.c print-ppp.c print-pppoe.c print-pptp.c \
	print-radius.c print-raw.c print-rip.c print-rsvp.c print-rx.c \
	print-sctp.c print-sl.c print-sll.c print-smb.c print-snmp.c \
	print-stp.c print-sunatm.c print-sunrpc.c print-tcp.c \
	print-telnet.c print-tftp.c print-timed.c print-token.c \
	print-udp.c print-vjc.c print-vrrp.c print-wb.c print-zephyr.c \
	setsignal.c smbutil.c tcpdump.c util.c checksum.c signature.c
SRCS+=	print-eigrp.c l2vpn.c print-slow.c print-eap.c nlpid.c print-dccp.c \
	print-pgm.c ipproto.c print-syslog.c print-sip.c print-lmp.c \
	print-lspping.c print-symantec.c print-juniper.c af.c print-olsr.c \
	print-sflow.c print-vqp.c print-lwapp.c print-ipnet.c print-forces.c \
	print-vtp.c print-udld.c print-dtp.c print-rrcp.c print-mpcp.c \
	print-lldp.c print-cfm.c


SRCS+=	bpf_dump.c

SRCS+=	version.c

# disabled for now
#SRCS+=	print-smb.c smbutil.c

version.c: VERSION
	${_MKTARGET_CREATE}
	${TOOL_SED} -e 's/.*/char version[] = "&";/' ${.ALLSRC} > ${.TARGET}

LDADD+=	-lpcap -ll
DPADD+=	${LIBPCAP} ${LIBL}

CPPFLAGS+=	-I${.CURDIR}/../include -I${.CURDIR} -I${SRCDIR}
CPPFLAGS+=  -I${NETBSDSRCDIR}/dist/pf/sbin/pfctl \
	-I${NETBSDSRCDIR}/sys/dist/pf

AWKS= atime.awk packetdat.awk stime.awk send-ack.awk   

CPPFLAGS+=-DHAVE_CONFIG_H -D_U_="__attribute__((__unused__))"

.if (${USE_INET6} != "no")
SRCS+=	print-ip6.c print-ip6opts.c print-ripng.c print-icmp6.c print-frag6.c \
	print-rt6.c print-ospf6.c print-dhcp6.c

CPPFLAGS+=-DINET6=1 -DHAVE_RES_USE_INET6=1 
.endif

.if (${MKCRYPTO} != "no")
CPPFLAGS+=-DHAVE_LIBCRYPTO=1 
LDADD+=	-lcrypto -lcrypt
DPADD+=	${LIBCRYPTO} ${LIBCRYPT}
.endif

#.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
#COPTS+=	-fno-strict-aliasing
#.for f in print-802_11 print-esp print-fr print-radius print-telnet tcpdump util
#COPTS.${f}.c+=  -Wno-pointer-sign
#.endfor
#.endif

CLEANFILES+=	version.c tcpdump.8

tcpdump.8: tcpdump.1.in
	@rm -f ${.TARGET}
	cp ${.ALLSRC} ${.TARGET}

.include <bsd.prog.mk>
