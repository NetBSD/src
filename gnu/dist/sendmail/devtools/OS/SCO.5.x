#	$Id: SCO.5.x,v 1.1.1.1 2000/05/03 09:27:18 itojun Exp $
define(`confCC', `cc -b elf')
define(`confLIBS', `-lsocket -lndbm -lprot -lcurses -lm -lx -lgen')
define(`confMAPDEF', `-DMAP_REGEX -DNDBM')
define(`confSBINGRP', `bin')
define(`confMBINDIR', `/usr/lib')
define(`confSBINDIR', `/usr/etc')
define(`confUBINDIR', `/usr/bin')
define(`confINSTALL', `${BUILDBIN}/install.sh')
