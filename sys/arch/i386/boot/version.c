/*
 *	$Id: version.c,v 1.17 1994/02/03 23:22:55 mycroft Exp $
 */

/*
 *	NOTE ANY CHANGES YOU MAKE TO THE BOOTBLOCKS HERE.
 *
 *	1.16 -> 1.17
 *		removed with prejudice the extra buffer for xread(), changes
 *		to make the code smaller, and general cleanup. (mycroft)
 *
 *	1.15 -> 1.16
 *		reduce BUFSIZE to 4k, because that's fixed the
 *		boot problems, for some. (cgd)
 *
 *	1.14 -> 1.15
 *		seperated 'version' out from boot.c (cgd)
 *
 *	1.1 -> 1.14
 *		look in boot.c revision logs
 */

char *version = "$Revision: 1.17 $";
