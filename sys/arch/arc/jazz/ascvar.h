struct asc_config {
	struct asc_timing *ac_timing;
	int ac_cnfg3;
};

extern struct asc_config *asc_conf;

extern struct asc_timing asc_timing_40mhz, asc_timing_25mhz, asc_timing_12mhz;
