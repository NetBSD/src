#	Id: OSF1.V5.x,v 8.1.2.1 2001/02/26 21:09:00 gshapiro Exp
define(`confCC', `cc -std1 -Olimit 1000')
define(`confMAPDEF', `-DNDBM -DNIS -DMAP_REGEX')
define(`confENVDEF', `-DHASSNPRINTF=1')
define(`confLIBS', `-ldbm')
define(`confSTDIR', `/var/adm/sendmail')
define(`confINSTALL', `installbsd')
define(`confEBINDIR', `/usr/lbin')
define(`confUBINDIR', `${BINDIR}')
define(`confDEPEND_TYPE', `CC-M')

define(`confMTLDOPTS', `-lpthread')
