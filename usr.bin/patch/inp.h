/*	$NetBSD: inp.h,v 1.6 2002/03/16 22:36:42 kristerw Exp $	*/

EXT LINENUM input_lines INIT(0);	/* how long is input file in lines */
EXT LINENUM last_frozen_line INIT(0);	/* how many input lines have been */
					/* irretractibly output */
void re_input(void);
void scan_input(char *);
char *ifetch(LINENUM, int);

