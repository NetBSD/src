#	$NetBSD: Makefile,v 1.7 1998/09/05 14:58:51 lukem Exp $

.if make(clean) || make(cleandir) || make(distclean) || make(depend) || \
	make(obj) || make(lint)
SUBDIR += tools
.endif
SUBDIR += tn3270 mset

.include <bsd.subdir.mk>
