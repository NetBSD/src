#ifndef _IF_ALLOC_H
#define _IF_ALLOC_H

#ifndef _NET_IF_H
struct ifnet;
struct ifaddr;
#endif

#define _DEBUG_IFA_REF

#define	_HAS_IF_ALLOC	1
#define _HAS_IF_ADDREF	1
#define _HAS_IFA_ADDREF	1
struct ifnet *if_alloc __P((void));
void	if_free __P((struct ifnet *));
void	if_addref __P((struct ifnet *));
void	if_delref __P((struct ifnet *));
#ifdef _DEBUG_IFA_REF
void	ifa_addref1 __P((struct ifaddr *, char *));
void	ifa_delref1 __P((struct ifaddr *, char *));
#define ifa_addref(x) ifa_addref1((x), __FUNCTION__)
#define ifa_delref(x) ifa_delref1((x), __FUNCTION__)
#else
void	ifa_addref __P((struct ifaddr *));
void	ifa_delref __P((struct ifaddr *));
#endif  /* _DEBUG_IFA_REF */
#endif
