/*	$NetBSD: def.func_tab.h,v 1.5 2001/03/25 20:43:58 jsm Exp $	*/

/*
 * Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 */
#ifndef _DEF_FUNC_TAB_H_
#define _DEF_FUNC_TAB_H_
struct func_tab {
	char f_char;
	int (*f_funct) __P((void));
};

extern const struct func_tab cmdlist[];

struct ext_func_tab {
	const char *ef_txt;
	int (*ef_funct) __P((void));
};

extern const struct ext_func_tab extcmdlist[];
#endif /* _DEF_FUNC_TAB_H_ */
