/*	$NetBSD: d_cast_init2.c,v 1.3 2021/01/31 14:57:28 rillig Exp $	*/
# 3 "d_cast_init2.c"

/* cast initialization as the rhs of a - operand */
struct sockaddr_dl {
	char sdl_data[2];
};

int npdl_datasize = sizeof(struct sockaddr_dl) -
		    ((int)((unsigned long)
			&((struct sockaddr_dl *)0)->sdl_data[0]));
