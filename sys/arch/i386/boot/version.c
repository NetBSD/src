/*
 *	$Id: version.c,v 1.21.2.2 1994/07/27 06:29:13 cgd Exp $
 */

/*
 *	NOTE ANY CHANGES YOU MAKE TO THE BOOTBLOCKS HERE.
 *
 *	1.22 -> 1.23, 1.21.2.2
 *		fix problem with empty symbol tables. (mycroft)
 *
 *	1.21 -> 1.22, 1.21.2.1
 *		fix compatibility with pre-4.4 file systems. (mycroft)
 *
 *	1.20 -> 1.21
 *		update for 4.4-Lite file system includes and macros (cgd)
 *
 *	1.19 -> 1.20
 *		display options in std. format, more changes for size (cgd)
 *
 *	1.18 -> 1.19
 *		add a '-r' option, to specify RB_DFLTROOT (cgd)
 *
 *	1.17 -> 1.18
 *		removed some more code we don't need for BDB. (mycroft)
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

char *version = "$Revision: 1.21.2.2 $";
