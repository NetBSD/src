/*	$NetBSD: inp.h,v 1.5 2002/03/08 21:57:33 kristerw Exp $	*/

EXT LINENUM input_lines INIT(0);	/* how long is input file in lines */
EXT LINENUM last_frozen_line INIT(0);	/* how many input lines have been */
					/* irretractibly output */
void re_input(void);
void scan_input(char *);
bool plan_a(char *);
void plan_b(char *);
char *ifetch(LINENUM, int);
bool rev_in_string(char *);
