struct confargs {
	char *ca_name;
	u_int ca_node;
	int ca_nreg;
	u_int *ca_reg;
	int ca_nintr;
	int *ca_intr;

	u_int ca_baseaddr;
	/* bus_space_tag_t ca_tag; */
};

extern void * mapiodev __P((paddr_t, psize_t));
extern int kvtop __P((caddr_t));

