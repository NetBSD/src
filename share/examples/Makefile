#	$NetBSD: Makefile,v 1.25.2.1 2026/11/04 23:19:44 perseant Exp $

.include <bsd.own.mk>

SUBDIR= amd apm asm devpubd disktab emul fstab ftpd getdate \
        lua npf pppd syslogd wsmoused

.if (${MKIPSEC} != "no")
SUBDIR+=racoon
.endif

.if (${MKWLAN} != "no")
SUBDIR+=hostapd wpa_supplicant
.endif

.include <bsd.subdir.mk>
