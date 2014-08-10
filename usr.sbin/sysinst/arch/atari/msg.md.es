/*	$NetBSD: msg.md.es,v 1.1.2.2 2014/08/10 07:00:25 tls Exp $	*/

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

/* MD Message catalog -- spanish, atari version */

message md_hello
{
}

message md_may_remove_boot_medium
{Si ha iniciado desde disquette, ahora debería retirar el disco.
}

message dobootblks
{Instalando bloques de arranque en %s...
}

message infoahdilabel
{Ahora tiene que preparar su disco raiz para la instalación de NetBSD. Esto
se refiere mas a fondo como 'etiquetar' un disco.

Primero tiene la posibilidad de editar o crear un particionaje compatible
con AHDI en el disco de instalación. Note que NetBSD puede hacerlo sin
particiones AHDI, compruebe la documentación.
Si quiere usar un particionaje compatible con AHDI, tiene que asignar algunas
particiones a NetBSD antes de que NetBSD pueda usar el disco. Cambie la 'id'
de todas las particiones en las que quiera usar sistemas de archivos NetBSD
a 'NBD'. Cambie la 'id' de la partición en la que quiera usar swap a 'SWP'.

Quiere un particionaje compatible con AHDI? }

message set_kernel_1
{Núcleo (ATARITT)}
message set_kernel_2
{Núcleo (FALCON)}
message set_kernel_3
{Núcleo (SMALL030)}
message set_kernel_4
{Núcleo (HADES)}
message set_kernel_5
{Núcleo (MILAN-ISAIDE)}
message set_kernel_6
{Núcleo (MILAN-PCIIDE)}
