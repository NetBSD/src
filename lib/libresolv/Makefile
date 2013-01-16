#	$NetBSD: Makefile,v 1.11.48.1 2013/01/16 05:32:27 yamt Exp $
#	from: @(#)Makefile	5.1 (Berkeley) 6/5/90

.include <bsd.own.mk>

LIBC=	${NETBSDSRCDIR}/lib/libc

LIB=resolv
CPPFLAGS+=-I${LIBC}/include -I${LIBC}/resolv -D_LIBRESOLV

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

.PATH: ${LIBC}/net ${LIBC}/resolv ${LIBC}/nameser

SRCS+= res_update.c res_findzonecut.c res_mkupdate.c
SRCS+= res_sendsigned.c 
SRCS+= ns_date.c ns_samedomain.c ns_sign.c ns_verify.c
SRCS+= dst_api.c support.c hmac_link.c

# LIBC
# CPPFLAGS+=-D_LIBC -DCOMPAT__RES -DUSE_POLL -DLIBC_SCCS
# SRCS+= res_comp.c res_send.c res_data.c res_debug.c res_mkquery.c
# SRCS+= res_query.c res_mkupdate.c res_init.c res_state.c 
# SRCS+= ns_netint.c ns_name.c ns_print.c ns_parse.c ns_ttl.c ns_samedomain.c

.include <bsd.lib.mk>
