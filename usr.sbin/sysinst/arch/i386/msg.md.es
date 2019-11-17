/*	$NetBSD: msg.md.es,v 1.1.24.1 2019/11/17 13:45:26 msaitoh Exp $	*/

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

/* MD Message catalog -- Spanish, i386 version */

message md_hello
{
}

message md_may_remove_boot_medium
{
}

message Keyboard_type {Tipo de teclado}
message kb_default {es}

message dobad144
{Instalando la tabla de bloques malos ...
}

message getboottype
{�Le gustaria instalar el set normal de bootblocks o bootblocks por serie?
 
Bootblocks normal usa el dispositivo de consola de BIOS como consola
(normalmente el monitor y teclado). Bootblocks por serie usa el primer
puerto serie como consola.

Bootblock seleccionado: }

message console_PC {Consola BIOS}
message console_com {Puerto serie en com%D en %d baud}
message console_unchanged {Sin cambios}

message Bootblocks_selection
{Seleccion de bootblocks}
message Use_normal_bootblocks	{Usar consola BIOS}
message Use_serial_com0		{Usar puerto serie com0}
message Use_serial_com1		{Usar puerto serie com1}
message Use_serial_com2		{Usar puerto serie com2}
message Use_serial_com3		{Usar puerto serie com3}
message serial_baud_rate	{Baudios puerto serie}
message Use_existing_bootblocks	{Usar bootblocks existente}

message dobootblks
{Instalando bloques de arranque en %s....
}

message onebiosmatch
{Este disco coincide con el siguiente disco de BIOS:

}

message onebiosmatch_header
{BIOS # cilindros cabez sectors sect. totales   GB 
------ --------- ----- ------- ------------- ---- 
}

message onebiosmatch_row
{%#6x %9d %5d %7d %13u %3u\n}

message This_is_the_correct_geometry
{Esta es la geometr�a correcta}
message Set_the_geometry_by_hand
{Ajustar geometr�a a mano}
message Use_one_of_these_disks
{Usar uno de estos discos}

message biosmultmatch
{Este disco coincide con los siguientes discos de BIOS:

}

message biosmultmatch_header
{   BIOS # cilindros cabez sectors  sect totales   GB 
   ------ --------- ----- ------- ------------- ---- 
}

message biosmultmatch_row
{%-1d: %#6x %9d %5d %7d %13u %3u\n}

message biosgeom_advise
{
Nota: como sysinst es capaz de concordar unicamente el disco que escoja
con un disco conocido por BIOS, los valores anteriores son probablemente
correctos, y no deber�an ser cambiados (los valores para cilindros,
cabezales y sectores son probablemente 1023, 255 y 63 - esto es correcto).
Solo deber�a cambiar la geometr�a si sabe que su BIOS reporta valores
incorrectos.
}

message pickdisk
{Escoja disco: }

message partabovechs
{La parte NetBSD del disco est� fuera del rango que la BIOS de su maquina
puede acceder.
Puede que no sea posible iniciar desde ahi.
�Est� seguro de que quiere hacer eso?

(Seleccionando 'no' le devolver� al men� de edicion de particiones.)}

message missing_bootmenu_text
{Tiene mas de un sistema operativo en el disco, pero no ha especificado
un 'menu de arranque' para la partici�n activa o la partici�n de NetBSD
en la que lo va a instalar.

�Quiere re-editar la partici�n para a�adir una entrada en el menu de
arranque?}

message no_extended_bootmenu
{Ha pedido que una partici�n extendida sea incluida en el men� de arranque.
Sin embargo la BIOS de su sistema no parece soportar el comando de lectura
usado por esa versi�n del c�digo del menu de arranque.
�Est� seguro de que quiere hacer eso?

(Seleccionando 'no' le devolver� al men� de edici�n de particiones.)}

message installbootsel
{Su configuraci�n requiere el c�digo de bootselect de NetBSD
para seleccionar que sistema usar.

Actualmente no est� instalado, �lo quiere instalar ahora?}

message installmbr
{El c�digo de arranque del Master Boot Record no parece ser v�lido.

�Quiere instalar el c�digo de arranque de NetBSD?}

message updatembr
{�Quiere actualizar el c�digo de arranque en el Master Boot
Record a la ultima versi�n del c�digo de arranque de NetBSD?}

message set_kernel_1  {N�cleo (GENERIC)}
message set_kernel_2	{Kernel (GENERIC_KASLR)}
