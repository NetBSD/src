|
|	check FD format
|
|	Written by Yasha (ITOH Yasufumi)
|	This code is in the public domain
|
| $NetBSD: chkfmt.s,v 1.1 2001/09/27 10:03:27 minoura Exp $

/* FDC address */
#define FDC_STATUS		0xE94001	/* status register */
#define FDC_DATA		0xE94003	/* data register */

#define INT_STAT		0xE9C001	/* interrupt control */
#define INT_FDC_BIT		2

/* Status Register */
#define NE7ST_CB_BIT		4		/* FDC busy */
#define NE7ST_DIO_BIT		6		/* data input/output */
#define NE7ST_RQM_BIT		7		/* request for master */

/* FDC command */
#define NE7CMD_READID		0x4A		/* READ ID */

|
|	Read ID of all sectors in current track.
|	This routine expects that motor on, drive selection
|	and seek is already done.
|
|	input:	d0.b: 0 0 0 0 0 HD US1 US0 (binary)
|	output:	d0.l: min # sector (N C H R (2hex))
|		d1.l: max # sector (N C H R (2hex))
|	destroy:
|		d0, d1
|
	.text
	.even
	.globl	check_fd_format
check_fd_format:
	moveml	%d2-%d7/%a1-%a3,%sp@-

	moveb	%d0,%d6			| head, drive

	lea	FDC_STATUS:l,%a1
	lea	%a1@(FDC_DATA-FDC_STATUS),%a2	| FDC_DATA
	lea	%a2@(INT_STAT-FDC_DATA),%a3	| INT_STAT

	movew	%sr,%sp@-
	oriw	#0x0700,%sr		| keep out interrupts
	moveq	#INT_FDC_BIT,%d5
	bclr	%d5,%a3@		| disable FDC interrupt

	jbsr	read_id_sub
	jne	exit_check_format
	movel	%d3,%d2			| first sector
	movel	%d3,%d0			| sector min
sector_is_max:
	movel	%d3,%d1			| sector max

loop_read_id:
	jbsr	read_id_sub
	jne	exit_check_format
	cmpl	%d0,%d3
	jcc	sector_not_min
	movel	%d3,%d0
sector_not_min:
	cmpl	%d3,%d1
	jcs	sector_is_max
	cmpl	%d2,%d3
	jne	loop_read_id

exit_check_format:
	bset	%d5,%a3@		| enable FDC interrupt
	movew	%a7@+,%sr

	tstl	%d7
	moveml	%a7@+,%d2-%d7/%a1-%a3
	jne	_err_read_id

	rts

|
|	input:	d6.b: 0 0 0 0 0 HD US1 US0 (binary)
|		a1: FDC status addr
|		a2: FDC data addr
|		interrupt must be disabled
|	output:	d3.l: sector information: N C H R (2hex)
|		d7.l: status (nonzero if error)
|		Z flag: true if no error, false if error
|	destroy:
|		d3-d4, d7
|
read_id_sub:
	| wait for FDC ready
fdc_wait_ready:
	btst	#NE7ST_CB_BIT,%a1@
	jne	fdc_wait_ready

	| send READ ID command
fdc_send_command1:
	jbsr	fdc_wait_rqm
	jmi	fdc_send_command1
	moveb	#NE7CMD_READID,%a2@

fdc_send_command2:
	jbsr	fdc_wait_rqm
	jmi	fdc_send_command2
	moveb	%d6,%a2@		| X X X X X HD US1 US0 (binary)

	| receive data
	moveq	#2,%d4
	jbsr	fdc_read_bytes
	movel	%d3,%d7			| d7: FDC status: X ST0 ST1 ST2 (2hex)
	moveq	#3,%d4
	jbsr	fdc_read_bytes		| d3: sector info: C H R N (2hex)
	rorl	#8,%d3			| d3: sector info: N C H R (2hex)

	andil	#0x00f8ffff,%d7		| check status (must be zero)
	rts

|
|	receive d4:w + 1 bytes from FDC
|
fdc_read_bytes:
	asll	#8,%d3
fdc_read_loop:
	jbsr	fdc_wait_rqm
	jpl	fdc_read_loop
	moveb	%a2@,%d3
	dbra	%d4,fdc_read_bytes
	rts

fdc_wait_rqm:
	moveb	%a1@,%d3
	jpl	fdc_wait_rqm		| NE7ST_RQM_BIT = 7: sign bit
	lslb	#1,%d3			| NE7ST_DIO_BIT = 6 ... move to bit #7
	rts

|
|	error
|
_err_read_id:
	BOOT_ERROR("READ ID failed")
