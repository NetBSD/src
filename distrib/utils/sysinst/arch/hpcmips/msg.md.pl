/*	$NetBSD: msg.md.pl,v 1.4 2003/05/18 18:54:08 dsl Exp $	*/
/*	Based on english version: */
/*	NetBSD: msg.md.en,v 1.4 2002/03/23 03:24:34 shin Exp */

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

/* MD Message catalog -- Polish, hpcmips version */

message md_hello
{Jesli uruchomiles komputer z dyskietki, mozesz ja teraz wyciagnac.

}

message wdtype
{Jakim rodzajem dysku jest %s?}

message sectforward
{Czy twoj dysk przesuwa AUTOMATYCZNIE sektory?}

message dlgeom
{Wyglada na to, ze twoj dysk, %s, zostal juz skonfigurowany za pomoca
BSD disklabel i disklabel raportuje, ze geometria jest inna od prawdziwej.
Te dwie geometrie to:

disklabel:		%d cylindrow, %d glowic, %d sektorow 
prawdziwa geometria:	%d cylindrow, %d glowic, %d sektorow 
}

/* the %s's will expand into three character strings */
message dobad144
{Instalowanie tablicy zlych blokow ...
}

message getboottype
{Czy chcesz zainstalowac normalne bootbloki, czy te do uzycia z zewn. konsola?
}

message dobootblks
{Instalowanie bootblokow na %s....
}

message askfsroot1
{Bede pytal o informacje o partycjach.

Najpierw partycja glowna. Masz %d %s wolnego miejsca na dysku.
}

message askfsroot2
{Rozmiar partycji glownej? }

message askfsswap1
{
Nastepnie partycja wymiany. Masz %d %s wolnego miejsca na dysku.
}

message askfsswap2
{Rozmiar partycji wymiany? }

message otherparts
{Nadal masz wolna przestrzen na dysku. Podaj rozmiary i punkty montazu
dla ponizszych partycji.

}

message askfspart1
{Nastepna partycja jest /dev/%s%c. Masz %d %s wolnego miejsca na dysku.
}

message askfspart2
{Rozmiar partycji? }

message cyl1024
{Disklabel (zestaw partycji) ktory skonfigurowales ma glowna partycje, ktora
konczy sie poza 1024 cylindrem BIOS. Aby byc pewnym, ze system bedzie
mogl sie zawsze uruchomic, cala glowna partycja powinna znajdowac sie ponizej
tego ograniczenia. Mozesz ponadto: }

message onebiosmatch
{Ten dysk odpowiada ponizszemu dyskowi BIOS:

}

message onebiosmatch_header
{BIOS # cylindry  glowice sektory
------ ---------- ------- -------
}

message onebiosmatch_row
{%-6x %-10d %-7d %d\n}

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

message installbootsel
{Wyglada na to, ze masz wiecej niz jeden system operacyjny zainstalowany
na dysku. Czy chcesz zainstalowac program pozwalajacy na wybranie, ktory
system ma sie uruchomic kiedy wlaczasz/restartujesz komputer?}

message installmbr
{Poczatek dysku NetBSD lezy poza zakresem, ktory BIOS moze zaadresowac.
Inicjujacy bootcode w MBR musi miec mozliwosc korzystania z rozszerzonego
interfejsu  BIOS aby  uruchomic system z tej partycji.  Czy  chcesz
zainstalowac bootcode NetBSD do MBR aby bylo to mozliwe? Pamietaj, ze
taka operacja nadpisze istniejacy kod w MBR,  np. bootselector.} 

message installnormalmbr
{Wybrales aby nie instalowac bootselectora. Jesli zrobiles to poniewaz
masz juz taki program zainstalowany, nic wiecej nie musisz robic.
Jakkolwiek, jesli nie masz bootselectora, normalny bootcode musi byc
uzyty, aby system mogl sie prawidlowo uruchomic. Czy chcesz uzyc normalnego
bootcode NetBSD?}

message configbootsel
{Skonfiguruj rozne opcje bootselectora. Mozesz zmienic podstawowe wpisy
menu do odpowiednich partycji, ktore sa wyswietlane kiedy system sie
uruchamia. Mozesz takze ustawic opoznienie czasowe oraz domyslny system
do uruchomienia (jesli nic nie wybierzesz przy starcie w bootmenu).\n
}

message bootseltimeout
{Opoznienie bootmenu: %d\n}

message defbootselopt
{Domyslna akcja bootmenu: }

message defbootseloptactive
{uruchom pierwsza aktywna partycje.\n}

message defbootseloptpart
{uruchom partycje %d.\n}

message defbootseloptdisk
{uruchom twardy dysk %d.\n}

message bootselitemname
{Podaj nazwe dla opcji}

message bootseltimeoutval
{Opoznienie w sekundach (0-3600)}

message bootsel_header
{Numer  Typ                             Wpis Menu
------ -------------------------------- ----------
}

message bootsel_row
{%-6d %-32s %s\n}

message emulbackup
{Albo /emul/aout albo /emul w twoim systemie byl symbolicznym linkiem
wskazujacym na niezamontowany system. Zostalo mu dodane rozszerzenie '.old'.
Kiedy juz uruchomisz swoj zaktualizowany system, mozliwe ze bedziesz musial
zajac sie polaczeniem nowo utworzonego /emul/aout ze starym.
}
