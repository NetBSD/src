#ifndef CURDEF_H
#define CURDEF_H

/*
 * Copyright 1999, Mike Glover
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgment:
 * 	This product includes software developed by Mike Glover
 * 	and contributors.
 * 4. Neither the name of Mike Glover, nor the names of contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MIKE GLOVER AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL MIKE GLOVER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * This header file is for machines without a full curses definition.
 */

#ifdef NOCHTYPE
typedef unsigned long chtype;
#endif

#ifndef	CDK_REFRESH
#define	CDK_REFRESH	''	/* Key used to refresh the screen.	*/
#endif
#ifndef	CDK_PASTE
#define	CDK_PASTE	''	/* Key used for pasting.		*/
#endif
#ifndef	CDK_COPY
#define	CDK_COPY	''	/* Key used for taking copies.		*/
#endif
#ifndef	CDK_ERASE
#define	CDK_ERASE	''	/* Key used for erasing fields.		*/
#endif
#ifndef	CDK_CUT
#define	CDK_CUT		''	/* Key used for cutting.		*/
#endif
#ifndef CDK_BEGOFLINE
#define CDK_BEGOFLINE	''
#endif
#ifndef CDK_ENDOFLINE
#define CDK_ENDOFLINE	''
#endif
#ifndef CDK_BACKCHAR
#define CDK_BACKCHAR	''
#endif
#ifndef CDK_FORCHAR
#define CDK_FORCHAR	''
#endif
#ifndef CDK_TRANSPOSE
#define CDK_TRANSPOSE	''
#endif
#ifndef CDK_NEXT
#define CDK_NEXT	''
#endif
#ifndef CDK_PREV
#define CDK_PREV	''
#endif
#ifndef	SPACE
#define	SPACE		' '
#endif
#ifndef DELETE
#define DELETE		'\177'	/* Delete key				*/
#endif
#ifndef KEY_DC
#define KEY_DC		'\177'	/* Delete key				*/
#endif
#ifndef	TAB
#define	TAB		'\t'	/* Tab key.				*/
#endif
#ifndef	KEY_ESC
#define	KEY_ESC		''	/* Escape Key.				*/
#endif
#ifndef KEY_RETURN
#define KEY_RETURN	'\012'	/* Return key				*/
#endif
#ifndef KEY_CR
#define KEY_CR		'\r'	/* Enter key				*/
#endif
#ifndef KEY_TAB
#define KEY_TAB		'\t'	/* Tab key				*/
#endif
#ifndef KEY_A1
#define KEY_A1		0534	/* Upper left of keypad			*/
#endif
#ifndef KEY_A3
#define KEY_A3		0535	/* Upper right of keypad		*/
#endif
#ifndef KEY_B2
#define KEY_B2		0536	/* Center of keypad			*/
#endif
#ifndef KEY_C1
#define KEY_C1		0537	/* Lower left of keypad			*/
#endif
#ifndef KEY_C3
#define KEY_C3		0540	/* Lower right of keypad		*/
#endif
#ifndef KEY_BTAB
#define KEY_BTAB	0541	/* Back tab key				*/
#endif
#ifndef KEY_BEG
#define KEY_BEG		0542	/* beg(inning) key			*/
#endif
#ifndef KEY_CANCEL
#define KEY_CANCEL	0543	/* cancel key				*/
#endif
#ifndef KEY_CLOSE
#define KEY_CLOSE	0544	/* close key				*/
#endif
#ifndef KEY_COMMAND
#define KEY_COMMAND	0545	/* cmd (command) key			*/
#endif
#ifndef KEY_COPY
#define KEY_COPY	0546	/* copy key				*/
#endif
#ifndef KEY_CREATE
#define KEY_CREATE	0547	/* create key				*/
#endif
#ifndef KEY_END
#define KEY_END		0550	/* end key				*/
#endif
#ifndef KEY_EXIT
#define KEY_EXIT	0551	/* exit key				*/
#endif
#ifndef KEY_FIND
#define KEY_FIND	0552	/* find key				*/
#endif
#ifndef KEY_HELP
#define KEY_HELP	0553	/* help key				*/
#endif
#ifndef KEY_MARK
#define KEY_MARK	0554	/* mark key				*/
#endif
#ifndef KEY_MESSAGE
#define KEY_MESSAGE	0555	/* message key				*/
#endif
#ifndef KEY_MOVE
#define KEY_MOVE	0556	/* move key				*/
#endif
#ifndef KEY_NEXT
#define KEY_NEXT	0557	/* next object key			*/
#endif
#ifndef KEY_OPEN
#define KEY_OPEN	0560	/* open key				*/
#endif
#ifndef KEY_OPTIONS
#define KEY_OPTIONS	0561	/* options key				*/
#endif
#ifndef KEY_PREVIOUS
#define KEY_PREVIOUS	0562	/* previous object key			*/
#endif
#ifndef KEY_REDO
#define KEY_REDO	0563	/* redo key				*/
#endif
#ifndef KEY_REFERENCE
#define KEY_REFERENCE	0564	/* ref(erence) key			*/
#endif
#ifndef KEY_REFRESH
#define KEY_REFRESH	0565	/* refresh key				*/
#endif
#ifndef KEY_REPLACE
#define KEY_REPLACE	0566	/* replace key				*/
#endif
#ifndef KEY_RESTART
#define KEY_RESTART	0567	/* restart key				*/
#endif
#ifndef KEY_RESUME
#define KEY_RESUME	0570	/* resume key				*/
#endif
#ifndef KEY_SAVE
#define KEY_SAVE	0571	/* save key				*/
#endif
#ifndef KEY_SBEG
#define KEY_SBEG	0572	/* shifted beginning key		*/
#endif
#ifndef KEY_SCANCEL
#define KEY_SCANCEL	0573	/* shifted cancel key			*/
#endif
#ifndef KEY_SCOMMAND
#define KEY_SCOMMAND	0574	/* shifted command key			*/
#endif
#ifndef KEY_SCOPY
#define KEY_SCOPY	0575	/* shifted copy key			*/
#endif
#ifndef KEY_SCREATE
#define KEY_SCREATE	0576	/* shifted create key			*/
#endif
#ifndef KEY_SDC
#define KEY_SDC		0577	/* shifted delete char key		*/
#endif
#ifndef KEY_SDL
#define KEY_SDL		0600	/* shifted delete line key		*/
#endif
#ifndef KEY_SELECT
#define KEY_SELECT	0601	/* select key				*/
#endif
#ifndef KEY_SEND
#define KEY_SEND	0602	/* shifted end key			*/
#endif
#ifndef KEY_SEOL
#define KEY_SEOL	0603	/* shifted clear line key		*/
#endif
#ifndef KEY_SEXIT
#define KEY_SEXIT	0604	/* shifted exit key			*/
#endif
#ifndef KEY_SFIND
#define KEY_SFIND	0605	/* shifted find key			*/
#endif
#ifndef KEY_SHELP
#define KEY_SHELP	0606	/* shifted help key			*/
#endif
#ifndef KEY_SHOME
#define KEY_SHOME	0607	/* shifted home key			*/
#endif
#ifndef KEY_SIC
#define KEY_SIC		0610	/* shifted input key			*/
#endif
#ifndef KEY_SLEFT
#define KEY_SLEFT	0611	/* shifted left arrow key		*/
#endif
#ifndef KEY_SMESSAGE
#define KEY_SMESSAGE	0612	/* shifted message key			*/
#endif
#ifndef KEY_SMOVE
#define KEY_SMOVE	0613	/* shifted move key			*/
#endif
#ifndef KEY_SNEXT
#define KEY_SNEXT	0614	/* shifted next key			*/
#endif
#ifndef KEY_SOPTIONS
#define KEY_SOPTIONS	0615	/* shifted options key			*/
#endif
#ifndef KEY_SPREVIOUS
#define KEY_SPREVIOUS	0616	/* shifted prev key			*/
#endif
#ifndef KEY_SPRINT
#define KEY_SPRINT	0617	/* shifted print key			*/
#endif
#ifndef KEY_SREDO
#define KEY_SREDO	0620	/* shifted redo key			*/
#endif
#ifndef KEY_SREPLACE
#define KEY_SREPLACE	0621	/* shifted replace key			*/
#endif
#ifndef KEY_SRIGHT
#define KEY_SRIGHT	0622	/* shifted right arrow			*/
#endif
#ifndef KEY_SRSUME
#define KEY_SRSUME	0623	/* shifted resume key			*/
#endif
#ifndef KEY_SSAVE
#define KEY_SSAVE	0624	/* shifted save key			*/
#endif
#ifndef KEY_SSUSPEND
#define KEY_SSUSPEND	0625	/* shifted suspend key			*/
#endif
#ifndef KEY_SUNDO
#define KEY_SUNDO	0626	/* shifted undo key			*/
#endif
#ifndef KEY_SUSPEND
#define KEY_SUSPEND	0627	/* suspend key				*/
#endif
#ifndef KEY_UNDO
#define KEY_UNDO	0630	/* undo key				*/
#endif
#ifndef KEY_MAX
#define KEY_MAX		0777	/* Maximum curses key			*/
#endif
#ifndef A_ATTRIBUTES
#define A_ATTRIBUTES    0xffffff00
#endif
#ifndef A_NORMAL
#define A_NORMAL        0x00000000
#endif
#ifndef A_STANDOUT
#define A_STANDOUT      0x00010000
#endif
#ifndef A_UNDERLINE
#define A_UNDERLINE     0x00020000
#endif
#ifndef A_REVERSE
#define A_REVERSE       0x00040000
#endif
#ifndef A_BLINK
#define A_BLINK         0x00080000
#endif
#ifndef A_DIM
#define A_DIM           0x00100000
#endif
#ifndef A_BOLD
#define A_BOLD          0x00200000
#endif
#ifndef A_ALTCHARSET
#define A_ALTCHARSET    0x00400000
#endif
#ifndef A_INVIS
#define A_INVIS         0x00800000
#endif
#ifndef A_PROTECT
#define A_PROTECT       0x01000000
#endif
#ifndef A_CHARTEXT
#define A_CHARTEXT      0x000000ff
#endif
#ifndef A_COLOR
#define A_COLOR         0x0000ff00
#endif
#ifndef KEY_MIN
#define KEY_MIN		0401	/* Minimum curses key			*/
#endif
#ifndef KEY_BREAK
#define KEY_BREAK       0401	/* break key (unreliable)		*/
#endif
#ifndef KEY_DOWN
#define KEY_DOWN        0402	/* The four arrow keys ...		*/
#endif
#ifndef KEY_UP
#define KEY_UP          0403
#endif
#ifndef KEY_LEFT
#define KEY_LEFT        0404
#endif
#ifndef KEY_RIGHT
#define KEY_RIGHT       0405	/* ... */
#endif
#ifndef KEY_HOME
#define KEY_HOME        0406	/* Home key (upward+left arrow)		*/
#endif
#ifndef KEY_BACKSPACE
#define KEY_BACKSPACE   0407	/* backspace (unreliable)		*/
#endif
#ifndef KEY_F0
#define KEY_F0          0410	/* Function keys.  Space for 64		*/
#endif
#ifndef KEY_F1
#define KEY_F1          0411
#endif
#ifndef KEY_F2
#define KEY_F2          0412
#endif
#ifndef KEY_F3
#define KEY_F3          0413
#endif
#ifndef KEY_F4
#define KEY_F4          0414
#endif
#ifndef KEY_F5
#define KEY_F5          0415
#endif
#ifndef KEY_F6
#define KEY_F6          0416
#endif
#ifndef KEY_F7
#define KEY_F7          0417
#endif
#ifndef KEY_F8
#define KEY_F8          0420
#endif
#ifndef KEY_F9
#define KEY_F9          0421
#endif
#ifndef KEY_F10
#define KEY_F10		0422
#endif
#ifndef KEY_F11
#define KEY_F11		0423
#endif
#ifndef KEY_F12
#define KEY_F12		0424
#endif
#ifndef KEY_DL
#define KEY_DL          0510	/* Delete line				*/
#endif
#ifndef KEY_IL
#define KEY_IL          0511	/* Insert line				*/
#endif
#ifndef KEY_DC
#define KEY_DC          0512	/* Delete character			*/
#endif
#ifndef KEY_IC
#define KEY_IC          0513	/* Insert char or enter insert mode	*/
#endif
#ifndef KEY_EIC
#define KEY_EIC         0514	/* Exit insert char mode		*/
#endif
#ifndef KEY_CLEAR
#define KEY_CLEAR       0515	/* Clear screen				*/
#endif
#ifndef KEY_EOS
#define KEY_EOS         0516	/* Clear to end of screen		*/
#endif
#ifndef KEY_EOL
#define KEY_EOL         0517	/* Clear to end of line			*/
#endif
#ifndef KEY_SF
#define KEY_SF          0520	/* Scroll 1 line forward		*/
#endif
#ifndef KEY_SR
#define KEY_SR          0521	/* Scroll 1 line backwards (reverse)	*/
#endif
#ifndef KEY_NPAGE
#define KEY_NPAGE       0522	/* Next page				*/
#endif
#ifndef KEY_PPAGE
#define KEY_PPAGE       0523	/* Previous page			*/
#endif
#ifndef KEY_STAB
#define KEY_STAB        0524	/* Set tab				*/
#endif
#ifndef KEY_CTAB
#define KEY_CTAB        0525	/* Clear tab				*/
#endif
#ifndef KEY_CATAB
#define KEY_CATAB       0526	/* Clear all tabs			*/
#endif
#ifndef KEY_ENTER
#define KEY_ENTER       0527	/* Enter or send (unreliable)		*/
#endif
#ifndef KEY_SRESET
#define KEY_SRESET      0530	/* soft (partial) reset (unreliable)	*/
#endif
#ifndef KEY_RESET
#define KEY_RESET       0531	/* reset or hard reset (unreliable)	*/
#endif
#ifndef KEY_PRINT
#define KEY_PRINT       0532	/* print or copy			*/
#endif
#ifndef KEY_LL
#define KEY_LL          0533	/* home down or bottom (lower left)	*/
#endif

