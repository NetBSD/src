/*	$NetBSD: syscn.h,v 1.1.18.2 2017/12/03 11:36:23 jdolecek Exp $	*/

void syscninit(int);
int  syscngetc(dev_t);
void syscnputc(dev_t, int);

