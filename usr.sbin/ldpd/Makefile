# $NetBSD: Makefile,v 1.3 2010/12/30 11:29:21 kefren Exp $

.include <bsd.own.mk>

PROG=   ldpd
MAN=    ldpd.8

SRCS=   conffile.c \
	fsm.c \
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

LDADD+=	-lcrypt

.include <bsd.prog.mk>
