/*	$NetBSD: msg.mbr.pl,v 1.4 2019/06/12 06:20:18 martin Exp $	*/

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

/* MBR Message catalog -- Polish, i386 version */

message mbr_part_header_1	{Rodzaj}
message mbr_part_header_2	{Mount}
.if BOOTSEL
message mbr_part_header_3	{Wpis menu}
.endif

message noactivepart
{Nie zaznaczyles aktywnej partycji. Moze to spowodowac, ze twoj system nie
uruchomi sie prawidlowo.}

message fixactivepart
{
Czy partycja NetBSD ma zostac zaznaczona jako aktywna?}

message setbiosgeom
{
Zostaniesz poproszony o podanie geometrii dysku:
Podaj ilosc sektorow na sciezce (maksimum 63) oraz
ilosc glowic (maksimum 256) ktore BIOS ustalil jako
wlasciwe parametry dysku.
Ilosc cylindrow zostanie obliczona automatycznie na
podstawie podanych danych i rozmiaru dysku.
}

message nobiosgeom
{Sysinst nie mogl automatycznie rozpoznac geometrii dysku z BIOS. 
Fizyczna geometria to %d cylindrow %d sektorow %d glowic\n}

message biosguess
{Uzywajac informacji z dysku, najlepsze parametry geometrii dysku z BIOS to
%d cylindrow %d sektorow %d glowic\n}

message realgeom
{praw. geo: %d cyl, %d glowic, %d sek  (tylko dla porownania)\n}

message biosgeom
{BIOS geom: %d cyl, %d glowic, %d sek\n}

message ptn_active
{aktywna}
.if BOOTSEL
message bootmenu
{w menu startowym}
message boot_dflt
{domyslna}
.endif

message mbr_get_ptn_id {Typ partycji (0..255)}
message Only_one_extended_ptn {Moze byc tylko jedna partycja typu rozszerzonego}

message mbr_flags	{ad}
message mbr_flag_desc
.if BOOTSEL
{, (A)ctive partition, bootselect (d)efault}
.else
{, (A)ctive partition}
.endif

/* Called with:				Example
 *  $0 = device name			wd0
 *  $1 = outer partitioning name	Master Boot Record (MBR)
 *  $2 = short version of $1		MBR
 */
message dofdisk
{Konfigurowanie DOSowej tablicy partycji ...
}

message wmbrfail
{Nadpisanie MBR nie powiodlo sie. Nie moge kontynuowac.}

message mbr_inside_ext
{The partition needs to start within the Extended Partition}

message mbr_ext_nofit
{The Extended Partition must be able to hold all contained partitions}

message mbr_ext_not_empty
{Can not delete a non-empty extended partition!}

message mbr_no_free_primary_have_ext
{This partition is not inside the extended partition
and there is no free slot in the primary boot record}

message mbr_no_free_primary_no_ext
{No space in the primary boot block.
You may consider deleting one partition, creating an extended partition
and then re-adding the deleted one}

.if 0
.if BOOTSEL
message Set_timeout_value
{Ustaw opoznienie}

message bootseltimeout
{Opoznienie bootmenu: %d\n}

.endif
.endif

message parttype_mbr {Master Boot Record (MBR)}
message parttype_mbr_short {MBR}

message mbr_type_invalid	{Invalid partition type (0 .. 255)}
