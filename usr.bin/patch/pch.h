/*	$NetBSD: pch.h,v 1.4 1998/02/22 13:33:50 christos Exp $	*/

EXT FILE *pfp INIT(Nullfp);		/* patch file pointer */

void re_patch __P((void));
void open_patch_file __P((char *));
void set_hunkmax __P((void));
void grow_hunkmax __P((void));
bool there_is_another_patch __P((void));
int intuit_diff_type __P((void));
void next_intuit_at __P((long, long));
void skip_to __P((long, long));
bool another_hunk __P((void));
char *pgets __P((char *, int, FILE *));
bool pch_swap __P((void));
LINENUM pch_first __P((void));
LINENUM pch_ptrn_lines __P((void));
LINENUM pch_newfirst __P((void));
LINENUM pch_repl_lines __P((void));
LINENUM pch_end __P((void));
LINENUM pch_context __P((void));
short pch_line_len __P((LINENUM));
char pch_char __P((LINENUM));
char *pfetch __P((LINENUM));
LINENUM pch_hunk_beg __P((void));
void do_ed_script __P((void));
