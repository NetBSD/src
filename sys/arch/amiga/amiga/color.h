/* some colors, handy for debugging 
 *
 *	$Id: color.h,v 1.6 1994/06/04 11:58:43 chopps Exp $
 */
#ifdef DEBUG
#define COL_BLACK	0x000
#define COL_DARK_GRAY	0x444
#define COL_MID_GRAY	0x888
#define COL_LITE_GRAY	0xbbb
#define COL_WHITE	0xfff

#define COL_BLUE	0x00f
#define COL_GREEN	0x0f0
#define COL_RED		0xf00
#define COL_CYAN	0x0ff
#define COL_YELLOW	0xff0
#define COL_MAGENTA	0xf0f

#define COL_LITE_BLUE	0x0af
#define COL_LITE_GREEN	0x0fa
#define COL_ORANGE	0xfa0
#define COL_PURPLE	0xf0a

#define COL24_BLACK	0x0
#define COL24_DGREY	0x0009
#define COL24_LGREY	0x0880
#define COL24_WHITE	0xffff

void rollcolor __P((int));
#endif
