# 
#	$NetBSD: files.adb,v 1.6 2012/08/29 02:44:07 macallan Exp $
#
# Apple Desktop Bus protocol and drivers

defflag	adbdebug.h	ADB_DEBUG
defflag	adbdebug.h	ADBKBD_DEBUG
defflag	adbdebug.h	ADBMS_DEBUG
defflag	adbdebug.h	ADBBT_DEBUG
defflag adbdebug.h	ADBKBD_POWER_PANIC

define adb_bus {}

device nadb {}
attach nadb at adb_bus
file dev/adb/adb_bus.c		nadb needs-flag

device adbkbd : wskbddev, wsmousedev, sysmon_power, sysmon_taskq
attach adbkbd at nadb
file dev/adb/adb_kbd.c		adbkbd needs-flag
file dev/adb/adb_usb_map.c	adbkbd

device adbbt : wskbddev
attach adbbt at nadb
file dev/adb/adb_bt.c		adbbt

device adbms : wsmousedev
attach adbms at nadb
file dev/adb/adb_ms.c		adbms needs-flag
