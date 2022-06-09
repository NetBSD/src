/*	$NetBSD: msg.mi.pl,v 1.41 2022/06/09 18:26:06 martin Exp $	*/
/*	Based on english version: */
/*	NetBSD: msg.mi.pl,v 1.36 2004/04/17 18:55:35 atatat Exp       */

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

/* MI Message catalog -- polish, machine independent */

/*
 * We can not use non ascii characters in this message - it is displayed
 * before the locale is set up!
 */
message sysinst_message_language
{Komunikaty instalacyjne w jezyku polskim}

message sysinst_message_locale
{pl_PL.ISO8859-2}

message	out_of_memory	{Zabraklo pamieci!}
message Yes {Tak}
message No {Nie}
message All {Wszystkie}
message Some {Niektore}
message None {Zadne}
message none {zadne}
message ok {ok}
message OK {OK}
message unchanged {niezmienione}
message On {Wlaczone}
message Off {Wylaczone}
message Delete {Usunac?}

message install
{zainstaluj}

message reinstall
{przeinstaluj pakiety dla}

message upgrade
{zaktualizuj}

message mount_failed
{Montowanie %s nie powiodlo sie. Kontynuowac?
}

message hello
{Witaj w sysinst, systemie instalacyjnym NetBSD-@@VERSION@@.
To oparte na menu narzedzie (sysinst) zostalo zaprojektowane w celu
ulatwienia instalacji NetBSD na dysku twardym, lub uaktualnienia istniejacego 
systemu NetBSD przy minimalnym nakladzie pracy. W ponizszych menu, aby wybrac
pozycje wpisz litere (a, b, c, ...). Mozesz tez uzyc CTRL+N/CTRL+P, aby wybrac
nastepna/poprzednia pozycje. Klawisze strzalek i Page-up/Page-down takze moga
dzialac. Aby aktywowac biezacy wybor z menu, nacisnij klawisz Enter.
}

message thanks
{Dziekujemy za wybranie NetBSD!
}

message installusure
{Zdecydowales sie zainstalowac NetBSD na twardym dysku. Spowoduje to zmiane
informacji na twoim dysku. Powinienes zrobic pelny backup danych przed
rozpoczeciem tej procedury!  Zostana wykonane nastepujace czynnosci:
	a) Podzial dysku twardego
	b) Stworzenie nowych systemow plikow BSD
	c) Wgranie i zainstalowanie pakietow dystrybucji

(Po wprowadzeniu informacji o partycjach, ale jeszcze zanim twoj dysk
zostanie zmieniony, bedziesz mial mozliwosc przerwac te procedure.)

Czy kontynuowac?
}

message upgradeusure
{Ok, zaktualizujmy NetBSD na twoim dysku.  Jak zawsze, spowoduje to
zmiane informacji na twoim dysku. Powinienes zrobic pelny backup danych
przed rozpoczeciem!  Czy napewno chcesz zaktualizowac NetBSD?
(Jest to ostatnie ostrzezenie zanim zacznie sie modyfikacja danych na
twoich dyskach.)
}

