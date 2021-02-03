# $NetBSD: Makefile,v 1.6 2021/02/03 06:46:27 roy Exp $

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

CWARNFLAGS.gcc+=        ${GCC_NO_ADDR_OF_PACKED_MEMBER}

.include <bsd.prog.mk>
