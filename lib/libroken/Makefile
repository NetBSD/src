# $NetBSD: Makefile,v 1.15 2007/05/28 12:06:21 tls Exp $

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:		${DIST}/heimdal/lib/roken ${NETBSDSRCDIR}/include/heimdal

WARNS?=		1

LIB=		roken

.if ${OBJECT_FMT} == "ELF"
LIBDPLIBS+= util ${.CURDIR}/../libutil
.else
.PATH: ${NETBSDSRCDIR}/lib/libutil
SRCS+=	efun.c
.endif

SRCS=						\
	base64.c				\
	concat.c				\
	environment.c				\
	eread.c					\
	esetenv.c				\
	ewrite.c				\
	get_default_username.c			\
	get_window_size.c			\
	getaddrinfo_hostspec.c			\
	getarg.c				\
	getnameinfo_verified.c			\
	getprogname.c				\
	h_errno.c				\
	hostent_find_fqdn.c			\
	issuid.c				\
	k_getpwnam.c				\
	k_getpwuid.c				\
	mini_inetd.c				\
	net_read.c				\
	net_write.c				\
	parse_bytes.c				\
	parse_time.c				\
	parse_units.c				\
	resolve.c				\
	roken_gethostby.c			\
	rtbl.c					\
	setprogname.c				\
	signal.c				\
	simple_exec.c				\
	socket.c				\
	strcollect.c				\
	strlwr.c				\
	strndup.c				\
	strnlen.c				\
	strsep_copy.c				\
	strupr.c				\
	tm2time.c				\
	verify.c				\
	warnerr.c				\
	write_pid.c

INCS=		base64.h getarg.h parse_bytes.h parse_time.h parse_units.h \
		resolve.h roken.h roken-common.h rtbl.h
INCSDIR=	/usr/include/krb5

CPPFLAGS+= -I. \
	-I${NETBSDSRCDIR}/include/heimdal \
	-I${DIST}/heimdal/lib/roken \
	-DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

HOST_CPPFLAGS+=	-I${NETBSDSRCDIR}/include/heimdal -DHAVE_CONFIG_H

.include <bsd.lib.mk>
