# $NetBSD: Makefile,v 1.3.6.2 2014/05/22 11:43:04 yamt Exp $

.include <bsd.own.mk>

PROG=   ldpd
MAN=    ldpd.8 ldpd.conf.5

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

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

.include <bsd.prog.mk>
