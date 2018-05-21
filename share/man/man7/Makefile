#	$NetBSD: Makefile,v 1.31.14.1 2018/05/21 04:35:56 pgoyette Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.init.mk>

# missing: eqnchar.7 man.7 ms.7 term.7

MAN=	ascii.7 audio.7 c.7 environ.7 glob.7 hier.7 hostname.7 intro.7 \
	mailaddr.7 module.7 nls.7 operator.7 orders.7 pkgsrc.7 release.7 \
	rfc6056.7 security.7 script.7 setuid.7 signal.7 src.7 sticky.7 \
	symlink.7 sysctl.7 tests.7

CLEANFILES=	tests.7
.if ${MKKYUA} != "no"
tests.7: tests.kyua.7
	rm -f ${.TARGET} && cp ${.ALLSRC} ${.TARGET}
.else
tests.7: tests.atf.7
	rm -f ${.TARGET} && cp ${.ALLSRC} ${.TARGET}
.endif

MLINKS+=c.7 c78.7 \
	c.7 c89.7 \
	c.7 c90.7 \
	c.7 c99.7

.include <bsd.man.mk>
