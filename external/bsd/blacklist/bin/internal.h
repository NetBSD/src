

typedef struct {
	uint32_t bl_len;
	uint32_t bl_version;
	uint32_t bl_type;
	char bl_data[];
} bl_message_t;

struct blacklist {
	int b_fd;
	int b_connected;
};


#define BL_VERSION	1
