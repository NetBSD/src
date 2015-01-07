/*	$NetBSD: ntp_lineedit.h,v 1.2.2.2 2015/01/07 04:45:24 msaitoh Exp $	*/


/*
 * ntp_lineedit.h - generic interface to various line editing libs
 */

int		ntp_readline_init(const char *prompt);
void		ntp_readline_uninit(void);

/*
 * strings returned by ntp_readline go home to free()
 */
char *		ntp_readline(int *pcount);

