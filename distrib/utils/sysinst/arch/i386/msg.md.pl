/*	$NetBSD: msg.md.pl,v 1.10 2003/06/12 10:51:41 dsl Exp $	*/
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

message Select_type
{Wybierz typ}

message Choose_an_option
{Wybierz opcje}
message Use_real_geometry
{Uzyj prawdziwej geometrii}
message Use_disklabel_geometry
{Uzyj geometrii disklabel}

message dobad144
{Instalowanie tablicy zlych blokow ...
}

message getboottype
{Czy chcesz zainstalowac normalne bootbloki, czy te do uzycia z zewn. konsola?
}

message Bootblocks_selection
{Wybor bootblokow}
message Use_normal_bootblocks
{Uzyj normalnych bootblokow}
message Use_serial_9600_bootblocks
{Uzyj bootblokow na zewn. konsole (9600)}
message Use_serial_38400_bootblocks
{Uzyj bootblokow na zewn. konsole (38400)}
message Use_serial_57600_bootblocks
{Uzyj bootblokow na zewn. konsole (57600)}
message Use_serial_115200_bootblocks
{Uzyj bootblokow na zewn. konsole (115200)}

message dobootblks
{Instalowanie bootblokow na %s....
}

message Reedit_both_MBR_and_label
{Zmien MBR i disklabel}
message Reedit_the_label
{Zmien disklabel}
message Use_it_anyway
{Uzyj, mimo to}

message onebiosmatch
{Ten dysk odpowiada ponizszemu dyskowi BIOS:

}

message onebiosmatch_header
{BIOS # cylindry  glowice sektory
------ ---------- ------- -------
}

message onebiosmatch_row
{%-6x %-10d %-7d %d\n}

message This_is_the_correct_geometry
{To jest prawidlowa geometria}
message Set_the_geometry_by_hand
{Ustaw geometrie recznie}
message Use_one_of_these_disks
{Uzyj jednego z tych dyskow}

message biosmultmatch
{Ten dysk odpowiada ponizszym dyskom BIOS:

}

message biosmultmatch_header
{   BIOS # cylindry  glowice sektory
   ------ ---------- ------- -------
}

message biosgeom_advise
{
Notatka: od kiedy sysinst jest w stanie unikalnie rozpoznac dysk, ktory
wybrales i powiazac go z dyskiem BIOS, wartosci wyswietlane powyzej sa
bardzo prawdopodobnie prawidlowe i nie powinny byc zmieniane. Zmieniaj je
tylko wtedy jesli sa naprawde _obrzydliwie_ zle.
}

message biosmultmatch_row
{%-1d: %-6x %-10d %-7d %d\n}

message pickdisk
{Wybierz dysk: }

message partabovechs
{Czesc NetBSD dysku lezy poza obszarem, ktory BIOS w twojej maszynie moze
zaadresowac. Nie mozliwe bedzie bootowanie z tego dysku. Jestes pewien, ze
chcesz to zrobic?

(Odpowiedz 'nie' zabierze cie spowrotem do menu edycji partycji.)}

message emulbackup
{Albo /emul/aout albo /emul w twoim systemie byl symbolicznym linkiem
wskazujacym na niezamontowany system. Zostalo mu dodane rozszerzenie '.old'.
Kiedy juz uruchomisz swoj zaktualizowany system, mozliwe ze bedziesz musial
zajac sie polaczeniem nowo utworzonego /emul/aout ze starym.
}

message set_kernel_1
{Kernel (GENERIC)}
message set_kernel_2
{Kernel (GENERIC_TINY)}
message set_kernel_3
{Kernel (GENERIC_LAPTOP)}
message set_kernel_4
{Kernel (GENERIC_DIAGNOSTIC)}
message set_kernel_5
{Kernel (GENERIC_PS2TINY)}
message set_kernel_6 
{Kernel (GENERIC.MP)}

message set_md_1        
{GNU toochain}          /* for amd64 */

