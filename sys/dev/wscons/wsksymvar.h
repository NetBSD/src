#ifndef _DEV_WSCONS_WSKSYMVAR_H_
#define _DEV_WSCONS_WSKSYMVAR_H_

#define WSKBD_STRING_LEN	8		/* Function keys, 7 chars+NUL */

#ifndef _KERNEL
#include <sys/types.h>
#endif

typedef u_int16_t keysym_t;
typedef u_int16_t kbd_t;

struct wscons_keymap {
	keysym_t command;
	keysym_t group1[2];
	keysym_t group2[2];
};

#ifdef _KERNEL
struct wscons_keydesc {
	kbd_t	name;				/* name of this map */
	kbd_t	base;				/* map this one is based on */
	int	map_size;			/* size of map */
	const keysym_t *map;			/* the map itself */
};

/*
 * Utility functions.
 */
void	wskbd_init_keymap __P((int, struct wscons_keymap **, int *));
int	wskbd_load_keymap __P((kbd_t, const struct wscons_keydesc *, int,
                               struct wscons_keymap **, int *));
keysym_t wskbd_compose_value __P((keysym_t *));
char *	wskbd_get_string __P((keysym_t));
int	wskbd_set_string __P((keysym_t, char *));

#endif

#endif /* !_DEV_WSCONS_WSKSYMVAR_H_ */
