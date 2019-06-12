/*	$NetBSD: msg.mbr.es,v 1.4 2019/06/12 06:20:17 martin Exp $	*/

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

/* MBR Message catalog -- Spanish, i386 version */

/* NB: Lines ending in spaces force line breaks */

message mbr_part_header_1	{Tipo}
message mbr_part_header_2	{Mount}
.if BOOTSEL
message mbr_part_header_3	{Bootmenu}
.endif

message noactivepart
{No ha marcado ninguna partición como activa. Esto podría provocar que su
sistema no arrancase correctamente.}

message fixactivepart
{
¿Debe marcarse como activa la partición NetBSD del disco?}

message setbiosgeom
{
Se le va a preguntar por la geometría.
Por favor, introduzca el número de sectores por pista (máximo 63)
y el número de cabezas (máximo 256) que usa el BIOS para acceder al disco. 
El número de cilindros se calculará a partir del tamaño del disco.

}

message nobiosgeom
{sysinst no ha podido determinar automáticamente la geometría BIOS del disco. 
La geometría física es de %d cilindros %d sectores %d cabezas\n}

message biosguess
{Usando la información ya en disco, mi mejor estimación para la geometría
de la BIOS es de %d cilindros %d sectores %d cabezas\n}

message realgeom
{geom real: %d cil, %d cabezas, %d sec  (NB: solo para comparación)\n}

message biosgeom
{geom BIOS: %d cil, %d cabez, %d sec\n}

message ptn_active
{activa}
.if BOOTSEL
message bootmenu
{bootmenu}
message boot_dflt
{predef.}
.endif

message mbr_get_ptn_id {Tipo de partición (0..255)}
message Only_one_extended_ptn {Solamente puede haber una partición extendida}

message mbr_flags	{ap}
message mbr_flag_desc
.if BOOTSEL
{, Partición (a)ctiva, bootselect (p)redefinido}
.else
{, Partición (a)ctiva}
.endif

/* Called with:				Example
 *  $0 = device name			wd0
 *  $1 = outer partitioning name	Master Boot Record (MBR)
 *  $2 = short version of $1		MBR
 */
message dofdisk
{Configurando la tabla de particiones $2 ...
}

message wmbrfail
{Reescritura de MBR fallida. No se puede continuar.}

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
{Elija el tiempo de espera}

message bootseltimeout
{Tiempo de espera del menú: %d\n}

.endif
.endif

message parttype_mbr {Master Boot Record (MBR)}
message parttype_mbr_short {MBR}

message mbr_type_invalid	{Invalid partition type (0 .. 255)}
