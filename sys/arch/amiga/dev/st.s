#NO_APP
gcc2_compiled.:
.globl _stdriver
.text
LC0:
	.ascii "st\0"
.data
	.even
_stdriver:
	.long _stinit
	.long LC0
	.long _ststart
	.long _stgo
	.long _stintr
	.long 0
	.even
_st_read_cmd:
	.long 6
	.byte 8
	.skip 27
	.even
_st_write_cmd:
	.long 6
	.byte 10
	.skip 27
.globl _st_debug
	.even
_st_debug:
	.long 0
.globl _st_dmaoddretry
	.even
_st_dmaoddretry:
	.long 0
.globl _st_exblklen
	.even
_st_exblklen:
	.long 0
.globl _st_exvup
	.even
_st_exvup:
	.long 168
.globl _st_exmotthr
	.even
_st_exmotthr:
	.long 128
.globl _st_exreconthr
	.even
_st_exreconthr:
	.long 160
.globl _st_exgapthr
	.even
_st_exgapthr:
	.long 7
	.even
_havest.4:
	.long 0
	.even
_st_inq.5:
	.long 6
	.byte 18
	.byte 0
	.byte 0
	.byte 0
	.byte 52
	.byte 0
	.skip 22
.text
LC1:
	.ascii "EXB-8200\0"
LC2:
	.ascii "VIPER 150\0"
LC3:
	.ascii "Python 25501\0"
LC4:
	.ascii "HP35450A\0"
.data
	.even
_modsel.8:
	.long 6
	.byte 21
	.byte 0
	.byte 0
	.byte 0
	.byte 18
	.byte 0
	.skip 22
	.even
_unitready.9:
	.long 6
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.skip 22
	.even
_modsense.10:
	.long 6
	.byte 26
	.byte 0
	.byte 0
	.byte 0
	.byte 18
	.byte 0
	.skip 22
.comm _cpuspeed,4
.comm _buf,4
.comm _buffers,4
.comm _nbuf,4
.comm _bufpages,4
.comm _swbuf,4
.comm _nswbuf,4
.comm _bufhash,6144
.comm _bfreelist,368
.comm _bswlist,92
.comm _bclnlist,4
.comm _format_parms,1020
.comm _reassign_parms,12
.comm _zombproc,4
.comm _allproc,4
.comm _initproc,4
.comm _pageproc,4
.comm _qs,256
.comm _whichqs,4
.comm _hostid,4
.comm _hostname,256
.comm _hostnamelen,4
.comm _domainname,256
.comm _domainnamelen,4
.comm _boottime,8
.comm _time,8
.comm _tz,8
.comm _hz,4
.comm _phz,4
.comm _tick,4
.comm _lbolt,4
.comm _averunnable,12
.comm _st_softc,714
.comm _st_mode,84
.lcomm _st_xsense,182
.lcomm _stcmd,224
.comm _sttab,644
.comm _stbuf,644
