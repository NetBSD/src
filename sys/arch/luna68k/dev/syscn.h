/*	$NetBSD: syscn.h,v 1.1.2.2 2015/09/22 12:05:45 skrll Exp $	*/

void syscninit(int);
int  syscngetc(dev_t);
void syscnputc(dev_t, int);

