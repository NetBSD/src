/*
 * Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985.
 *
 *	$Id: def.func_tab.h,v 1.2 1993/08/02 17:16:43 mycroft Exp $
 */

struct func_tab {
	char f_char;
	int (*f_funct)();
};

extern struct func_tab cmdlist[];

struct ext_func_tab {
	char *ef_txt;
	int (*ef_funct)();
};

extern struct ext_func_tab extcmdlist[];
