/*	$NetBSD: def.func_tab.h,v 1.4 1997/10/19 16:56:58 christos Exp $	*/

/*
 * Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 */
#ifndef _DEF_FUNC_TAB_H_
#define _DEF_FUNC_TAB_H_
struct func_tab {
	char f_char;
	int (*f_funct) __P((void));
};

extern struct func_tab cmdlist[];

struct ext_func_tab {
	char *ef_txt;
	int (*ef_funct) __P((void));
};

extern struct ext_func_tab extcmdlist[];
#endif /* _DEF_FUNC_TAB_H_ */
