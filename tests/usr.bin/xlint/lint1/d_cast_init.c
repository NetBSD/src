/*	$NetBSD: d_cast_init.c,v 1.5 2021/07/03 19:34:47 rillig Exp $	*/
# 3 "d_cast_init.c"

/* cast initialization */

typedef unsigned char u_char;

struct sockaddr_x25 {
	u_char x25_len;
	u_char x25_family;
	signed char x25_udata[4];
};

struct sockaddr_x25 x25_dgmask = {
    (unsigned char)(unsigned char)(unsigned int)(unsigned long)
	(&(((struct sockaddr_x25 *)0)->x25_udata[1])),
    0,
    { -1, -1, -1, -1 },
};
