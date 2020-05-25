/*	$NetBSD: ntp_lineedit.h,v 1.1.1.7 2020/05/25 20:40:05 christos Exp $	*/


/*
 * ntp_lineedit.h - generic interface to various line editing libs
 */

int		ntp_readline_init(const char *prompt);
void		ntp_readline_uninit(void);

/*
 * strings returned by ntp_readline go home to free()
 */
char *		ntp_readline(int *pcount);

