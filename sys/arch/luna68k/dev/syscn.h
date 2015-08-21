/*	$NetBSD: syscn.h,v 1.1 2015/08/21 10:48:06 christos Exp $	*/

void syscninit(int);
int  syscngetc(dev_t);
void syscnputc(dev_t, int);

