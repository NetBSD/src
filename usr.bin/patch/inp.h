/*	$NetBSD: inp.h,v 1.8 2003/07/08 01:55:35 kristerw Exp $	*/

EXT LINENUM input_lines INIT(0);	/* how long is input file in lines */
EXT LINENUM last_frozen_line INIT(0);	/* how many input lines have been */
					/* irretractibly output */
void re_input(void);
void scan_input(char *);
const char *ifetch(LINENUM);

