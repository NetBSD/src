#NO_APP
gcc2_compiled.:
.globl _dmadebug
.data
	.even
_dmadebug:
	.long 0
.globl _SDMAC_address
	.even
_SDMAC_address:
	.long 0
.globl _SCSI_address
	.even
_SCSI_address:
	.long 0
.comm _cpuspeed,4
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
.comm _zombproc,4
.comm _allproc,4
.comm _initproc,4
.comm _pageproc,4
.comm _qs,256
.comm _whichqs,4
.comm _astpending,4
.comm _want_resched,4
.comm _dma_softc,88
.comm _dmachan,48
.comm _dmatimo,4
.comm _dmahits,4
.comm _dmamisses,4
.comm _dmabyte,4
.comm _dmaword,4
.comm _dmalword,4
