/*	$NetBSD: boot_params.S,v 1.1 2003/04/16 22:17:44 dsl Exp $	*/

/* Default boot parameters */

	.long	0			/* flags */
	.long	5			/* timeout in seconds */
	.long	0			/* console device 0 => CONSDEV_PC */
	.long	9600			/* serial baud rate */
	.space	16			/* md5 boot password */
