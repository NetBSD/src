/*	$NetBSD: skit.c,v 1.5 1994/11/25 19:10:00 ragge Exp $	*/

/*
 * This file shouldn't exist; it contains all undefines that currently is.
 * (even undefined pointers, bleh!) It is actually created from an awk
 * script taking input from ld :)
 * Things are taken away as soon they are implemented.
 */
cpu_coredump(){conout(" cpu_coredump \n");asm("halt");
cpu_swapin(){conout(" cpu_swapin \n");asm("halt");}
ifubareset(){conout(" ifubareset \n");asm("halt");}
chrtoblk(){conout(" chrtoblk \n");asm("halt");}
isdisk(){conout(" isdisk \n");asm("halt");}
iskmemdev(){conout(" iskmemdev \n");asm("halt");}
fuibyte (){conout(" fuibyte \n");asm("halt");}
fuswintr(){conout("fuswintr\n");asm("halt");}
iszerodev(){conout("iszerodev\n");asm("halt");}
need_proftick(){conout("need_proftick\n");asm("halt");}
pmap_phys_address(){conout(" pmap_phys_address\n");asm("halt");}
process_set_pc (){conout(" process_set_pc \n");asm("halt");}
process_sstep (){conout(" process_sstep \n");asm("halt");}
resuba (){conout(" resuba \n");asm("halt");} /* When to use this??? */
suibyte (){conout(" suibyte \n");asm("halt");}
suswintr(){conout("suswintr\n");asm("halt");}
