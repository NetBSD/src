/*	$NetBSD: msg.pm.pl,v 1.1.2.1 2020/01/28 10:17:58 msaitoh Exp $	*/

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

/* extended partition manager message catalog -- Polish, machine independent */

message fillzeros {Wype�nij zerami}
message fillrandom {Wype�nij losowymi danymi}
message raid0 {0 - brak parzysto�ci, tylko proste rozk�adanie.}
message raid1 {1 - Mirroring. Parytet jest lustro.}
message raid4 {4 - Malowanie z parytetu przechowywanych w ostatniej cz�ci.}
message raid5 {5 - Malowanie z parytetu w tych wszystkich element�w.}

message wannaunblock {Urz�dzenie jest zablokowane. Czy chcesz, aby zmusi� go odblokowa� i kontynuowa�?}
message wannatry {Czy chcesz spr�bowa�?}
message create_cgd {Utw�rz wolumen kryptograficznych (CGD)}
message create_vnd {Utw�rz wirtualnego obraz dysku (VND)}
message create_vg {Utw�rz grup� wolumin�w (LVM VG)}
message create_lv {Utw�rz wolumin logiczny}
message create_raid {Utw�rz oprogramowania RAID}
message updpmlist {Aktualizuj list� urz�dze�}
message savepm {Zapisz zmiany}
message pmblocked {ZABLOK.}
message pmunchanged {BEZ ZMIAN}
message pmused {U�YWANE}
message pmmounted {(zamontowany)}
message pmunused {(nieu�ywany)}
message pmgptdisk {Dysku z GPT}

message finishpm {Zako�cz partycjonowanie}
message limitcount {Limit liczby urz�dze� zosta� osi�gni�ty!}
message invaliddev {Nieprawid�owy urz�dzenie!}
message avdisks {Dost�pne dysk�w:}
message nofreedev {Nie mo�na przydzieli� w�ze�!}
message partman_header
{Partition Manager. Wszystkie dyski, partycje oraz itp. tam wy�wietlane.
Je�li chcesz korzysta� z RAID, LVM lub CGD, wykonaj nast�puj�ce kroki:
1) Tworzenie partycji z potrzebnego typu;
2) Utw�rz RAID / LVM VG / CGD korzystania z tych partycji; 3) Zapisz go;
4) Tworzenie partycji dla RAID / CGD lub Logical Volumes dla LVM.}

/* used to form strings like: "vnd0 on /var/tmp/disk1.img" */
message        pm_menu_on {on}
/* Called with: 			Example
 *  $0 = device name			raid0
 *  $1 = raid level		       1
 */
message raid_menufmt {$0 (level $1)}
message raid_err_menufmt {EMPTY RAID!}
message raid_disks_fmt {Dyski}
message raid_spares_fmt {Cz�ci zamienne}
message raid_level_fmt {Poziom RAID}
message raid_numrow_fmt {numRow}
message raid_numcol_fmt {numCol}
message raid_numspare_fmt {numSpare}
message raid_sectpersu_fmt {sectPerSU}
message raid_superpar_fmt {SUsPerParityUnit}
message raid_superrec_fmt {SUsPerReconUnit}
message raid_nomultidim {Tablice wielowymiarowe nie s� obs�ugiwane!}
message raid_numrow_ask {numRow?}
message raid_numcol_ask {numCol?}
message raid_numspare_ask {numSpare?}
message raid_sectpersu_ask {sectPerSU?}
message raid_superpar_ask {SUsPerParityUnit?}
message raid_superrec_ask {SUsPerReconUnit?}
message raid_disks {Dyski w RAID}
message vnd_err_menufmt {�CIE�KA NIE ZDEFINIOWANE!}
message vnd_assign {ASSIGN}
message vnd_path_fmt {�cie�ka}
message vnd_assign_fmt {Create new image}
message vnd_size_fmt {Rozmiar}
message vnd_ro_fmt {Tylko do odczytu}
message vnd_geom_fmt {Ustaw geometri� r�cznie}
message vnd_bps_fmt {Bajt�w na sektor�w}
message vnd_spt_fmt {Sektory na �cie�ki}
message vnd_tpc_fmt {Utwory na cylinder}
message vnd_cyl_fmt {Si�owniki}
message vnd_path_ask {File Path?}
message vnd_size_ask {Rozmiar (MB)?}
message vnd_bps_ask {Bajt�w na sektor?}
message vnd_spt_ask {Sektory na �cie�k�?}
message vnd_tpc_ask {Utwory na cylinder?}
message vnd_cyl_ask {Cylindry}
message cgd_err_menufmt {DISK NIE ZDEFINIOWANE!}
message cgd_dev_fmt {Urz�dzenie podstawy}
message cgd_enc_fmt {Encryption}
message cgd_key_fmt {Rozmiar klucza}
message cgd_iv_fmt {Algorytm IV}
message cgd_keygen_fmt {Generowanie kluczy}
message cgd_verif_fmt {Metoda weryfikacji}
message lvm_disks {Dyski w VG}
message lvm_err_menufmt {EMPTY VG!}
message lvm_disks_fmt {PV's}
message lvm_name_fmt {Nazwa}
message lvm_maxlv_fmt {MaxLogicalVolumes}
message lvm_maxpv_fmt {MaxPhysicalVolumes}
message lvm_extsiz_fmt {PhysicalExtentSize}
message lvm_name_ask {Nazwa?}
message lvm_maxlv_ask {MaxLogicalVolumes?}
message lvm_maxpv_ask {MaxPhysicalVolumes?}
message lvm_extsiz_ask {PhysicalExtentSize (MB)?}
message lvmlv_menufmt {Wolumin logiczny}
message lvmlv_name_fmt {Nazwa}
message lvmlv_size_fmt {Rozmiar}
message lvmlv_ro_fmt {Read-only}
message lvmlv_cont_fmt {S�siaduj�cy}
message lvmlv_extnum_fmt {LogicalExtentsNumber}
message lvmlv_minor_fmt {Mniejsza liczba}
message lvmlv_mirrors_fmt {Mirrors}
message lvmlv_regsiz_fmt {MirrorLogRegionSize}
message lvmlv_pers_fmt {Trwa�e drobne numer}
message lvmlv_readahsect_fmt {ReadAheadSectors ceny}
message lvmlv_stripes_fmt {Stripes}
message lvmlv_stripesiz_fmt {StripeSize}
message lvmlv_zero_fmt {Zerowanie pierwszej KB}
message lvmlv_name_ask {Nazwa?}
message lvmlv_size_ask {Rozmiar (MB)?}
message lvmlv_extnum_ask {LogicalExtentsNumber?}
message lvmlv_minor_ask {Mniejsza liczba?}
message lvmlv_mirrors_ask {Mirrors?}
message lvmlv_regsiz_ask {MirrorLogRegionSize?}
message lvmlv_readahsect_ask {ReadAheadSectors?}
message lvmlv_stripes_ask {Stripes?}

