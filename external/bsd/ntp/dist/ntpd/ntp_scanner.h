/*	$NetBSD: ntp_scanner.h,v 1.1.1.1.6.1 2012/04/17 00:03:47 yamt Exp $	*/

/* ntp_scanner.h
 *
 * The header file for a simple lexical analyzer. 
 *
 * Written By:	Sachin Kamboj
 *		University of Delaware
 *		Newark, DE 19711
 * Copyright (c) 2006
 */

#ifndef NTP_SCANNER_H
#define NTP_SCANNER_H

/*
 * ntp.conf syntax is slightly irregular in that some tokens such as
 * hostnames do not require quoting even if they might otherwise be
 * recognized as T_ terminal tokens.  This hand-crafted lexical scanner
 * uses a "followed by" value associated with each keyword to indicate
 * normal scanning of the next token, forced scanning of the next token
 * alone as a T_String, or forced scanning of all tokens to the end of
 * the command as T_String.
 * In the past the identifiers for this functionality ended in _ARG:
 *
 * NO_ARG	->	FOLLBY_TOKEN
 * SINGLE_ARG	->	FOLLBY_STRING
 * MULTIPLE_ARG	->	FOLLBY_STRINGS_TO_EOC
 *
 * Note that some tokens use FOLLBY_TOKEN even though they sometimes
 * are followed by strings.  FOLLBY_STRING is used only when needed to
 * avoid the keyword scanner matching a token where a string is needed.
 *
 * FOLLBY_NON_ACCEPT is an overloading of this field to distinguish
 * non-accepting states (where the state number does not match a T_
 * value).
 */
typedef enum {
	FOLLBY_TOKEN = 0,
	FOLLBY_STRING,
	FOLLBY_STRINGS_TO_EOC,
	FOLLBY_NON_ACCEPTING
} follby;

#define MAXLINE		1024	/* maximum length of line */
#define MAXINCLUDELEVEL	5	/* maximum include file levels */

/* STRUCTURES
 * ----------
 */

/* 
 * Define a structure to hold the FSA for the keywords.
 * The structure is actually a trie.
 *
 * To save space, a single u_int32 encodes four fields, and a fifth
 * (the token completed for terminal states) is implied by the index of
 * the rule within the scan state array, taking advantage of the fact
 * there are more scan states than the highest T_ token number.
 *
 * The lowest 8 bits hold the character the state matches on.
 * Bits 8 and 9 hold the followedby value (0 - 3).  For non-accepting
 *   states (which do not match a completed token) the followedby
 *   value 3 (FOLLBY_NONACCEPTING) denotes that fact.  For accepting
 *   states, values 0 - 2 control whether the scanner forces the
 *   following token(s) to strings.
 * Bits 10 through 20 hold the next state to check not matching
 * this state's character.
 * Bits 21 through 31 hold the next state to check matching the char.
 */

#define S_ST(ch, fb, match_n, other_n) (			\
	(u_char)((ch) & 0xff) |					\
	((u_int32)(fb) << 8) |					\
	((u_int32)(match_n) << 10) |				\
	((u_int32)(other_n) << 21)				\
)

#define SS_CH(ss)	((char)(u_char)((ss) & 0xff))
#define SS_FB(ss)	(((u_int)(ss) >>  8) & 0x3)
#define SS_MATCH_N(ss)	(((u_int)(ss) >> 10) & 0x7ff)
#define SS_OTHER_N(ss)	(((u_int)(ss) >> 21) & 0x7ff)

typedef u_int32 scan_state;


/* Structure to hold a filename, file pointer and positional info */
struct FILE_INFO {
	const char *	fname;			/* Path to the file */
	FILE *		fd;			/* File Descriptor */
	int		line_no;		/* Line Number */
	int		col_no;			/* Column Number */
	int		prev_line_col_no;	/* Col No on the 
						   previous line when a
						   '\n' was seen */
	int		prev_token_line_no;	/* Line at start of
						   token */
	int		prev_token_col_no;	/* Col No at start of
						   token */
	int		err_line_no;
	int		err_col_no;
};


/* SCANNER GLOBAL VARIABLES 
 * ------------------------
 */
extern struct config_tree cfgt;	  /* Parser output stored here */
extern int curr_include_level;    /* The current include level */

extern struct FILE_INFO *ip_file; /* Pointer to the configuration file stream */

/* VARIOUS EXTERNAL DECLARATIONS
 * -----------------------------
 */
extern int old_config_style;
extern int input_from_file;
extern struct FILE_INFO *fp[];

/* VARIOUS SUBROUTINE DECLARATIONS
 * -------------------------------
 */
extern const char *keyword(int token);
extern char *quote_if_needed(char *str);
int yylex(void);

struct FILE_INFO *F_OPEN(const char *path, const char *mode);
int FGETC(struct FILE_INFO *stream);
int UNGETC(int ch, struct FILE_INFO *stream);
int FCLOSE(struct FILE_INFO *stream);

void push_back_char(int ch);

#endif	/* NTP_SCANNER_H */
