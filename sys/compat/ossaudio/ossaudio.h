/*	$NetBSD: ossaudio.h,v 1.4 1997/04/04 15:36:01 augustss Exp $	*/
struct oss_sys_ioctl_args;

int oss_ioctl_audio __P((struct proc *, struct oss_sys_ioctl_args *,
    register_t *));
int oss_ioctl_mixer __P((struct proc *, struct oss_sys_ioctl_args *,
    register_t *));
int oss_ioctl_sequencer __P((struct proc *, struct oss_sys_ioctl_args *,
    register_t *));
