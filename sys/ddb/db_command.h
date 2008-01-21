/*	$NetBSD: db_command.h,v 1.24.2.5 2008/01/21 09:42:21 yamt Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998, 1999, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Hamsik.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 *
 *	Author: David B. Golub, Carnegie Mellon University
 *	Date:	7/90
 */

#ifndef _DDB_COMMAND_
#define _DDB_COMMAND_

void	db_skip_to_eol(void);
void	db_command_loop(void);
void	db_error(const char *) __dead;

extern db_addr_t db_dot;	/* current location */
extern db_addr_t db_last_addr;	/* last explicit address typed */
extern db_addr_t db_prev;	/* last address examined
				   or written */
extern db_addr_t db_next;	/* next address to be examined
				   or written */

extern char db_cmd_on_enter[];

struct db_command;



/*
 * Macro include help when DDB_VERBOSE_HELP option(9) is used
 */
#ifdef DDB_VERBOSE_HELP
#define DDB_ADD_CMD(name,funct,type,cmd_descr,cmd_arg,arg_desc)\
 name,funct,type,cmd_descr,cmd_arg,arg_desc
#else
#define DDB_ADD_CMD(name,funct,type,cmd_descr,cmd_arg,arg_desc)\
 name,funct,type
#endif
   


/*
 * we have two types of lists one for base commands like reboot
 * and another list for show subcommands.
 */

#define DDB_BASE_CMD 0
#define DDB_SHOW_CMD 1
#define DDB_MACH_CMD 2


int db_register_tbl(uint8_t, const struct db_command *);
int db_unregister_tbl(uint8_t, const struct db_command *);

/*
 * Command table
 */
struct db_command {
	const char	*name;		/* command name */
  
	/* function to call */
	void		(*fcn)(db_expr_t, bool, db_expr_t, const char *);
	/*
	 *Flag is used for modifing command behaviour.
	 *CS_OWN && CS_MORE are specify type of command arguments.
	 *CS_OWN commandmanage arguments in own way.
	 *CS_MORE db_command() prepare argument list.
	 *
	 *CS_COMPAT is set for all level 2 commands with level 3 childs (show all pages)
	 *
	 *CS_SHOW identify show command in BASE command list
	 *CS_MACH identify mach command in BASE command list
	 *
	 *CS_SET_DOT specify if this command is put to last added command memory.
	 *CS_NOREPEAT this command does not repeat
	 */
	uint16_t		flag;		/* extra info: */
#define	CS_OWN		0x1		/* non-standard syntax */
#define	CS_MORE		0x2		/* standard syntax, but may have other
					   words at end */
#define CS_COMPAT	0x4		/*is set for compatibilty with old ddb versions*/
	
#define CS_SHOW		0x8		/*select show list*/
#define CS_MACH		0x16		/*select machine dependent list*/

#define	CS_SET_DOT	0x100		/* set dot after command */
#define	CS_NOREPEAT	0x200		/* don't set last_command */
#ifdef DDB_VERBOSE_HELP
	const char *cmd_descr; /*description of command*/
	const char *cmd_arg;   /*command arguments*/
	const char *cmd_arg_help;	/* arguments description */
#endif
};

#endif /*_DDB_COMMAND_*/

