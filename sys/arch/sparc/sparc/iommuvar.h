struct iommu_attach_args {
	bus_space_tag_t	iom_bustag;
	bus_dma_tag_t	iom_dmatag;
	char		*iom_name;	/* PROM node name */
        int		iom_node;	/* PROM handle */
	struct bootpath	*iom_bp;	/* used for locating boot device */
};
