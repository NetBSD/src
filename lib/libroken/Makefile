# $NetBSD: Makefile,v 1.1.1.1 2000/06/16 19:27:32 thorpej Exp $
DIST=		${.CURDIR}/../../dist
.PATH: ${DIST}/heimdal/lib/roken
.PATH: ${.CURDIR}/../../include/heimdal

WARNS?=  1
MKLINT= no

includes: print_version.h

LIB=	roken

SRCS=	base64.c concat.c emalloc.c eread.c erealloc.c estrdup.c ewrite.c \
	getaddrinfo_hostspec.c \
	get_default_username.c get_window_size.c getarg.c \
	getnameinfo_verified.c issuid.c k_getpwnam.c k_getpwuid.c \
	mini_inetd.c net_read.c net_write.c parse_bytes.c parse_time.c \
	parse_units.c resolve.c roken_gethostby.c signal.c simple_exec.c \
	socket.c tm2time.c verify.c warnerr.c print_version.c \
	strcollect.c strlwr.c strndup.c strnlen.c strupr.c

INCS=	base64.h getarg.h parse_bytes.h parse_time.h parse_units.h \
	resolve.h roken.h roken-common.h

INCSDIR= /usr/include/krb5

DPSRCS=	print_version.h

CPPFLAGS+= -I. \
	-I${.CURDIR}/../../include/heimdal \
	-I${DIST}/heimdal/lib/roken \
	-DHAVE_CONFIG_H

make-print-version: make-print-version.o
	${CC} ${LDFLAGS} -o ${.TARGET} ${.ALLSRC}

print_version.h: make-print-version
	./make-print-version print_version.h

print_version.o print_version.so print_version.po print_version.So print_version.ln: print_version.h

CLEANFILES+= print_version.h make-print-version make-print-version.o

.include <bsd.lib.mk>
