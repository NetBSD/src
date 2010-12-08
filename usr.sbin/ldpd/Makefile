# $NetBSD: Makefile,v 1.1 2010/12/08 07:20:14 kefren Exp $

.include <bsd.own.mk>

PROG=   ldpd
MAN=    ldpd.8

SRCS=   fsm.c \
	label.c \
	ldp_command.c \
	ldp_errors.c \
	ldp_peer.c \
	main.c \
	mpls_interface.c \
	mpls_routes.c \
	notifications.c \
	pdu.c \
	socketops.c \
	tlv.c \
	tlv_stack.c

CFLAGS=	-Wall -g

LDADD+=	-lcrypt

.include <bsd.prog.mk>
