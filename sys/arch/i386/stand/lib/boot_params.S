/*	$NetBSD: boot_params.S,v 1.2 2004/03/13 22:41:37 dsl Exp $	*/

/* Default boot parameters */

	.long	0			/* flags */
	.long	5			/* timeout in seconds */
	.long	0			/* console device 0 => CONSDEV_PC */
	.long	9600			/* serial baud rate */
	.space	16			/* md5 boot password */
	.space	16			/* keyboard xlat map */
