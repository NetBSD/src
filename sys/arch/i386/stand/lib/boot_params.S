/*	$NetBSD: boot_params.S,v 1.4.78.1 2010/03/11 15:02:31 yamt Exp $	*/

/* Default boot parameters - must match struct x86_boot_params in bootblock.h */

#ifdef BOOTPARAM_DEFFLAGS
	.long	BOOTPARAM_DEFFLAGS
#else
	.long	0x0
#endif
	.long	5			/* timeout in seconds */
	.long	0			/* console device 0 => CONSDEV_PC */
	.long	9600			/* serial baud rate */
	.space	16			/* md5 boot password */
	.space	64			/* keyboard xlat map */
	.long	0			/* console ioaddr */
