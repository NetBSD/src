# $Id: Makefile,v 1.3 1994/02/22 04:08:21 cgd Exp $
#
# Possible Options:
#  -DCSLIP	include support for kernel slip interface
#  -DPPP	include support for point to point protocol
#  -DIBMRTPC	enable the MERIT additions to the Stanford Enetfilter
#  -DNOVFPRINTF	simulate vfprintf() on systems that don't have it
#
# CSLIP and PPP work only under BPF.
#
PROG=	tcpdump
MAN8=	tcpdump.0
DEFINES = -DCSLIP -DPPP -DFDDI
CFLAGS+=-I. -I${.CURDIR} ${DEFINES}
.if ${MACHINE_ARCH} == "sparc"
CFLAGS+=-DTCPDUMP_ALIGN
.endif

LDADD=  -ll
DPADD=  ${LIBL}

SRCS=	tcpdump.c addrtoname.c \
	print-ether.c print-ip.c print-arp.c print-tcp.c print-udp.c \
	print-atalk.c print-domain.c print-tftp.c print-bootp.c print-nfs.c \
	print-icmp.c nametoaddr.c print-sl.c print-ppp.c print-rip.c \
	print-snmp.c print-ntp.c print-null.c print-egp.c print-ospf.c \
	print-fddi.c print-sunrpc.c savefile.c util.c etherent.c inet.c \
	gencode.c optimize.c bpf_dump.c bpf_filter.c \
	bpf_image.c tcpgram.y tcplex.l md.c pcap.c version.c
.PATH:	${.CURDIR}/../../sys/net

CLEANFILES+= y.tab.h


#SRCS2 = tcpslice.o gwtm2secs.o search.o savefile.o bpf_filter.o md.o util.o
#tcpslice:	SRSC2

.include <bsd.prog.mk>
