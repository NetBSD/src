/* Header: weapon.h,v 7.0 86/10/08 15:18:20 lwall Exp */

/* Log:	weapon.h,v
 * Revision 7.0  86/10/08  15:18:20  lwall
 * Split into separate files.  Added amoebas and pirates.
 *
 */

EXT int tractor INIT(0);

EXT int etorp;
EXT int btorp;

EXT OBJECT *isatorp[2][3][3];

EXT int aretorps;

void weapon_init(void);
void fire_torp(OBJECT *, int, int);
void attack(OBJECT *);
void fire_phaser(OBJECT *, int, int);
int tract(OBJECT *, int, int, int);
