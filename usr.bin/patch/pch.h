/*	$NetBSD: pch.h,v 1.6 2002/03/11 18:47:51 kristerw Exp $	*/

EXT FILE *pfp INIT(NULL);		/* patch file pointer */

void re_patch(void);
void open_patch_file(char *);
void set_hunkmax(void);
void grow_hunkmax(void);
bool there_is_another_patch(void);
int intuit_diff_type(void);
void next_intuit_at(long, long);
void skip_to(long, long);
bool another_hunk(void);
char *pgets(char *, int, FILE *);
bool pch_swap(void);
LINENUM pch_first(void);
LINENUM pch_ptrn_lines(void);
LINENUM pch_newfirst(void);
LINENUM pch_repl_lines(void);
LINENUM pch_end(void);
LINENUM pch_context(void);
short pch_line_len(LINENUM);
char pch_char(LINENUM);
char *pfetch(LINENUM);
LINENUM pch_hunk_beg(void);
void do_ed_script(void);
