/*	$NetBSD: username.h,v 1.2 1999/04/12 06:02:26 ross Exp $	*/

void init_hash __P((void));
char *username __P((int));
int userid __P((char *));
int enter_user __P((int, char *, int));
int get_user __P((int));
