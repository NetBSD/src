# 
#	$NetBSD: files.adb,v 1.7.40.2 2020/04/13 08:04:18 martin Exp $
#
# Apple Desktop Bus protocol and drivers

defflag	adbdebug.h	ADB_DEBUG
defflag	adbdebug.h	ADBKBD_DEBUG
defflag	adbdebug.h	ADBMS_DEBUG
defflag	adbdebug.h	ADBBT_DEBUG
defflag adbdebug.h	ADBKBD_POWER_DDB
defflag	adbdebug.h	KTM_DEBUG

define adb_bus {}

device nadb {}
attach nadb at adb_bus
file dev/adb/adb_bus.c		nadb needs-flag

device adbkbd : wskbddev, wsmousedev, sysmon_power, sysmon_taskq
attach adbkbd at nadb
file dev/adb/adb_kbd.c		adbkbd needs-flag
file dev/adb/adb_usb_map.c	adbkbd
defflag	opt_adbkbd.h	ADBKBD_EMUL_USB

device adbbt : wskbddev
attach adbbt at nadb
file dev/adb/adb_bt.c		adbbt

device adbms : wsmousedev
attach adbms at nadb
file dev/adb/adb_ms.c		adbms needs-flag

device ktm : wsmousedev
attach ktm at nadb
file dev/adb/adb_ktm.c		ktm needs-flag
