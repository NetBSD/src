#	$NetBSD: Makefile,v 1.6 2007/07/01 07:55:53 xtraeme Exp $

.include <bsd.own.mk>

# If you're adding new scripts, don't forget to update following:
#	src/etc/mtree/special
#	src/distrib/sets/lists/etc/mi

CONFIGFILES=	acadapter lid_switch power_button reset_button sleep_button
CONFIGFILES+=	sensor_battery sensor_drive sensor_fan sensor_power
CONFIGFILES+=	sensor_resistance sensor_temperature sensor_voltage

FILESDIR=	/etc/powerd/scripts
FILESMODE=	${BINMODE}

.include <bsd.prog.mk>
