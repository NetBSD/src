/*
 * This file shouldn't exist; it contains all undefines that currently is.
 * (even undefined pointers, bleh!) It is actually created from an awk
 * script taking input from ld :)
 * Things are taken away as soon they are implemented.
 */
boot (){conout(" boot \n");asm("halt");}
/*
cnopen(){conout(" cnopen \n");asm("halt");}
cnclose(){conout(" cnclose \n");asm("halt");}
cnread(){conout(" cnread \n");asm("halt");}
cnwrite(){conout(" cnwrite \n");asm("halt");}
cnioctl(){conout(" cnioctl \n");asm("halt");}
*/
cpu_exit(){conout(" cpu_exit \n");asm("halt");}
fubyte (){conout(" fubyte \n");asm("halt");}
fuibyte (){conout(" fuibyte \n");asm("halt");}
fuswintr(){conout("fuswintr\n");asm("halt");}
mmrw(){conout(" mmrw\n");asm("halt");}
need_proftick(){conout("need_proftick\n");asm("halt");}
pmap_phys_address(){conout(" pmap_phys_address\n");asm("halt");}
process_set_pc (){conout(" process_set_pc \n");asm("halt");}
process_sstep (){conout(" process_sstep \n");asm("halt");}
remrq (){conout(" remrq \n");asm("halt");}
resuba (){conout(" resuba \n");asm("halt");} /* When to use this??? */
sendsig (){conout(" sendsig \n");asm("halt");}
sigreturn (){conout(" sigreturn \n");asm("halt");}
subyte (){conout(" subyte \n");asm("halt");}
suibyte (){conout(" suibyte \n");asm("halt");}
suswintr(){conout("suswintr\n");asm("halt");}
syopen(){conout(" syopen \n");asm("halt");}
syread(){conout(" syread \n");asm("halt");}
sywrite(){conout(" sywrite \n");asm("halt");}
syioctl(){conout(" syioctl \n");asm("halt");}
syselect(){conout(" syselect \n");asm("halt");}
/*
tuopen(){conout("tuopen\n");asm("halt");}
tuclose(){conout("tuclose\n");asm("halt");}
tustrategy(){conout("tustrategy\n");asm("halt");}
*/
vmapbuf (){conout(" vmapbuf \n");asm("halt");}
vunmapbuf (){conout(" vunmapbuf \n");asm("halt");}
