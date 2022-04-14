#	$NetBSD: Makefile,v 1.1 2022/04/14 16:50:26 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/scsipi/

KMOD=	scsi_subr
SRCS=	scsi_subr.c scsi_base.c

.include <bsd.kmodule.mk>
