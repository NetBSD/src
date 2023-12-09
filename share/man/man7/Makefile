#	$NetBSD: Makefile,v 1.36.6.1 2023/12/09 13:13:49 martin Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.init.mk>

# missing: eqnchar.7 man.7 ms.7 term.7

MAN+=	ascii.7
MAN+=	c.7
MAN+=	entropy.7
MAN+=	environ.7
MAN+=	glob.7
MAN+=	groups.7
MAN+=	hier.7
MAN+=	hostname.7
MAN+=	intro.7
MAN+=	kernel_sanitizers.7
MAN+=	mailaddr.7
MAN+=	module.7
MAN+=	nls.7
MAN+=	operator.7
MAN+=	orders.7
MAN+=	pkgsrc.7
MAN+=	release.7
MAN+=	rfc6056.7
MAN+=	script.7
MAN+=	security.7
MAN+=	setuid.7
MAN+=	signal.7
MAN+=	src.7
MAN+=	stack.7
MAN+=	sticky.7
MAN+=	symlink.7
MAN+=	sysctl.7
MAN+=	tests.7
MAN+=	users.7

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
	c.7 c99.7 \
	entropy.7 random.7

.include <bsd.man.mk>
