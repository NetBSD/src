/*	$NetBSD: d_cast_init2.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_cast_init2.c"

/* cast initialization as the rhs of a - operand */
struct sockaddr_dl {
	char sdl_data[2];
};

int             npdl_datasize = sizeof(struct sockaddr_dl) -
((int) ((unsigned long)&((struct sockaddr_dl *) 0)->sdl_data[0]));
