#NO_APP
gcc2_compiled.:
.data
_rcsid:
	.ascii "$Header: /cvsroot/src/sys/arch/amiga/dev/Attic/scsi.s,v 1.1.1.1 1993/09/02 16:53:11 mw Exp $\0"
.globl _scsidriver
.text
LC0:
	.ascii "scsi\0"
.data
	.even
_scsidriver:
	.long _scsiinit
	.long LC0
	.long _scsistart
	.long _scsigo
	.long _scsiintr
	.long _scsidone
.globl _scsi_cmd_wait
	.even
_scsi_cmd_wait:
	.long 1000
.globl _scsi_data_wait
	.even
_scsi_data_wait:
	.long 1000
.globl _scsi_init_wait
	.even
_scsi_init_wait:
	.long 50000
.globl _scsi_nosync
	.even
_scsi_nosync:
	.long 1
.globl _scsi_debug
	.even
_scsi_debug:
	.long 0
.lcomm _saved_cmd_wait.6,4
.lcomm _saved_data_wait.7,4
	.even
_initialized.10:
	.long 0
.text
LC1:
	.ascii "\0"
.lcomm _sent_reset.19,8
.data
	.even
_cdb.32:
	.byte 0
	.skip 5
	.even
_cdb.35:
	.byte 3
	.skip 5
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
.comm _astpending,4
.comm _want_resched,4
.comm _scsi_softc,64
.comm _ixstart_wait,4096
.comm _ixin_wait,4096
.comm _ixout_wait,4096
.comm _mxin_wait,4096
.comm _mxin2_wait,4096
.comm _cxin_wait,4096
.comm _fxfr_wait,4096
.comm _sgo_wait,4096
