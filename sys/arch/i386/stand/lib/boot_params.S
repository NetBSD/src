/*	$NetBSD: boot_params.S,v 1.1 2003/10/09 10:29:39 dsl Exp $	*/

/* Default boot parameters */

	.long	0			/* flags */
	.long	5			/* timeout in seconds */
	.long	0			/* console device 0 => CONSDEV_PC */
	.long	9600			/* serial baud rate */
	.space	16			/* md5 boot password */
