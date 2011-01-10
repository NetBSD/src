/*        $NetBSD: tcap.h,v 1.1.6.2 2011/01/10 00:38:23 riz Exp $      */

/*****************************************************************
**
**	tcap.h	-- termcap color capabilities
**
**	(c) Mar 2010 by hoz
**
*****************************************************************/

#ifndef TCAP_H
# define TCAP_H

typedef	enum	{
	TC_BLACK = 0,
	TC_RED,
	TC_GREEN,
	TC_YELLOW,
	TC_BLUE,
	TC_MAGENTA,
	TC_CYAN,
	TC_WHITE,

	TC_BOLD = 0x100,
	TC_ITALIC = 0x200
} tc_att_t;

extern	int	tc_init (FILE *fp, const char *term);
extern	int	tc_end (FILE *fp, const char *term);
extern	int	tc_attr (FILE *fp, tc_att_t attr, int on);
#endif
