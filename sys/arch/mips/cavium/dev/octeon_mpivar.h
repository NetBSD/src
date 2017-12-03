/*	$NetBSD: octeon_mpivar.h,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $	*/

#ifndef _DEV_OCTEON_MPI_H_
#define _DEV_OCTEON_MPI_H_

struct octeon_mpi_controller {
	void	*sct_cookie;
	int	(*sct_configure)(void *, void *, void *);
	void	(*sct_read)(void *, u_int, u_int, size_t, uint8_t *);
	void	(*sct_write)(void *, u_int, u_int, size_t, uint8_t *);
	bus_space_tag_t		sc_bust;	/* Bus space tag */
	bus_space_handle_t	sc_bush;	/* Bus space handle */
};

struct octeon_mpi_attach_args {
	struct octeon_mpi_controller	*octeon_mpi_ctrl;
};

#endif /* _DEV_OCTEON_MPI_H__ */
