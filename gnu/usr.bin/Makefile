#	$NetBSD: Makefile,v 1.45.2.1 1998/11/22 03:40:57 cgd Exp $

SUBDIR+=	bc cpio dc diff diff3 gawk grep groff
SUBDIR+=	gzip rcs sdiff send-pr sort tar

SUBDIR+=	gas gcc gdb ld

.if (${MACHINE_ARCH} == "mips")
SUBDIR+=	binutils gas.new ld.new
.endif

.include <bsd.subdir.mk>
