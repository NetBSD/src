/*	$NetBSD: libbug.h,v 1.1.32.1 2000/11/20 20:15:29 bouyer Exp $	*/

/*
 * prototypes and such.   note that get/put char are in stand.h
 */


void	mvmeprom_delay __P((int));
int	mvmeprom_diskrd __P((struct mvmeprom_dskio *));
int	mvmeprom_diskwr __P((struct mvmeprom_dskio *));
struct	mvmeprom_brdid *mvmeprom_getbrdid __P((void));
int	peekchar __P((void));
void	mvmeprom_outln __P((char *, char *));
void	mvmeprom_outstr __P((char *, char *));
void	mvmeprom_rtc_rd __P((struct mvmeprom_time *));

/*
 * bugcrt stuff 
 */

extern struct mvmeprom_args bugargs;

void	bugexec __P((void (*)(void)));