/*
 * Some systems (like AIX >:| ) don't have predefined items
 * like ACS_* so I have to 'fake' them here.
*/
#ifndef ACS_HLINE
#define	ACS_HLINE	'-'
#endif
#ifndef ACS_VLINE
#define	ACS_VLINE	'|'
#endif
#ifndef ACS_ULCORNER
#define	ACS_ULCORNER	'+'
#endif
#ifndef ACS_URCORNER
#define	ACS_URCORNER	'+'
#endif
#ifndef ACS_LLCORNER
#define	ACS_LLCORNER	'+'
#endif
#ifndef ACS_LRCORNER
#define	ACS_LRCORNER	'+'
#endif
#ifndef ACS_LTEE
#define	ACS_LTEE	'+'
#endif
#ifndef ACS_RTEE
#define	ACS_RTEE	'+'
#endif
#ifndef ACS_TTEE
#define	ACS_TTEE	'+'
#endif
#ifndef ACS_BTEE
#define	ACS_BTEE	'+'
#endif
#ifndef ACS_PLUS
#define	ACS_PLUS	'+'
#endif
#ifndef ACS_LARROW
#define ACS_LARROW	'<'
#endif
#ifndef ACS_RARROW
#define ACS_RARROW	'>'
#endif
#ifndef ACS_UARROW
#define ACS_UARROW	'^'
#endif
#ifndef ACS_DARROW
#define ACS_DARROW	'v'
#endif
#ifndef ACS_DIAMOND
#define ACS_DIAMOND	'+'
#endif
#ifndef ACS_CKBOARD
#define ACS_CKBOARD	'#'
#endif
#ifndef ACS_DEGREE
#define ACS_DEGREE	'o'
#endif
#ifndef ACS_PLMINUS
#define ACS_PLMINUS	'~'
#endif
#ifndef ACS_BULLET
#define ACS_BULLET	'*'
#endif
#ifndef ACS_S1
#define ACS_S1		'1'
#endif
#ifndef ACS_S2
#define ACS_S2		'2'
#endif
#endif /* CURDEF_H */
