# $NetBSD: Makefile,v 1.1 2024/10/12 22:19:37 riastradh Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/usr.bin/ftp
TESTS_SH=	t_custom_headers

SCRIPTSDIR=	${TESTSDIR}
SCRIPTS+=	custom_headers.sh

# Keep the .sh suffix because we use it to trigger cgi-bin handling in
# bozohttpd (rather silly but it's easier that way).
SCRIPTSNAME_custom_headers.sh=	custom_headers.sh

.include <bsd.test.mk>
