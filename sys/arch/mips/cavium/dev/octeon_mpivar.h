/*	$NetBSD: octeon_mpivar.h,v 1.2 2020/05/31 06:27:06 simonb Exp $	*/

#ifndef _DEV_OCTEON_MPI_H_
#define _DEV_OCTEON_MPI_H_

struct octmpi_controller {
	void	*sct_cookie;
	int	(*sct_configure)(void *, void *, void *);
	void	(*sct_read)(void *, u_int, u_int, size_t, uint8_t *);
	void	(*sct_write)(void *, u_int, u_int, size_t, uint8_t *);
	bus_space_tag_t		sc_bust;	/* Bus space tag */
	bus_space_handle_t	sc_bush;	/* Bus space handle */
};

struct octmpi_attach_args {
	struct octmpi_controller	*octmpi_ctrl;
};

#endif /* _DEV_OCTEON_MPI_H__ */
