struct _db_factory;

typedef struct {
	const char *key;
	char *value;
	int (*save)(struct _db_factory *,
	    const char *, const char *);
} token_t;

typedef struct {
	const char *magic, * vers_sym;
	uint32_t version;
	const token_t tokens[];
} category_t;

const category_t *
mklocaledb_init(const char *);

void
mklocaledb_dump(const category_t *, FILE *);

void
mklocaledb_record(const category_t *, const char *, const char *);

void
mklocaledb_record_uint8(const category_t *, const char *, char);
