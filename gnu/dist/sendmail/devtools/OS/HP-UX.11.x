#	Id: HP-UX.11.x,v 8.11.18.3 2000/09/19 07:04:52 gshapiro Exp

# +z is to generate position independant code
define(`confCC', `cc -Ae +z')
define(`confMAPDEF', `-DNDBM -DNIS -DMAP_REGEX')
define(`confENVDEF', `-DV4FS -DHPUX11 -D_PATH_SENDMAIL=\"/usr/sbin/sendmail\"')
define(`confOPTIMIZE', `+O3')
define(`confLIBS', `-ldbm -lnsl')
define(`confSHELL', `/usr/bin/sh')
define(`confINSTALL', `${BUILDBIN}/install.sh')
define(`confSBINGRP', `mail')
define(`confEBINDIR', `/usr/sbin')

define(`confMTCCOPTS', `-D_POSIX_C_SOURCE=199506L')
define(`confMTLDOPTS', `-lpthread')
define(`confLD', `ld')
define(`confLDOPTS_SO', `-b')
define(`confCCOPTS_SO', `')

define(`confSONAME',`+h')
define(`confSHAREDLIB_EXT',`.sl')
