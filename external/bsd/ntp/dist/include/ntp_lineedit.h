/*	$NetBSD: ntp_lineedit.h,v 1.1.1.1.8.1 2014/12/25 02:34:32 snj Exp $	*/


/*
 * ntp_lineedit.h - generic interface to various line editing libs
 */

int		ntp_readline_init(const char *prompt);
void		ntp_readline_uninit(void);

/*
 * strings returned by ntp_readline go home to free()
 */
char *		ntp_readline(int *pcount);

