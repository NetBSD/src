
#include <stdbool.h>

typedef enum {
	BL_INVALID,
	BL_ADD,
} bl_type_t;

typedef struct blacklist *bl_t;

bl_t bl_create(bool);
void bl_destroy(bl_t);
int bl_send(bl_t, bl_type_t, int, int, const char *);
int bl_recv(bl_t, bl_type_t *, int *, int *, char *, size_t);

#define _PATH_BLACKLIST "/tmp/blacklist"