message reinstallusure
{Ok, rozpakujmy pakiety dystrybucyjne NetBSD na bootowalny twardy dysk.
Ta procedura tylko sciaga i rozpakowuje pakiety na pre-partycjonowany
bootowalny dysk. Nie nazywa dyskow, aktualizuje bootblokow, lub zapisuje
istniejacej konfiguracji.   (Wyjdz i wybierz `instaluj' lub
`aktualizuj' jesli chcesz to zrobic.) Powinienes wykonac `instaluj' lub
`aktualizuj' przed rozpoczeciem tej procedury!

Czy napewno chcesz przeinstalowac pakiety dystrybucjne NetBSD?
(Jest to ostatnie ostrzezenie zanim zacznie sie modyfikacja danych na
twoich dyskach.)
}

message nodisk
{Nie moge znalezc zadnych twardych dyskow do uzycia z NetBSD. Zostaniesz
przeniesiony do menu glownego.
}

message onedisk
{Znalazlem tylko jeden dysk, %s.  Stad przyjmuje, ze chcesz %s
NetBSD wlasnie na nim.
}

message ask_disk
{Na ktorym z nich chcesz %s NetBSD? }

message Available_disks
{Dostepne dyski}

message Available_wedges	{Existing "wedges"}

message heads
{glowice}

message sectors
{sektory}

message mountpoint
{punkt montowania (lub 'zaden')}

message cylname
{cyl}

message secname
{sek}

message megname
{MB}

message gigname
{GB}

/* Called with:				Example
 *  $0 = device name			wd0
 *  $1 = partitioning scheme name	Guid Partition Table
 *  $2 = short version of $1		GPT
 *  $3 = disk size for NetBSD		3TB
 *  $4 = full install size min.		127M
 *  $5 = install with X min.		427M
 */
message	layout_prologue_none
{Mozesz uzyc prostego edytora aby ustawic rozmiary partycji NetBSD,
lub uzyc domyslnych rozmiarow i zawartosci.}

/* Called with:				Example
 *  $0 = device name			wd0
 *  $1 = partitioning scheme name	Guid Partition Table
 *  $2 = short version of $1		GPT
 *  $3 = disk size for NetBSD		3TB
 *  $4 = full install size min.		127M
 *  $5 = install with X min.		427M
 */

message	layout_prologue_existing
{Jezeli nie chcesz uzywac instniejacych partycji, mozesz uzyc
prostego edytora aby ustawic rozmiary partycji NetBSD, lub usunac
istniejace partycje i uzyc domyslnych rozmiarow partycji.}

/* Called with:				Example
 *  $0 = device name			wd0
 *  $1 = partitioning scheme name	Guid Partition Table
 *  $2 = short version of $1		GPT
 *  $3 = disk size for NetBSD		3TB
 *  $4 = full install size min.		127M
 *  $5 = install with X min.		427M
 */
message layout_main
{
Bedziesz mial szanse zmienic wlasciwosci partycji.

Czesc NetBSD (lub wolna) twojego dysku ($0) to $3.

Pelna instalacja wymaga co najmniej $4 bez Xorg, oraz co najmniej $5
jezeli zainstalujesz z pakietami instalacyjnymi Xorg.}

message Choose_your_size_specifier
{Wybranie Mega/Gigabajtow nada partycji rozmiar bliski twojemu wyborowi,
ale dopasowany do granic cylindrow.  Wybranie sektorow pozwoli ci na
bardziej dokladne okreslenie rozmiarow.  Na nowych dyskach ZBR rozmiar
cylindra zmienia sie w zaleznosci od miejsca na dysku, jest wiec
niewielka korzysc z dopasowania cylindrow.  Na starszych dyskach
bardziej wydajne jest podawanie rozmiaru dysku, ktore sa
wielokrotnoscia aktualnego rozmiaru cylindra.

Wybierz specyfikator rozmiaru}

message ptnsizes
{Mozesz teraz zmienic rozmiary partycji systemowych. Domyslne ustawienia
alokuja cala przestrzen na glowny system plikow, aczkolwiek mozesz zdefiniowac
osobne partycje /usr (dodatkowe pliki systemowe), /var (dane systemowe i logi)
lub /home (katalogi domowe uzytkownikow).

Wolna przestrzen zostanie dodana do partycji oznaczonej '+'.}

/* Called with: 			Example
 *  $0 = list of marker explanations	'=' existining, '@' external
 */
message ptnsizes_markers		{Inne znaczniki: partycja $0.}
message ptnsizes_mark_existing		{'=' istniejacy}
message ptnsizes_mark_external		{'@' zewnetrzny}

message ptnheaders_size		{Rozmiar}
message ptnheaders_filesystem	{System plikow}

message askfsmount
{Punkt montowania?}

message askfssize
{Rozmiar dla %s w %s?}

message askunits
{Zmien jednostki wejsciowe (sektory/cylindry/MB/GB)}

message NetBSD_partition_cant_change
{partycja NetBSD}

message Whole_disk_cant_change
{Caly dysk}

message Boot_partition_cant_change
{partycja rozruchowa}

message add_another_ptn
{Dodaj partycje zdefiniowana przez uzytkownika}

/* Called with: 			Example
 *  $0 = free space			1.4
 *  $1 = size unit			GB
 */
message fssizesok
{Dalej.  Wolne miejsce: $0 $1.}

/* Called with: 			Example
 *  $0 = missing space			1.4
 *  $1 = size unit			GB
 */
message fssizesbad
{Anulowano.  Niewystarczajace miejsce, brakuje $0 $1!}

message startoutsidedisk
{Wartosc poczatkowa ktora podales jest poza koncem dysku.}

message endoutsidedisk
{Przy tej wartosci, koniec partycji znajduje sie poza koncem dysku. Rozmiar
twojej partycji zostal zmniejszony.}

message toobigdisklabel
{
Ten dysk ($0) jest zbyt duzy ($3) na tablice partycji $2 (maks. $4),
wiec tylko poczatek dysku jest mozliwy do uzycia.
}

message cvtscheme_hdr		{Co chcesz zrobic z istniejacymi partycjami?}
message cvtscheme_keep		{zostaw (uzyj tylko czesci dysku)}
message cvtscheme_delete	{skasuj (wszystkie dane beda utracone!)}
message cvtscheme_convert	{uzyj innego typu tablicy partycji}
message cvtscheme_abort		{anuluj}

/* Called with:				Example
 *  $0 = device name			wd0
 *  $1 = partitioning scheme name	BSD disklabel
 *  $2 = short version of $1		disklabel
 *  $3 = optional install flag		(I)nstall,
 *  $4 = additional flags description	(B)ootable
 *  $5 = total size			2TB
 *  $6 = free size			244MB
 */
message fspart
{Twoje partycje sa ustawione jako:

Flags: $3(N)ewfs$4.   Total: $5, free: $6}

message ptnheaders_start	{Start}
message ptnheaders_end		{Koniec}
message ptnheaders_fstype	{Typ SP}

message partition_sizes_ok
{Rozmiary partycji w porzadku}

message edfspart
{Powinienes najpierw ustawic rodzaj systemu
plikow (SP). Pozniej inne wartosci.

Aktualne wartosci dla partycji:}

message ptn_newfs		{newfs}
message ptn_mount		{montowanie}
message ptn_mount_options	{opcje montowania}
message ptn_mountpt		{punkt montowania}

message toggle
{Przelacz}

message restore
{Odzyskaj oryginalne wartosci}

message Select_the_type
{Wybierz typ}

message other_types
{inne typy}

/* Called with:				Example
 *  $0 = valid partition shortcuts	a-e
 *  $1 = maximum allowed		4292098047
 *  $2 = size unit			MB
 */
message label_size_head
{Specjalne wartosci, ktore moga byc podane jako wartosci rozmiaru:
    -1:   az do konca czesci dysku}

/* Called with:				Example
 *  $0 = valid partition shortcuts	a-e
 *  $1 = maximum allowed		4292098047
 *  $2 = size unit			MB
 */
message label_size_part_hint
{   $0:   zakoncz ta partycje tam gdzie partycja X sie zaczyna}

/* Called with:				Example
 *  $0 = valid partition shortcuts	a-e
 *  $1 = maximum allowed		4292098047
 *  $2 = size unit			MB
 */
message label_size_tail			{Rozmiar (max $1 $2)}

/* Called with:				Example
 *  $0 = valid partition shortcuts	a-e
 *  $1 = valid free space shortcuts	f-h
 *  $2 = size unit			MB
 */
message label_offset_head
{Specjalne wartosci, ktore moga byc podane jako wartosci przesuniecia:
    -1:   zacznij na poczatku czesci dysku NetBSD}

/* Called with:				Example
 *  $0 = valid partition shortcuts	a-e
 *  $1 = valid free space shortcuts	f-h
 *  $2 = size unit			MB
 */
message label_offset_part_hint
{   $0:   zacznij na koncu partycji X}

/* Called with:				Example
 *  $0 = valid partition shortcuts	a-e
 *  $1 = valid free space shortcuts	f-h
 *  $2 = size unit			MB
 */
message label_offset_space_hint
{   $1:   zacznij na poczatku podanego wolnego fragmentu}

/* Called with:				Example
 *  $0 = valid partition shortcuts	a-e
 *  $1 = valid free space shortcuts	f-h
 *  $2 = size unit			MB
 */
message label_offset_tail		{Poczatek ($2)}

message invalid_sector_number
{Nieprawidlowa liczba}

message packname
{Podaj nazwe dla swojego dysku NetBSD}

message lastchance
{Jestesmy teraz gotowi zainstalowac NetBSD na twoim dysku (%s). Nic
nie zostalo jeszcze zapisane. Masz teraz ostatnia szanse na przerwanie tego
procesu poki nic nie zostalo jeszcze zmienione.

Czy kontynuowac?
}

message disksetupdone
{Pierwsza czesc procedury zostala zakonczona. Sysinst zapisal
disklabel na dysk doceloway, oraz utworzyl system plikow i sprawdzil
nowe partycje, ktore podales jako docelowe.
}

message disksetupdoneupdate
{Pierwsza czesc procedury zostala zakonczona. Sysinst zapisal
disklabel na dysk docelowy, oraz sprawdzil nowe partycje, ktore
podales jako docelowe.
}

message openfail
{Nie moglem otworzyc %s, blad: %s.
}

/* Called with:				Example
 *  $0 = device name			/dev/wd0a
 *  $1 = mount path			/usr
 */
message mountfail
{zamontowanie urzadzenia $0 na $1 nie powiodlo sie.
}

message extractcomplete
{Rozpakowywanie wybranych pakietow dla NetBSD-@@VERSION@@ zakonczone.
System moze sie teraz uruchomic z wybranego twardego dysku. Aby zakonczyc
instalacje, sysinst da ci mozliwosc skonfigurowania kilku istotnych rzeczy.
}

message instcomplete
{Instalacja NetBSD-@@VERSION@@ zostala zakonczona. System powinien
uruchomic sie z twardego dysku. Wykonaj polecenia zawarte w pliku
INSTALL o koncowej konfiguracji systemu.

Powinienes przynajmniej zedytowac /etc/rc.conf aby odpowiadal twoim
potrzebom. Zajrzyj do /etc/defaults/rc.conf aby poznac domyslne wartosci.
}

message upgrcomplete
{Aktualizacja NetBSD-@@VERSION@@ zostala zakonczona. Bedziesz teraz
musial wykonac polecenia zawarte w pliku INSTALL, aby skonfigurowac system
odpowiadajacy twoim potrzebom.

Musisz przynajmniej dostosowac rc.conf do swojego lokalnego srodowiska
i zmienic rc_configured=NO na rc_configured=YES, inaczej start systemu
zatrzyma sie na trybie jednego-uzytkownika, oraz skopiowac z powrotem
pliki z haslami (biorac pod uwage nowe konta systemowe ktore mogly
zostac utworzone dla tej wersji), jesli uzywales lokalnych plikow hasel.
}


message unpackcomplete
{Rozpakowywanie dodatkowych pakietow NetBSD-@@VERSION@@ zostalo zakonczone. 
Musisz teraz wykonac
polecenia zawarte w pliku INSTALL aby przekonfigurowac system do swoich
potrzeb.

Musisz przynajmniej dostosowac rc.conf do swojego lokalnego srodowiska
i zmienic rc_configured=NO na rc_configured=YES inaczej start systemu
zatrzyma sie na trybie jednego-uzytkownika.
}

message distmedium
{Twoj dysk jest teraz gotowy na zainstalowanie jadra oraz pakietow
dystrybucyjnych. Jak napisano w pliku INSTALL masz tez kilka opcji. Dla
ftp lub nfs, musisz byc podlaczony do sieci z dostepem do odpowidnich maszyn.

%d wybranych pakietow, %d zainstalowanych. Nastepnym pakietem jest %s.

}

message distset
{Dystrybucja NetBSD jest rozbita w kolekcje pakietow dystrybucyjnych.
Czesc z nich to pakiety podstawowe wymagane przez wszystkie instalacje,
a czesc nie jest przez wszystkie wymagana. Mozesz zainstalowac je
wszystkie (Pelna instalacja) lub wybrac z opcjonalnych pakietow. Mozesz
takze zainstalowac tylko podstawowy zestaw (minimalna instalacja), lub
wybrac te, ktore chcesz (Inna instalacja)
}

/* Called with: 			Example
 *  $0 = sets suffix			.tgz
 *  $1 = URL protocol used		ftp
 */
message ftpsource
{Ponizej masz site $1, katalog, uzytkownika, oraz haslo gotowe do uzycia.
Jesli "uzytkownik" to "ftp", wtedy haslo nie jest wymagane.

}

message email
{adres e-mail}

message dev
{urzadzenie}

/* Called with: 			Example
 *  $0 = sets suffix			.tgz
 */
message nfssource
{Wprowadz hosta NFS oraz katalog gdzie znajduje sie dystrybucja. 
Pamietaj, ze katalog musi zawierac pliki $0, oraz musi byc
dostepny przez NFS.

}

message floppysource
{Podaj urzadzenie bedace stacja dyskietek oraz katalog pomocniczy
w docelowym systemie plikow. Pliki z pakietami instalacyjnymi musza
znajdowac sie w glownym katalogu dyskietki.

}

/* Called with: 			Example
 *  $0 = sets suffix			.tgz
 */
message cdromsource
{Podaj urzadzenie CDROM oraz katalog na CDROMie, w ktorym znajduje sie
dystrybucja. 
Pamietaj, ze katalog musi zawierac pliki $0.

}

message No_cd_found
{Could not locate a CD medium in any drive with the distribution sets! 
Enter the correct data manually, or insert a disk and retry. 
}

message abort_install
{Cancel installation}

message source_sel_retry
{Back to source selection & retry}

message Available_cds
{Dostepne napedy CD}

message ask_cd
{Znaleziono kilka napedow CD, prosze wybrac ten, ktory zawiera
nosnik instalacyjny.}

message cd_path_not_found
{Zbiory instalacyjne nie zostaly znalezione w domyslnym polozeniu na tym
CD. Prosze sprawdzic urzadzenie i sciezke.}

/* Called with: 			Example
 *  $0 = sets suffix			.tgz
 */
message localfssource
{Podaj niezamontowane lokalne urzadzenie oraz katalog na nim, gdzie
znajduje sie dystrybucja. 
Pamietaj, ze katalog musi zawierac pliki $0.

}

/* Called with: 			Example
 *  $0 = sets suffix			.tgz
 */
message localdir
{Podaj aktualnie zamontowany lokalny katalog, gdzie znajduje sie dystrybucja. 
Pamietaj, ze katalog musi zawierac pliki $0.

}

message filesys
{system plikow}

message nonet
{Nie znalazlem zadnych interfejsow sieciowych do uzycia z NetBSD.
Zostaniesz przeniesiony do glownego menu.
}

message netup
{Nastepujace interfejsy sieciowe sa aktywne: %s
Czy ktorys z nich jest podlaczony do serwera?}

message asknetdev
{Ktorego urzadzenia mam uzyc?}

message netdevs
{Dostepne interfejsy}

message netinfo
{Aby mozna bylo uzywac sieci, potrzebujemy odpowiedzi na ponizsze pytania:

}

message net_domain
{Twoja domena DNS}

message net_host
{Twoja nazwa hosta}

message net_ip
{Twoj adres IPv4}

message net_srv_ip
{Adres IPv4 serwera}

message net_mask
{Maska podsieci IPv4}

message net_namesrv
{Serwer nazw IPv4}

message net_defroute
{bramka IPv4}

message net_media
{Typ interfejsu sieciowego}

message net_ssid
{Wi-Fi SSID?}

message net_passphrase
{Wi-Fi passphrase?}

message netok
{Ponizej sa wartosci, ktore wprowadziles.

Domena DNS:		%s 
Nazwa hosta:		%s 
Serwer nazw:		%s 
Podstawowy interfejs:	%s 
Medium sieciowe:	%s
Twoj adres IP:		%s 
Maska podsieci:		%s 
Bramka IPv4:		%s 
}

message netok_slip
{Ponizej sa wartosci, ktore wprowadziles. Czy sa poprawne?

Domena DNS:		%s 
Nazwa hosta:		%s 
Serwer nazw:		%s 
Podstawowy interfejs:	%s 
Medium sieciowe:	%s
Twoj adres IP:		%s 
Adres IP serwera:	%s
Maska podsieci:		%s 
Bramka IPv4:		%s 
}

message netokv6
{Autkonfiguracja IPv6:	%s 
}

message netok_ok
{Czy sa poprawne?}

message slattach {
Podaja parametry dla polecenia 'slattach'
}

message wait_network
{
Poczekaj, az interfejs sieciowy zostanie uaktywniony.
}

message resolv
{Nie moglem utworzyc /etc/resolv.conf.  Instalacja przerwana.
}

/* Called with: 			Example
 *  $0 = target prefix			/target
 *  $1 = error message			No such file or directory
 */
message realdir
{Nie moglem przejsc do katalogu $0: $1
Instalacja przerwana.}

message delete_xfer_file
{Usun po zakonczeniu instalacji}

/* Called with: 			Example
 *  $0 = set name			base
 */
message notarfile
{Pakiet $0 nie istnieje.}

message endtarok
{Wszystkie wybrane pakiety dystrybucji zostaly rozpakowane.}

message endtar
{Wystapil blad w trackie rozpakowywania pakietow.
Twoja instalacja jest niekompletna.

Wybrales %d pakietow dystrybucyjnych. %d pakiety nie zostaly znalezione
i %d zostalo pominietych z powodu bledow. Z %d docelowych,
%d rozpakowalo sie bez bledow i %d z bledami.

Instalacja zostala przerwana. Sprawdz zrodlo swojej dystrybucji i rozwaz
reinstalacje pakietow z glownego menu.}

message abort_inst {Instalacja zostala przerwana.}
message abort_part {Install aborted.}

message abortinst
{Dystrybucja nie zostala pomyslnie wgrana. Bedziesz musial zrobic to recznie.
Instalacja zostala przerwana.
}

message abortupgr
{Dystrybucja nie zostala pomyslnie wgrana. Bedziesz musial zrobic to recznie.
Aktualizacja zostala przerwana.
}

message abortunpack
{Rozpakowanie dodatkowych pakietow nie udalo sie. Bedziesz musial
to zrobic recznie, albo wybrac inne zrodlo pakietow i sprobowac ponownie.
}

message createfstab
{Pojawil sie powazny problem! Nie mozna utworzyc /mnt/etc/fstab. Spadamy!
}


message noetcfstab
{Pomocy! Na dysku docelowym %s nie ma /etc/fstab. Przerywamy aktualizacje. 
}

message badetcfstab
{Pomocy! Nie moge przeczytac /etc/fstab na dysku %s. Przerywamy aktualizacje.
}

message X_oldexists
{Nie moge zapisac %s/bin/X jako %s/bin/X.old, poniewaz
na docelowym dysku jest juz %s/bin/X.old. Napraw to przed kontynuacja.

Jedyny sposob to uruchomic powloke z menu Narzedziowego i sprawdzic
docelowe %s/bin/X oraz %s/bin/X.old. Jesli
%s/bin/X.old pochodzi z zakonczonej aktualizacji, mozesz usunac
%s/bin/X.old i zrobic restart. Albo jesli %s/bin/X.old
pochodzi z aktualnej niekompletnej aktualizacji, mozesz usunac
%s/bin/X i przeniesc %s/bin/X.old na %s/bin/X.

Przerywamy aktualizacje.}

message netnotup
{Pojawil sie problem z konfiguracja twojej sieci. Albo twoja bramka
albo serwer nazw nie byl osiagalny przez ping. Czy chcesz skonfigurowac
siec jeszcze raz? (Wybor "Nie" pozwala ci kontynuowac lub przerwac instalacje.)
}

message netnotup_continueanyway
{Czy chcesz kontynuowac proces instalacji i zalozyc, ze twoja siec dziala?
("Nie" przerywa proces instalacji.)
}

message makedev
{Tworzenie plikow urzadzen ...
}

/* Called with:				Example
 *  $0 = device name			/dev/rwd0a
 *  $1 = file system type		ffs
 *  $2 = error return code form fsck	8
 */
message badfs
{Wyglada na to, ze $0 nie jest systemem plikow $1 lub
fsck dysku nie powiodlo sie. Zamontowac dysk pomimo tego? (Kod
bledu $2.)}

message rootmissing
{ Nie moge znalezc docelowego / %s.
}

message badroot
{Utowrzony nowy system plikow nie przeszedl podstawowych testow.
 Jestes pewien, ze zainstalowales wszystkie wymagane pakiety? 
}

message fd_type
{System plikow na dyskietce}

message fdnotfound
{Nie moglem znalezc pliku na dyskietce.
}

message fdremount
{Dyskietka nie zostala pomyslnie zamontowana.
}

message fdmount
{Wloz dyskietke zawierajaca plik "%s.%s".

Jezeli nie masz juz wiecej dyskietek, wybierz "Pakiet kompletny"
aby rozpoczac proces jego instalacji. Wybierz "Przerwij pobieranie"
zeby wybrac inne zrodlo oprogramowania.
}

message mntnetconfig
{Czy informacje o sieci, ktore podales sa prawidlowe dla tej maszyny
w reguralnej pracy i czy chcesz aby je zapisac w /etc? }

message cur_distsets
{Ponizej jest lista pakietow dystrybucyjnych, ktore zostana uzyte.

}

message cur_distsets_header
{Pakiet dystrybucyjny             Uzyc?
--------------------------------- -----
}

message set_base
{Base}

message set_system
{System (/etc)}

message set_compiler
{Narzedzia Kompilacyjne}

message set_dtb
{Devicetree hardware descriptions}

message set_games
{Gry}

message set_gpufw
{Graphics driver firmware}

message set_man_pages
{Strony Podrecznika}

message set_misc
{Inne}

message set_modules
{Moduly kernela}

message set_rescue
{Recovery tools}

message set_tests
{Programy testujace}

message set_text_tools
{Narzedzia przetw. tekstu}

message set_X11
{Pakiety X11}

message set_X11_base
{X11 base oraz klienci}

message set_X11_etc
{Konfiguracja X11}

message set_X11_fonts
{Czcionki X11}

message set_X11_servers
{Serwery X11}

message set_X11_prog
{Programowanie X11}

message set_source
{Pakiety zrodel i debug.}

message set_syssrc
{Zrodla kernela}

message set_src
{Zrodla pak. Base}

message set_sharesrc
{Zrodla Share}

message set_gnusrc
{Zrodla GNU}

message set_xsrc
{Zrodla X11}

message set_debug
{Symbole debugowania}

message set_xdebug
{Symbole debugowania X11}

message select_all
{Wybierz wszystkie powyzsze pakiety}

message select_none
{Odznacz wszystkie powyzsze pakiety}

message install_selected_sets
{Zainstaluj wybrane pakiety}

message tarerror
{Pojawil sie blad w trakcie rozpakowywanie pliku %s. To znaczy, ze
pewne pliki nie zostaly prawidlowo rozpakowane i twoj system
nie bedzie kompletny.

Kontynuowac rozpakowywanie pakietow?}

/* Called with: 			Example
 *  $0 = partitioning name		Master Boot Record (MBR)
 *  $1 = short version of $0		MBR
 */
message must_be_one_root
{Musi byc jedna partycja do zamontowania pod '/'.}

/* Called with: 			Example
 *  $0 = first partition description	70 - 90 MB, MSDOS
 *  $1 = second partition description	80 - 1500 MB, 4.2BSD
 */
message partitions_overlap
{partycje $0 i $1 pokrywaja sie.}

message No_Bootcode
{Brak programu rozruchowego dla glownej partycji}

message cannot_ufs2_root
{Glowny system plikow nie moze byc FFSv2 poniewaz nie ma programu rozruchowego dla
tej platformy.}

message edit_partitions_again
{

Mozesz albo zedytowac tablice partycji recznie, albo poddac sie
i powrocic do glownego menu.

Edytowac tablice partycji ponownie ?}

/* Called with: 			Example
 *  $0 = missing file			/some/path
 */
message config_open_error
{Nie moglem otworzyc pliku konfiguracyjnego $0}

message choose_timezone
{Wybierz strefe czasowa, ktora najlepiej ci odpowiada z ponizszej listy. 
Nacisnij ENTER aby wybrac. 
Nacisnij 'x' a potem ENTER aby wyjsc.

 Domyslna:	%s 
 Wybrana:	%s 
 Lokalny czas:	%s %s 
}

message tz_back
{Powroc do glownej listy stref}

message swapactive
{Dysk, ktory wybrales posiada partycje wymiany, ktora moze byc aktualnie
w uzyciu jesli twoj system ma malo pamieci. Poniewaz chcesz zmienic uklad
partycji, partycja wymiany zostanie teraz wylaczona. Moze to spowodowac
pojawienie sie bledow. Jesli zuwazysz takie bledy zrestartuj komputer,
a nastepnie sprobuj jeszcze raz.}

message swapdelfailed
{Sysinst nie mogl deaktywowac partycji wymiany na dysku, ktory wybrales
do instalacji. Zrestartuj komputer i sprobuj jeszcze raz.}

message rootpw
{Haslo root'a w nowo zainstalowanym systemie nie zostalo jeszcze ustawione,
i dlatego jest puste. Czy chcesz teraz ustawic haslo dla root'a?}

message force_rootpw
{The root password of the newly installed system has not yet been
initialized. 
 
If you do not want to set a password, enter an empty line.}

message rootsh
{Mozesz teraz wybrac, ktorej powloki ma uzywac uzytkownik root. Domyslnie
jest to /bin/sh, ale moze preferujesz inna.}

message no_root_fs
{
Nie zdefiniowano glownego systemu plikow. Musisz zdefiniowac przynajmniej
jeden mountpoint z "/".

Nacisnij <enter> aby kontynuowac.
}

message Pick_an_option {Wybierz opcje aby je wlaczyc lub wylaczyc.}
message Scripting {Skrypty}
message Logging {Logowanie}

message Status  {   Status: }
message Command {Polecenie: }
message Running {Pracuje}
message Finished {Zakonczone}
message Command_failed {Polecenie nie powiodlo sie}
message Command_ended_on_signal {Polecenie zakonczylo sie sygnalem}

message NetBSD_VERSION_Install_System {System Instalacyjny NetBSD-@@VERSION@@}
message Exit_Install_System {Wyjdz z Systemu Instalacyjnego}
message Install_NetBSD_to_hard_disk {Zainstaluj NetBSD na twardym dysku}
message Upgrade_NetBSD_on_a_hard_disk {Zaktualizuj NetBSD na twardym dysku}
message Re_install_sets_or_install_additional_sets {Przeinstaluj albo zainstaluj dodatkowe pakiety}
message Reboot_the_computer {Zrestartuj komputer}
message Utility_menu {Menu Narzedziowe}
message Config_menu {Menu konfiguracji}
message exit_menu_generic {Wstecz}
message exit_utility_menu {Exit}
message NetBSD_VERSION_Utilities {Narzedzia NetBSD-@@VERSION@@}
message Run_bin_sh {Uruchom /bin/sh}
message Set_timezone {Ustaw strefe czasowa}
message Configure_network {Skonfiguruj siec}
message Partition_a_disk {Skonfiguruj dysk}
message Logging_functions {Funkcje logowania}
message Halt_the_system {Zatrzymaj system}
message yes_or_no {tak lub nie?}
message Hit_enter_to_continue {Nacisnij enter aby kontynuowac}
message Choose_your_installation {Wybierz swoja instalacje}

/* Called with:				Example
 *  $0 = partitioning name		Master Boot Record (MBR)
 *  $1 = short version of $0		MBR
 */
message Keep_existing_partitions
{Uzyj istniejacych partycji MBR}

/* Called with:				Example
 *  $0 = partitioning name		Master Boot Record (MBR)
 *  $1 = short version of $0		MBR
 */
message Set_Sizes {Ustaw rozmiary partycji NetBSD}

/* Called with:				Example
 *  $0 = partitioning name		Master Boot Record (MBR)
 *  $1 = short version of $0		MBR
 */
message Use_Default_Parts {Uzyj domyslnych rozmiarow partycji}

/* Called with:				Example
 *  $0 = current partitioning name	Master Boot Record (MBR)
 *  $1 = short version of $0		MBR
 */
message Use_Different_Part_Scheme
{Delete everything, use different partitions (not $1)}

message Gigabytes {Gigabajty}
message Megabytes {Megabajty}
message Bytes {Bajty}
message Cylinders {Cylindry}
message Sectors {Sektory}
message Select_medium {Wybierz nosnik}
message ftp {FTP}
message http {HTTP}
message nfs {NFS}
.if HAVE_INSTALL_IMAGE
message cdrom {CD-ROM / DVD / nosnik instalacyjny}
.else
message cdrom {CD-ROM / DVD}
.endif
message floppy {Dyskietka}
message local_fs {Niezamontowany system plikow}
message local_dir {Lokalny katalog}
message Select_your_distribution {Wybierz swoja dystrybucje}
message Full_installation {Pelna instalacja}
message Full_installation_nox {Instalacja bez X11}
message Minimal_installation {Minimalna instalacja}
message Custom_installation {Inna instalacja}
message hidden {** ukryte **}
message Host {Host}
message Base_dir {Katalog}
message Set_dir_src {Katalog pakietow binarych}
message Set_dir_bin {Katalog pkgsrc}
message Xfer_dir {Katalog z plikami pobranymi}
message transfer_method {Sposob pobierania}
message User {Uzytkownik}
message Password {Haslo}
message Proxy {Proxy}
message Get_Distribution {Sciagnij Dystrybucje}
message Continue {Kontynuuj}
message Prompt_Continue {Kontynuuj?}
message What_do_you_want_to_do {Co chcesz zrobic?}
message Try_again {Sprobowac jeszcze raz}
message Set_finished {Pakiet kompletny}
message Skip_set {Pomin pakiet}
message Skip_group {Pomin grupe pakietow}
message Abandon {Przerwij instalacje}
message Abort_fetch {Przerwij pobieranie}
message Device {Urzadzenie}
message File_system {SystemPlikow}
message Select_DNS_server {  Wybierz serwer nazw}
message other {inny  }
message Perform_autoconfiguration {Wykonac autkonfiguracje?}
message Root_shell {Powloka root'a}
message Color_scheme {Kolorystyka}
message White_on_black {Bialy na czarnym}
message Black_on_white {Czarny na bia³ym}
message White_on_blue {Bialy na niebieskim}
message Green_on_black {Zielony na czarnym}
message User_shell {Powloka uzytkownika}

.if AOUT2ELF
message aoutfail
{Docelowy katalog na biblioteki wpsoldzielone a.out nie mogl zostac utworzony
Sproboj jeszcze raz procedury
aktualizacji i upewnij sie, ze zamontowales wszystkie systemy plikow.}

message emulbackup
{Albo /emul/aout albo /emul w twoim systemie byl symbolicznym linkiem
wskazujacym na niezamontowany system. Zostalo mu dodane rozszerzenie '.old'.
Kiedy juz uruchomisz swoj zaktualizowany system, mozliwe ze bedziesz musial
zajac sie polaczeniem nowo utworzonego /emul/aout ze starym.
}
.endif

message oldsendmail
{Sendmail nie jest dostepny w tym wydaniu NetBSD. Domyslnym MTA jest
postfix. Plik /etc/mailer.conf ciagle wskazuje usuniety program
sendmail. Chcesz automatycznie uaktualnic /etc/mailer.conf dla
postfix? Jesli wybierzesz "Nie", trzeba bedzie recznie zmienic
/etc/mailer.conf, aby dzialalo dostarczanie poczty.}

message license
{Aby uzywac interfejsu sieciowego %s, musisz zgodzic sie na licencje
zawarta w pliku %s.
Aby obejrzec ten plik, mozesz wpisac ^Z, przejrzec jego zawartosc,
a nastepnie wpisac "fg".}

message binpkg
{Aby skonfigurowac binarny system pakietow, wybierz lokalizacje siecowa, z
ktorej nalezy pobrac pakiety. Gdy system wstanie, mozesz wykorzystac 'pkgin' do
zainstalowania albo odinstalowania dodatkowych pakietow.}

message pkgpath
{Ponizej wyszczegolniono protokol, hosta, katalog, nazwe uzytkownika oraz haslo,
ktore beda wykorzystane do nawiazania polaczenia. Haslo nie jest wymagane gdy
nazwa uzytkownika to "ftp".

}
message rcconf_backup_failed {Stworzenie kopii zapasowej rc.conf nie powiodlo
sie. Kontynuowac?}
message rcconf_backup_succeeded {Kopia zapasowa rc.conf zostala zapisana do %s.}
message rcconf_restore_failed {Przywracania kopii zapasowej rc.conf nie powiodlo
sie.}
message rcconf_delete_failed {Usuwanie starego wpisu %s nie powiodlo sie.}
message Pkg_dir {Katalog z pakietami}
message configure_prior {skonfiguruj przed instalacja}
message configure {skonfiguruj}
message change {zmien}
message password_set {ustawiono haslo}
message YES {TAK}
message NO {NIE}
message DONE {ZAKONCZ}
message abandoned {Porzucony}
message empty {***PUSTE***}
message timezone {Strefa czasowa}
message change_rootpw {Zmien haslo root'a}
message enable_binpkg {Wlacz instalacje pakietow binarnych}
message enable_sshd {Wlacz sshd}
message enable_ntpd {Wlacz ntpd}
message run_ntpdate {uruchom ntpdate podczas startu systemu}
message enable_mdnsd {Wlacz mdnsd}
message enable_xdm {Wlacz xdm}
message enable_cgd {Wlacz cgd}
message enable_lvm {Wlacz lvm}
message enable_raid {Wlacz raidframe}
message add_a_user {Dodaj uzytkownika}
message configmenu {Skonfiguruj dodatkowe elementy w razie potrzeby.}
message doneconfig {Zakoncz konfiguracje}
message Install_pkgin {Zainstaluj pkgin i uaktualnij liste pakietow}
message binpkg_installed 
{Skonfigurowales system tak aby wykorzystywal pkgin do instalacji pakietow
binarnych. Aby zainstalowac pakiet, wykonaj:

pkgin install <packagename>

z powloki root'a. Jesli potrzebujesz wiecej informacji przeczytaj strone
podrecznika pkgin(1).}
message Install_pkgsrc {Pobierz i rozpakuj pkgsrc}
message pkgsrc
{Instalacja pkgsrc wymaga rozpakowania archiwum pobranego z sieci.
Ponizej wyszczegolniono protokol, hosta, katalog, nazwe uzytkownika oraz haslo,
ktore beda wykorzystane do nawiazania polaczenia. Haslo nie jest wymagane gdy
nazwa uzytkownika to "ftp".

}
message Pkgsrc_dir {katalog pkgsrc}
message get_pkgsrc {Pobierz i rozpakuj pkgsrc}
message retry_pkgsrc_network {Konfiguracja sieci nie powiodla sie. Sprobowac
ponownie?}
message quit_pkgsrc {Zakoncz bez zainstalowania pkgsrc}
message quit_pkgs_install {Zakoncz bez zainstalowania bin pkg}
message pkgin_failed 
{Instalacja pkgin nie powiodla sie, prawdopodobnie dlatego ze nie znaleziono 
pakietow binarnych.
Sprawdz sciezke pakietow i sprobuj ponownie.}
message failed {Nie powiodlo sie}

message askfsmountadv {Punkt montowania (lub "raid", "cgd" albo "lvm")?}
message partman {Partycje rozszerzone}
message editpart {Edytuj partycje}
message selectwedge {Preconfigured "wedges" dk(4)}

message fremove {USUN}
message remove {Usun}
message add {Dodaj}
message auto {auto}

message removepartswarn {To usunie wszystkie partycje na dysku!}
message saveprompt {Zapisac zmiany przed zakonczeniem?}
message cantsave {Zmiany nie moga byc zapisane.}
message noroot {Brak zedfiniowanej partycji /. Nie mozna kontynuowac.\n}

message addusername {Nazwa nowego uzytkownika (8 zn.)}
message addusertowheel {Chcesz dodac tego uzytkownika do grupy wheel?}
message Delete_partition
{Usun partycje}

message No_filesystem_newfs
{Na wybranej partycji nie znaleziono systemu plikow. Czy chcesz ja
sformatowac?}

message swap_display	{swap}

/* Called with: 			Example
 *  $0 = parent device name		sd0
 *  $1 = swap partition name		my_swap
 */
message Auto_add_swap_part
{Partycja swap ($1) 
istnieje na $0. 
Chcesz jej uzyc?}

message parttype_disklabel {disklabel BSD}
message parttype_disklabel_short {disklabel}
/*
 * This is used on architectures with MBR above disklabel when there is
 * no MBR on a disk.
 */
message parttype_only_disklabel {disklabel (NetBSD only)}

message select_part_scheme
{Ten dysk nie byl wczesniej partycjonowany. Wybierz typ tablicy
partycji z ponizszych.}

message select_other_partscheme
{Wybierz inny typ tablicy partycji z ponizszych.}

message select_part_limit
{Niektore typy tablic moga byc uzywane tylko na poczatkach duzych dyskow.
Ponizej przedstawione sa limity.}

/* Called with: 			Example
 *  $0 = device name			ld0
 *  $1 = size				3 TB
 */
message part_limit_disksize
{To urzadzenie ($0) ma rozmiar $1.}

message size_limit	{Maks.:}

message	addpart		{Dodaj partycje}
message	nopart		{      (brak zdefiniowanej)}
message	custom_type	{Nieznane}

message dl_type_invalid	{Nieznany typ systemu plikow (0 .. 255)}

message	cancel		{Anuluj}

message	out_of_range	{Nieprawidlowa wartosc}
message	invalid_guid	{Nieprawidlowa GUID}

message	reedit_partitions	{Edytuj ponownie}
message abort_installation	{Anuluj instalacje}

message dl_get_custom_fstype {ID typu systemu plikow (do 255)}

message err_too_many_partitions	{Zbyt duzo partycji}

/* Called with: 			Example
 *  $0 = mount point			/home
 */
message	mp_already_exists	{$0 juz istnieje!}

message ptnsize_replace_existing
{Ta partycja juz istnieje.
Aby zmienic jej rozmiar, trzeba ja skasowac i stworzyc ponownie.
Wszystkie dane zostana utracone
Chcesz ja skasowac i kontynuowac?}

message part_not_deletable	{Nieusuwalna partycja systemowa}

message ptn_type		{rodzaj}
message ptn_start		{poczatek}
message ptn_size		{rozmiar}
message ptn_end			{koniec}

message ptn_bsize		{rozmiar bloku}
message ptn_fsize		{rozmiar fragmentu}
message ptn_isize		{Sredni rozm. pliku}

/* Called with: 			Example
 *  $0 = avg file size in byte		1200
 */
message ptn_isize_bytes		{$0 bajtow}
message ptn_isize_dflt		{4 fragmenty}

message Select_file_system_block_size
{Wybierz rozmiar bloku dla systemu plikow}

message Select_file_system_fragment_size
{Wybierz rozmiar fragmentu dla systemu plikow}

message ptn_isize_prompt
{sredni rozmiar pliku (bajty)}

message No_free_space {Brak wolnego miejsca}
message Invalid_numeric {Nieprawidlowa wartosc!}
message Too_large {Zbyt duza wartosc!}

/* Called with:				Example
 *  $0 = start of free space		500
 *  $1 = end of free space		599
 *  $2 = size of free space		100
 *  $3 = unit in use			MB
 */
message free_space_line {Miejsce od $0..$1 $3 (rozmiar $2 $3)\n}

message	fs_type_ffsv2	{FFSv2}
message	fs_type_ffs	{FFS}
message	fs_type_efi_sp	{EFI system partition}
message fs_type_ext2old	{Linux Ext2 (old)}
message	other_fs_type	{Inny typ}

message	editpack	{Edytuj nazwe dysku}
message	edit_disk_pack_hdr
{Nazwa dysku jest dowolna.
Sluzy rozroznieniu roznych dyskow od siebie.
Moze tez sluzyc do tworzenia "wedges" dla tego dysku

Wprowadz nazwe dysku}

/* Called with:				Example
 *  $0 = outer partitioning name	Master Boot Record (MBR)
 *  $1 = short version of $0		MBR
 */
message reeditpart
{Czy chcesz zmienic tablice partycji ($1)?}


/* Called with:				Example
 *  $0 = device name			wd0
 *  $1 = outer partitioning name	Master Boot Record (MBR)
 *  $2 = inner partitioning name	BSD disklabel
 *  $3 = short version of $1		MBR
 *  $4 = short version of $2		disklabel
 *  $5 = size needed for NetBSD		250M
 *  $6 = size needed to build NetBSD	15G
 */
message fullpart
{Zainstalujemy teraz NetBSD na dysku $0. Mozesz wybrac, czy chcesz
zainstalowac NetBSD na calym dysku, czy tylko na jego czesci.

Instalacja na czesci dysku, tworzy partycje, lub 'plaster', dla NetBSD
w tablicy partycji $3 twojego dysku. Instalacja na calym dysku jest
`zdecydowanie polecana': zabiera ona caly $3. Spowoduje to calkowita
utrate danych na dysku. Uniemozliwia ona take pozniejsza instalacje kilku
systemow na tym dysku (chyba, ze nadpiszesz NetBSD i przeinstalujesz uzywajac
tylko czesci dysku).}

message Select_your_choice
{Wybierz}

/* Called with: 			Example
 *  $0 = partitioning name		Master Boot Record (MBR)
 *  $1 = short version of $0		MBR
 */
message Use_only_part_of_the_disk
{Uzyj tylko czesci dysku}

message Use_the_entire_disk
{Uzyj calego dysku}

/* Called with:				Example
 *  $0 = device name			wd0
 *  $1 = total disk size		3000 GB
 *  $2 = unallocated space		1.2 GB
 */
message part_header
{ Calkowity rozmiar dysku $0: $1 - wolna: $2}
message part_header_col_start	{Pocz}
message part_header_col_size	{Rozm}
message part_header_col_flag	{Flg}

message Partition_table_ok
{Tablica partycji jest poprawna}

message Dont_change
{Nie zmieniaj}
message Other_kind
{Inny typ}

/* Called with:				Example
 *  $0 = outer partitioning name	Master Boot Record (MBR)
 *  $1 = short version of $0		MBR
 */
message nobsdpart
{Nie ma partycji NetBSD w tablicy partycji $1.}

/* Called with:				Example
 *  $0 = outer partitioning name	Master Boot Record (MBR)
 *  $1 = short version of $0		MBR
 */
message multbsdpart
{W tablicy partycji $1 znajduje sie kilka partycji NetBSD.
Powinienies oznaczyc jedna z nich jako przeznaczona do instalacji.
}

message ovrwrite
{Twoj dysk aktualnie posiada partycje nie-NetBSD. Czy napewno chcesz ja
nadpisac z NetBSD?
}

message Partition_OK
{Partycje OK}

/* Called with:				Example
 *  $0 = device name			wd0
 *  $1 = outer partitioning name	Master Boot Record (MBR)
 *  $2 = short version of $1		MBR
 *  $3 = other flag options		d = bootselect default, a = active
 */
message editparttable
{Edytuj DOSowa tablice partycji. Tablica partycji wyglada tak:

}

message install_flag	{I}
message newfs_flag	{N}

message ptn_install	{do instalacji}
message ptn_instflag_desc	{(I)nstalacja, }

message clone_flag	{C}
message clone_flag_desc	{, (C)lone}

message parttype_gpt {Guid Partition Table (GPT)}
message parttype_gpt_short {GPT}

message	ptn_label	{Label}
message ptn_uuid	{UUID}
message	ptn_gpt_type	{GPT Type}
message	ptn_boot	{Boot}

/* Called with:				Example
 *  $0 = outer partitioning name	Master Boot Record (MBR)
 *  $1 = short version of $0		MBR
 */
message use_partitions_anyway
{Use this partitions anyway}

message	gpt_flags	{R}
message	gpt_flag_desc	{, (R)ozruchowa}

/* Called with:				Example
 *  $0 = file system type		FFSv2
 */
message size_ptn_not_mounted		{(Inna: $0)}

message running_system			{current system}

message clone_from_elsewhere		{Clone external partition(s)}
message select_foreign_part
{Please select an external source partition:}
message select_source_hdr
{Your currently selected source partitions are:}
message clone_with_data			{Clone with data}
message	select_source_add		{Add another partition}
message clone_target_end		{Add at end}
message clone_target_hdr
{Insert cloned partitions before:}
message clone_target_disp		{cloned partition(s)}
message clone_src_done
{Source selection OK, proceed to target selection}

message network_ok
{Your network seems to work fine. 
Should we skip the configuration 
and just use the network as-is?}
