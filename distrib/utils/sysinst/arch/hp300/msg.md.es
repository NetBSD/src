/*	$NetBSD: msg.md.es,v 1.2.2.2 2005/09/19 21:10:50 tron Exp $	*/

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

/* hp300 machine dependent messages, spanish */


message md_hello
{Si su maquina tiene 4MB o menos, sysinst no funcionara correctamente.

}

message dobootblks
{Instalando bloques de arranque en %s....
}

message newdisk
{Parece que su disco, %s, no tiene una marca de disco X68K. sysinst
escribira una marca de disco.
Note que si pretende usar una parte de %s desde Human68k, deberia abortar
aqui y formatear el disco con la utilidad format.x de Human68k.
}

message ordering
{El orden de la particion %c esta mal.  ¿Editar de nuevo?}

message emptypart
{Hay una particion valida %c despues de particion(es) valida(s).
Por favor, reedite la tabla de particiones}

message set_kernel_1
{Nucleo (GENERIC)}

.if notyet
/* XXX: not yet implemented */
message existing
{¿Quiere preservar particion(es) BSD existente(s)?}

message nofreepart
{%s no tiene suficiente espacio libre para particiones para NetBSD.
Debe tener como minimo 2 particiones libres (para el sistema de archivos
raiz y para swap).
}

message notfirst
{NetBSD/hp300 debe estar instalado en la primera parte del disco de arranque.
La primera parte de %s no esta libre.
}
.endif
