#ifndef _ETHTOOL_H
#define _ETHTOOL_H

#include <libmnl/libmnl.h>
#include <linux/ethtool.h>
#include "../nl.h"

/* Initialize netlink socket for ethtool family */
int ethtool_nl_init(struct mnl_socket *nl);

/* Generic ethtool netlink family info */
extern struct nl_family ethtool_nl_family;

#endif /* _ETHTOOL_H */
