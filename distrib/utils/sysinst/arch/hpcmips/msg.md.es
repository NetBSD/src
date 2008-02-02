/*	$NetBSD: msg.md.es,v 1.3.2.2 2008/02/02 05:34:01 itohy Exp $	*/

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

/* MD Message catalog -- Spanish, hpcmips version */

message md_hello
{Si ha iniciado desde disquette, ahora deberia retirar el disco.

}

message dobad144
{Instalando la tabla de bloques malos ...
}

message getboottype
{¿Le gustaría instalar el set normal de bootblocks o bootblocks por serie?

Bootblocks normal usa el dispositivo de consola de BIOS como consola
(normalmente el monitor y teclado). Bootblocks por serie usa el primer
puerto serie como consola.
}

message dobootblks
{Instalando bloques de arranque en %s....
}

message onebiosmatch
{Este disco coincide con el siguiente disco de BIOS:

}

message onebiosmatch_header
{BIOS # cilindros cabezal sectors
------ ---------- ------- -------
}

message onebiosmatch_row
{%-6x %-10d %-7d %d\n}

message biosmultmatch
{Este disco coincide con los siguientes discos de BIOS:

}

message biosmultmatch_header
{   BIOS # cilindros  cabezal sectors
   ------ ---------- ------- -------
}

message biosmultmatch_row
{%-1d: %-6x %-10d %-7d %d\n}

message pickdisk
{Escoja disco: }

message partabovechs
{La parte NetBSD del disco se halla fuera del rango que la BIOS en su
maquina puede dirigirse. Puede que no sea posible iniciar desde el.
¿Esta seguro de que quiere hacer esto?

(Si contesta 'no' sera devuelto al menú de edicion de particiones.)}

message set_kernel_1
{Núcleo (GENERIC)}
message set_kernel_2
{Núcleo (TX3912)}

