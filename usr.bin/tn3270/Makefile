#	$Id: Makefile,v 1.5 1995/11/22 05:38:58 cgd Exp $

.if make(clean) || make(cleandir) || make(depend) || make(obj) || make(lint)
SUBDIR += tools
.endif
SUBDIR += tn3270 mset

.include <bsd.subdir.mk>
