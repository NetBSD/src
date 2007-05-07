/*	$NetBSD: msg.md.pl,v 1.23.2.2 2007/05/07 22:28:09 pavel Exp $	*/
/*	Based on english version: */
/*	NetBSD: msg.md.en,v 1.24 2001/01/27 07:34:39 jmc Exp 	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Piermont Information Systems Inc.
 * 4. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* MD Message catalog -- Polish, i386 version */

message md_hello
{Jesli uruchomiles komputer z dyskietki, mozesz ja teraz wyciagnac.

}

message Keyboard_type {Keyboard type}
message kb_default {pl}

message dobad144
{Instalowanie tablicy zlych blokow ...
}

message getboottype
{Czy chcesz zainstalowac normalne bootbloki, czy te do uzycia z zewn. konsola?

Selected bootblock: }

message console_PC {BIOS console}
message console_com {Serial port com%d, baud rate %d}
message console_unchanged {Unchanged}

message Bootblocks_selection
{Wybor bootblokow}
message Use_normal_bootblocks	{Use BIOS console}
message Use_serial_com0		{Use serial port com0}
message Use_serial_com1		{Use serial port com1}
message Use_serial_com2		{Use serial port com2}
message Use_serial_com3		{Use serial port com3}
message serial_baud_rate	{Serial baud rate}
message Use_existing_bootblocks	{Use existing bootblocks}

message No_Bootcode		{No bootcode for root partition}

message dobootblks
{Instalowanie bootblokow na %s....
}

message onebiosmatch
{Ten dysk odpowiada ponizszemu dyskowi BIOS:

}

message onebiosmatch_header	/* XXX translate total */
{BIOS # cylindry glowice sektory total sektory  GB
------ -------- ------- ------- ------------- ---
}

message onebiosmatch_row
{%#6x %8d %6d %7d %13u %3u\n}

message This_is_the_correct_geometry
{To jest prawidlowa geometria}
message Set_the_geometry_by_hand
{Ustaw geometrie recznie}
message Use_one_of_these_disks
{Uzyj jednego z tych dyskow}

message biosmultmatch
{Ten dysk odpowiada ponizszym dyskom BIOS:

}

message biosmultmatch_header	/* XXX translate total */
{   BIOS # cylindry glowice sektory total sektory  GB
   ------ -------- ------- ------- ------------- ---
}

message biosmultmatch_row
{%-1d: %6x %8d %7d %7d %13u %3u\n}

message biosgeom_advise
{
Notatka: od kiedy sysinst jest w stanie unikalnie rozpoznac dysk, ktory
wybrales i powiazac go z dyskiem BIOS, wartosci wyswietlane powyzej sa
bardzo prawdopodobnie prawidlowe i nie powinny byc zmieniane. Zmieniaj je
tylko wtedy jesli sa naprawde _obrzydliwie_ zle.
}

message pickdisk
{Wybierz dysk: }

message partabovechs
{Czesc NetBSD dysku lezy poza obszarem, ktory BIOS w twojej maszynie moze
zaadresowac. Nie mozliwe bedzie bootowanie z tego dysku. Jestes pewien, ze
chcesz to zrobic?

(Odpowiedz 'nie' zabierze cie spowrotem do menu edycji partycji.)}

message missing_bootmenu_text	/* XXX translate */
{You have more than one operating system on this disk, but have not
specified a 'bootmenu' for either the active partition or the
NetBSD partition that you are going to install into. 

Do you want to re-edit the partition to add a bootmenu entry?}

message no_extended_bootmenu	/* XXX translate */
{You have requested that an extended partition be included in the bootmenu.
However your system BIOS doesn't appear to support the read command used
by that version of the bootmenu code. 
Are you sure you that you want to do this?

(Answering 'no' will take you back to the partition edit menu.)}

message installbootsel	/* XXX translate */
{Your configuration requires the NetBSD bootselect code to
select which operating system to use. 

It is not currently installed, do you want to install it now?}

message installmbr	/* XXX translate */
{The bootcode in the Master Boot Record does not appear to be valid.

Do you want to install the NetBSD bootcode?}

message updatembr	/* XXX translate */
{Do you want to update the bootcode in the Master Boot Record to
the latest version of the NetBSD bootcode?}

message set_kernel_1	{Kernel (GENERIC)}
message set_kernel_2	{Kernel (GENERIC.MP)}
message set_kernel_3	{Kernel (GENERIC_LAPTOP)}
message set_kernel_4	{Kernel (GENERIC_DIAGNOSTIC)}
/* message set_kernel_5	{Kernel (GENERIC_TINY)} */
/* message set_kernel_6	{Kernel (GENERIC_PS2TINY)} */
