/*	$NetBSD: msg.md.pl,v 1.1.6.2 2014/08/20 00:05:16 tls Exp $	*/
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
 * 3. The name of Piermont Information Systems Inc. may not be used to endorse
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
{
}

message md_may_remove_boot_medium
{
}

message Keyboard_type {Keyboard type}
message kb_default {pl}

message dobad144
{Instalowanie tablicy zlych blokow ...
}

message getboottype
{Czy chcesz zainstalowac normalne bootbloki, czy te do uzycia z zewn. konsola?

Selected bootblock: }

message console_PC {Konsola BIOS}
message console_com {Port szeregowy com%d, %d bodow}
message console_unchanged {Bez zmian}

message Bootblocks_selection
{Wybor bootblokow}
message Use_normal_bootblocks	{Uzyj konsoli BIOS}
message Use_serial_com0		{Uzyj portu szeregowego com0}
message Use_serial_com1		{Uzyj portu szeregowego com1}
message Use_serial_com2		{Uzyj portu szeregowego com2}
message Use_serial_com3		{Uzyj portu szeregowego com3}
message serial_baud_rate	{Liczba bodow}
message Use_existing_bootblocks	{Uzyj istniejacych bootblokow}

message dobootblks
{Instalowanie bootblokow na %s....
}

message onebiosmatch
{Ten dysk odpowiada ponizszemu dyskowi BIOS:

}

message onebiosmatch_header
{BIOS # cylindry glowice sektory razem sektory  GB
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

message biosmultmatch_header
{   BIOS # cylindry glowice sektory razem sektory  GB
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

message missing_bootmenu_text
{Masz wiecej niz jeden system operacyjny na tym dysku, ale nie
zostalo wskazane menu ani dla aktywnej partycji ani dla partycji
NetBSD, ktora bedzie zainstalowana.

Chcesz zmienic partycje, aby dodac wpis do menu?}

message no_extended_bootmenu
{Wskazane zostalo, ze w menu zostanie zawarta partycja rozszerzona.
Nie wyglada na to, ze system BIOS wspiera polecenie odczytu uzywane
przez te wersje kodu menu.
Czy na pewno chcesz tak zrobic?

(Odpowiedz 'nie' przeniesie z powrotem do menu zmiany partycji.)}

message installbootsel
{Twoje konfiguracja wymaga kodu rozruchowego NetBSD, aby
wybierac, ktorego systemu operacyjnego uzywac.

Nie jest on obecnie zainstalowany. Czy zainstalowac go teraz?}

message installmbr
{Kod rozruchowy w MBR nie wyglada na poprawny.

Chcesz zainstalowac kod rozruchowy NetBSD?}

message updatembr
{Chcesz uaktualnic kod rozruchowy w MBR do najnowszej wersji
kodu rozruchowego NetBSD?}

message set_kernel_1	{Kernel (GENERIC)}
