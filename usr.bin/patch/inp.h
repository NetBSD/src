/*	$NetBSD: inp.h,v 1.4 1998/02/22 13:33:49 christos Exp $	*/

EXT LINENUM input_lines INIT(0);	/* how long is input file in lines */
EXT LINENUM last_frozen_line INIT(0);	/* how many input lines have been */
					/* irretractibly output */
void re_input __P((void));
void scan_input __P((char *));
bool plan_a __P((char *));
void plan_b __P((char *));
char *ifetch __P((Reg1 LINENUM, int));
bool rev_in_string __P((char *));
