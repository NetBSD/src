/* $NetBSD: lk201var.h,v 1.1 1998/09/17 20:01:57 drochner Exp $ */

struct lk201_state {
#define LK_KLL 8
	int down_keys_list[LK_KLL];
};

void lk201_init_keystate __P((struct lk201_state *));
int lk201_decode __P((struct lk201_state *, int, u_int *, int *));
