#	$Id: NEWS-OS.4.x,v 1.1.1.1 2000/05/03 09:27:18 itojun Exp $
define(`confBEFORE', `limits.h')
define(`confMAPDEF', `-DNDBM')
define(`confLIBS', `-lmld')
define(`confMBINDIR', `/usr/lib')
define(`confSBINDIR', `/usr/etc')
define(`confUBINDIR', `/usr/ucb')
define(`confEBINDIR', `/usr/lib')
PUSHDIVERT(3)
limits.h:
	touch limits.h
POPDIVERT
