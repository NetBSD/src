/* Header: move.h,v 7.0 86/10/08 15:12:46 lwall Exp */

/* Log:	move.h,v
 * Revision 7.0  86/10/08  15:12:46  lwall
 * Split into separate files.  Added amoebas and pirates.
 *
 */

void move_init(void);
void bounce(OBJECT *);
void move_universe(void);
int lookaround(int, int, char);
int lookfor(int, int, char);
OBJECT *lookimg(int, int, char);

