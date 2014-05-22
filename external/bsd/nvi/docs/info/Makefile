#	$NetBSD: Makefile,v 1.1.4.2 2014/05/22 15:50:36 yamt Exp $

.include "${.CURDIR}/../../Makefile.inc"

.PATH:	${DIST}/docs/vi.ref

TEXINFO=	vi.texi
INFOFLAGS=	-I${DIST}/doc

vi.info: ref.texi vi.cmd.texi ex.cmd.texi set.opt.texi

.include <bsd.info.mk>
