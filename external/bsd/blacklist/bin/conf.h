
struct conf {
	int			c_port;
	int			c_proto;
	int			c_family;
	int			c_uid;
	int			c_nfail;
	int			c_duration;
};

void parseconf(const char *);
const struct conf *findconf(bl_info_t *);
