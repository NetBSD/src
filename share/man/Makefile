#	$NetBSD: Makefile,v 1.22 2002/06/10 18:47:12 mason Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/16/94

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
SUBDIR=	man0 man1 man3 man4 man5 man6 man7 man8 man9
MAKEWHATIS?=	/usr/libexec/makewhatis

makedb:
	${MAKEWHATIS} ${DESTDIR}/usr/share/man
.if defined(UNPRIVED)
	echo "${DESTDIR}/usr/share/man/whatis.db type=file mode=0444 uname=root gname=wheel" | \
		sed -e 's|^/|./|g' -e 's|//|/|g' | ${CAT} -l >> ${METALOG}
.endif
.else
makedb:
.endif

.include <bsd.subdir.mk>
