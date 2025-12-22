#	$NetBSD: Makefile,v 1.26 2025/12/22 16:48:48 thorpej Exp $

.include <bsd.own.mk>

SUBDIR= amd apm asm devpubd disktab emul fstab ftpd getdate \
        lua npf pppd racoon syslogd wsmoused

.if (${MKWLAN} != "no")
SUBDIR+=hostapd wpa_supplicant
.endif

.include <bsd.subdir.mk>
