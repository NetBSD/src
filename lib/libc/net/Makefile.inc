#	from: @(#)Makefile.inc	5.3 (Berkeley) 2/21/91
#	$Id: Makefile.inc,v 1.20 1995/02/10 17:46:52 cgd Exp $

# net sources
.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}/net ${.CURDIR}/net

SRCS+=	gethostnamadr.c getnetbyaddr.c getnetbyname.c getnetent.c \
	getproto.c getprotoent.c getprotoname.c getservbyname.c \
	getservbyport.c getservent.c herror.c inet_addr.c inet_lnaof.c \
	inet_makeaddr.c inet_netof.c inet_network.c inet_ntoa.c \
	iso_addr.c ns_addr.c ns_ntoa.c \
	iso_addr.c linkaddr.c ns_addr.c ns_ntoa.c rcmd.c recv.c res_comp.c \
	res_debug.c res_init.c res_mkquery.c res_query.c res_send.c \
	send.c sethostent.c ethers.c

.if   (${MACHINE_ARCH} == "m68k")
SRCS+=	htonl.S htons.S ntohl.S ntohs.S
.elif (${MACHINE_ARCH} == "i386")
SRCS+=	htonl.S htons.S ntohl.S ntohs.S
.elif (${MACHINE_ARCH} == "ns32k")
SRCS+=	htonl.S htons.S ntohl.S ntohs.S
.elif (${MACHINE_ARCH} == "tahoe")
SRCS+=	htonl.S htons.S ntohl.S ntohs.S
.elif (${MACHINE_ARCH} == "vax")
SRCS+=	htonl.S htons.S ntohl.S ntohs.S
.elif (${MACHINE_ARCH} == "sparc")
SRCS+=	htonl.S htons.S ntohl.S ntohs.S
.elif (${MACHINE_ARCH} == "mips")
SRCS+=	htonl.S htons.S
.elif (${MACHINE_ARCH} == "alpha")
# XXX should be .S's
SRCS+=	htonl.c htons.c ntohl.c ntohs.c
.else 
SRCS+=	htonl.c htons.c ntohl.c ntohs.c
.endif

MAN+=	byteorder.3 gethostbyname.3 getnetent.3 getprotoent.3 getservent.3 \
	inet.3 ns.3 rcmd.3 resolver.3 ethers.3

MLINKS+=byteorder.3 htonl.3 byteorder.3 htons.3 byteorder.3 ntohl.3 \
	byteorder.3 ntohs.3
MLINKS+=gethostbyname.3 endhostent.3 gethostbyname.3 gethostbyaddr.3 \
	gethostbyname.3 sethostent.3 gethostbyname.3 gethostent.3 \
	gethostbyname.3 herror.3
MLINKS+=getnetent.3 endnetent.3 getnetent.3 getnetbyaddr.3 \
	getnetent.3 getnetbyname.3 getnetent.3 setnetent.3
MLINKS+=getprotoent.3 endprotoent.3 getprotoent.3 getprotobyname.3 \
	getprotoent.3 getprotobynumber.3 getprotoent.3 setprotoent.3
MLINKS+=getservent.3 endservent.3 getservent.3 getservbyname.3 \
	getservent.3 getservbyport.3 getservent.3 setservent.3
MLINKS+=inet.3 addr.3 inet.3 inet_addr.3 inet.3 inet_aton.3 \
	inet.3 inet_lnaof.3 inet.3 inet_makeaddr.3 inet.3 inet_netof.3 \
	inet.3 inet_network.3 inet.3 inet_ntoa.3 inet.3 network.3 \
	inet.3 ntoa.3
MLINKS+=ns.3 ns_addr.3 ns.3 ns_ntoa.3
MLINKS+=rcmd.3 iruserok.3 rcmd.3 rresvport.3 rcmd.3 ruserok.3
MLINKS+=resolver.3 dn_comp.3 resolver.3 dn_expand.3 resolver.3 res_init.3 \
	resolver.3 res_mkquery.3 resolver.3 res_send.3 resolver.3 res_query.3 \
	resolver.3 res_search.3
