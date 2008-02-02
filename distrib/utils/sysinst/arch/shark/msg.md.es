/*	$NetBSD: msg.md.es,v 1.6.2.2 2008/02/02 05:34:04 itohy Exp $	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Based on code written by Philip A. Nelson for Piermont Information
 * Systems Inc.
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

/* shark machine dependent messages, spanish */


message md_hello
{Si ha iniciado desde disquette, ahora debería retirar el disco.

}

message Keyboard_type {Tipo de teclado}
message kb_default {es}

message badreadbb
{No se puede leer bloque de arranque filecore
}

message badreadriscix
{No se puede leer la tabla de particiones RISCiX
}

message notnetbsdriscix
{No se ha encontrado ninguna particion NetBSD en la tabla de
particiones RISCiX - No se puede etiquetar
}

message notnetbsd
{No se ha encontrado ninguna particion NetBSD (¿disco solo de filecore?)
 - No se puede etiquetar
}

message dobootblks
{Instalando bloques de arranque en %s...
}

message arm32fspart
{Ahora tenemos nuestras particiones NetBSD en % como sigue (Tamaño y
Compensación en %s):
}

message set_kernel_1
{Núcleo (GENERIC)}

message setbootdevice
{Para conseguir que el sistema arranque automaticamente desde el sistema
de ficheros del disco, necesita configura manualmente OpenFirmware para
indicarle desde dónde debe cargar el núcleo del sistema.

OpenFirmware puede cargar un núcleo a.out de NetBSD (perdone, pero ELF no está soportado) directamente desde una partición FFS del disco local.  Así que, para
configurarlo para que arranque desde disco deberá ejecutar manualmente el
siguiente comando desde la interfaz de OpenFirmware:

setenv boot-device disk:\\netbsd.aout

Sólo tiene que ejecutar esto una vez y sólamente si la propiedad
'boot-device' no contenía el valor mostrado arriba.
}

message badclearmbr
{Ha ocurrido un error al intentar borrar el primer sector del disco.  Si el
firmware no puede verlo, pruebe a ejecutar el siguiente comando manualmente
desde la linea de comandos de la utilidad de instalación:

dd if=/dev/zero of=%s bs=512 count=1
}
