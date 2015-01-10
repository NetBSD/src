
typedef enum {
	BL_ADD,
	BL_PING,
} bl_type_t;

typedef struct blacklist *bl_t;

bl_t bl_create(void);
void bl_destroy(bl_t);
int bl_add(bl_t, bl_type_t, int, int, const char *ctx);

#define _PATH_BLACKLIST "/tmp/blacklist"
