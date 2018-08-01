/*	$NetBSD: conv.h,v 1.4 2018/08/01 02:48:47 rin Exp $	*/

/*
 * We ensure that every wide char occupies at least one display width.
 * See vs_line.c for more details.
 */
#define WIDE_COL(sp, ch)						\
	(CHAR_WIDTH(sp, ch) > 0 ? CHAR_WIDTH(sp, ch) : 1)

#define KEY_COL(sp, ch)							\
	(INTISWIDE(ch) ? (size_t)WIDE_COL(sp, ch) : KEY_LEN(sp, ch))

struct _conv_win {
    void    *bp1;
    size_t   blen1;
};

typedef int (*char2wchar_t) 
    (SCR *, const char *, ssize_t, struct _conv_win *, size_t *, const CHAR_T **);
typedef int (*wchar2char_t) 
    (SCR *, const CHAR_T *, ssize_t, struct _conv_win *, size_t *, const char **);

struct _conv {
	char2wchar_t	sys2int;
	wchar2char_t	int2sys;
	char2wchar_t	file2int;
	wchar2char_t	int2file;
	char2wchar_t	input2int;
	wchar2char_t	int2disp;
};
void conv_init __P((SCR *, SCR *));
int conv_enc __P((SCR *, int, const char *));
