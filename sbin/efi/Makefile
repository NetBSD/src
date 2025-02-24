#	$NetBSD: Makefile,v 1.1 2025/02/24 13:47:55 christos Exp $

WARNS=9	

.include <bsd.init.mk>

GPT=${NETBSDSRCDIR}/sbin/gpt

.PATH: ${GPT}

CFLAGS+=	-Wno-address-of-packed-member
CPPFLAGS+=	-I${GPT}
DPADD+= 	${LIBUTIL}
LDADD+= 	-lutil

PROG=		efi
MAN=		${PROG}.8

LINTFLAGS.certs.c += -X177
LINTFLAGS.devpath3.c += -X177
LINTFLAGS.devpath4.c += -X177

SRCS=		main.c
SRCS+=		bootvar.c
SRCS+=		certs.c
SRCS+=		devpath.c
SRCS+=		devpath1.c
SRCS+=		devpath2.c
SRCS+=		devpath3.c
SRCS+=		devpath4.c
SRCS+=		devpath5.c
SRCS+=		efiio.c
SRCS+=		getvars.c
SRCS+=		gpt.c
SRCS+=		gpt_uuid.c
SRCS+=		gptsubr.c
SRCS+=		map.c
SRCS+=		setvar.c
SRCS+=		showvar.c
SRCS+=		utils.c

.include <bsd.prog.mk>
