#ifdef __FreeBSD__
typedef struct mtx ieee80211_node_lock_t;
#define	IEEE80211_NODE_LOCK_INIT(_ic, _name) \
	mtx_init(&(_ic)->ic_nodelock, _name, "802.11 node table", MTX_DEF)
#define	IEEE80211_NODE_LOCK_DESTROY(_ic)	mtx_destroy(&(_ic)->ic_nodelock)
#define	IEEE80211_NODE_LOCK(_ic)		mtx_lock(&(_ic)->ic_nodelock)
#define	IEEE80211_NODE_UNLOCK(_ic)		mtx_unlock(&(_ic)->ic_nodelock)
#define	IEEE80211_NODE_LOCK_ASSERT(_ic) \
	mtx_assert(&(_ic)->ic_nodelock, MA_OWNED)
#else
typedef int ieee80211_node_lock_t;
#define	IEEE80211_NODE_LOCK_INIT(_ic, _name)
#define	IEEE80211_NODE_LOCK_DESTROY(_ic)
#define	IEEE80211_NODE_LOCK(_ic)		(_ic)->ic_nodelock = splnet()
#define	IEEE80211_NODE_UNLOCK(_ic)		splx((_ic)->ic_nodelock)
#define	IEEE80211_NODE_LOCK_ASSERT(_ic)
#endif
#define	IEEE80211_NODE_LOCK_BH		IEEE80211_NODE_LOCK
#define	IEEE80211_NODE_UNLOCK_BH	IEEE80211_NODE_UNLOCK
