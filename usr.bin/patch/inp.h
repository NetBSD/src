/* $Header: /cvsroot/src/usr.bin/patch/inp.h,v 1.1 1993/04/09 11:34:01 cgd Exp $
 *
 * $Log: inp.h,v $
 * Revision 1.1  1993/04/09 11:34:01  cgd
 * patch 2.0.12u8, from prep.ai.mit.edu.  this is not under the GPL.
 *
 * Revision 2.0  86/09/17  15:37:25  lwall
 * Baseline for netwide release.
 * 
 */

EXT LINENUM input_lines INIT(0);	/* how long is input file in lines */
EXT LINENUM last_frozen_line INIT(0);	/* how many input lines have been */
					/* irretractibly output */

bool rev_in_string();
void scan_input();
bool plan_a();			/* returns false if insufficient memory */
void plan_b();
char *ifetch();

