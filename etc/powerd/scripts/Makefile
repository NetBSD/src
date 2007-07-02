#	$NetBSD: Makefile,v 1.7 2007/07/02 15:26:18 xtraeme Exp $

.include <bsd.own.mk>

# If you're adding new scripts, don't forget to update following:
#	src/etc/mtree/special
#	src/distrib/sets/lists/etc/mi

CONFIGFILES=	acadapter lid_switch power_button reset_button sleep_button
CONFIGFILES+=	sensor_battery sensor_drive sensor_fan sensor_indicator
CONFIGFILES+=	sensor_power sensor_resistance sensor_temperature
CONFIGFILES+=	sensor_voltage

FILESDIR=	/etc/powerd/scripts
FILESMODE=	${BINMODE}

.include <bsd.prog.mk>
