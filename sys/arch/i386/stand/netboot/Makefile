#	$NetBSD: Makefile,v 1.28 2003/10/08 18:38:43 dsl Exp $

SUBDIR+=	3c509
SUBDIR+=	.WAIT
SUBDIR+=	3c590
SUBDIR+=	3c90xb
SUBDIR+=	i82557
SUBDIR+=	ne2000_isa
SUBDIR+=	pcnet_isapnp
SUBDIR+=	pcnet_pci
SUBDIR+=	wd80x3

LIBOBJ=	${.OBJDIR}
.MAKEOVERRIDES+= LIBOBJ

.include <bsd.subdir.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib
