#	$NetBSD: Makefile,v 1.27 2025/12/24 20:06:00 thorpej Exp $

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
