/*	$NetBSD: console.h,v 1.1 2002/10/05 17:16:36 chs Exp $	*/

struct tty *find_tp(dev_t);
struct vconsole *vconsole_spawn_re(dev_t, struct vconsole *);

void console_switch(u_int);
int console_unblank(void);
int console_scrollback(void);
int console_scrollforward(void);
