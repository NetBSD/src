/*	$NetBSD: boot_params.S,v 1.2.4.2 2004/08/03 10:36:19 skrll Exp $	*/

/* Default boot parameters */

	.long	0			/* flags */
	.long	5			/* timeout in seconds */
	.long	0			/* console device 0 => CONSDEV_PC */
	.long	9600			/* serial baud rate */
	.space	16			/* md5 boot password */
	.space	16			/* keyboard xlat map */
