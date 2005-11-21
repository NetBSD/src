/*	$NetBSD: policy_parse.h,v 1.2 2005/11/21 14:20:28 manu Exp $	*/

typedef union {
	u_int num;
	u_int32_t num32;
	struct _val {
		int len;
		char *buf;
	} val;
} YYSTYPE;
#define	DIR	257
#define	PRIORITY	258
#define	PLUS	259
#define	PRIO_BASE	260
#define	PRIO_OFFSET	261
#define	ACTION	262
#define	PROTOCOL	263
#define	MODE	264
#define	LEVEL	265
#define	LEVEL_SPECIFY	266
#define	IPADDRESS	267
#define	PORT	268
#define	ME	269
#define	ANY	270
#define	SLASH	271
#define	HYPHEN	272


extern YYSTYPE __libipseclval;
