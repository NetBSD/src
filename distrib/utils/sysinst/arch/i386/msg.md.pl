/*	$NetBSD: msg.md.pl,v 1.6 2003/06/03 11:54:53 dsl Exp $	*/
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

message wdtype
{Jakim rodzajem dysku jest %s?}

message Select_type
{Wybierz typ}

message sectforward
{Czy twoj dysk przesuwa AUTOMATYCZNIE sektory?}

message dlgeom
{Wyglada na to, ze twoj dysk, %s, zostal juz skonfigurowany za pomoca
BSD disklabel i disklabel raportuje, ze geometria jest inna od prawdziwej.
Te dwie geometrie to:

disklabel:		%d cylindrow, %d glowic, %d sektorow
prawdziwa geometria:	%d cylindrow, %d glowic, %d sektorow
}

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

message askfsroot
{Bede pytal o informacje o partycjach.

Najpierw partycja glowna. Masz %d %s wolnego miejsca na dysku.
Rozmiar partycji glownej? }

message askfsswap
{
Nastepnie partycja wymiany. Masz %d %s wolnego miejsca na dysku.
Rozmiar partycji wymiany? }

message askfsusr
{
Nastepnie partycja /usr. Masz %d %s wolnego miejsca na dysku.
Rozmiar partycji /usr? }

message otherparts
{Nadal masz wolna przestrzen na dysku. Podaj rozmiary i punkty montazu
dla ponizszych partycji.

}

message askfspart
{Nastepna partycja jest /dev/%s%c. Masz %d %s wolnego miejsca na dysku.
Rozmiar partycji? }

message cyl1024
{Disklabel (zestaw partycji) ktory skonfigurowales ma glowna partycje, ktora
konczy sie poza 1024 cylindrem BIOS. Aby byc pewnym, ze system bedzie
mogl sie zawsze uruchomic, cala glowna partycja powinna znajdowac sie ponizej
tego ograniczenia. Mozesz ponadto: }

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

message wmbrfail
{Nadpisanie MBR nie powiodlo sie. Nie moge kontynuowac.}

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

message Change_a
{Zmien a}
message Change_b
{Zmien b}
message NetBSD_partition_cant_change
{partycja NetBSD - nie mozna zmienic}
message Whole_disk_cant_change
{Caly dysk - nie mozna zmienic}
message Change_e
{Zmien e}
message Change_f
{Zmien f}
message Change_g
{Zmien g}
message Change_h
{Zmien h}
message Change_i
{Zmien i}
message Change_j
{Zmien j}
message Change_k
{Zmien k}
message Change_l
{Zmien l}
message Change_m
{Zmien m}
message Change_n
{Zmien n}
message Change_o
{Zmien o}
message Change_p
{Zmien p}
message Set_new_allocation_size
{Ustaw nowy przydzial rozmiarow}

message Selection_toggles_inclusion
{Wybierz}
message Kernel_GENERIC
{Kernel (GENERIC)}
message Kernel_GENERIC_TINY
{Kernel (GENERIC_TINY)}
message Kernel_GENERIC_LAPTOP
{Kernel (GENERIC_LAPTOP)}
message Kernel_GENERIC_DIAGNOSTIC
{Kernel (GENERIC_DIAGNOSTIC)}
message Kernel_GENERIC_PS2TINY
{Kernel (GENERIC_PS2TINY)}
message Base
{Base}
message System_etc
{System (/etc)}
message Compiler_Tools
{Compiler Tools}
message Games
{Games}
message Online_Manual_Pages
{Online manual pages}
message Miscellaneous
{Miscellaneous}
message Text_Processing_Tools
{Text Processing Tools}
message X11_base_and_clients
{X11 base and clients}
message X11_fonts
{X11 fonts}
message X11_servers
{X11 servers}
message X_contrib_clients
{X contrib clients}
message X11_programming
{X11 programming}
message X11_Misc
{X11 Misc.}

