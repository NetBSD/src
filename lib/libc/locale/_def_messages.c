/*	$NetBSD: _def_messages.c,v 1.3 1997/04/29 16:40:14 kleink Exp $	*/

/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <sys/localedef.h>
#include <locale.h>

const _MessagesLocale _DefaultMessagesLocale = 
{
	"^[Yn]",
	"^[Nn]",
	"yes",
	"no"
} ;

const _MessagesLocale *_CurrentMessagesLocale = &_DefaultMessagesLocale;
