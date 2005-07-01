

.include "../Makefile.serverlib"
.include <bsd.x11.mk>

.if defined(X11FLAGS.LOADABLE)
SRCS=		afbmodule.c
CPPFLAGS+=	${X11FLAGS.LOADABLE}
XMODULEDIR=	${X11USRLIBDIR}/modules
.endif

.include "Makefile.afb"
