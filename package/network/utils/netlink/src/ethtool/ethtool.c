#include "ethtool.h"
#include <linux/netlink.h>

/* The netlink family for ethtool messages */
struct nl_family ethtool_nl_family = {
    .name = "ethtool",
    .id   = 0, /* will be assigned by ethtool_nl_init */
};

/* initialize netlink connection for ethtool family */
int ethtool_nl_init(struct mnl_socket *nl)
{
    /* minimal kernel 6.12 setup for testing */
    if (nl_lookup_family(nl, "ethtool", &ethtool_nl_family.id) < 0)
        return -1;
    return 0;
}
