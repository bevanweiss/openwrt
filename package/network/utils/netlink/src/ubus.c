#include "ubus.h"
#include "ethtool/pse.h"

/* Array of all UBUS objects (add future topics here) */
static struct ubus_object *netlink_objects[] = {
    &ethtool_pse_object,
    /* &ethtool_linkinfo_object, ... */
};

static const size_t netlink_objects_count = ARRAY_SIZE(netlink_objects);

int ubus_register_all(struct ubus_context *ctx)
{
    int ret;
    for (size_t i = 0; i < netlink_objects_count; i++) {
        if (ret = ubus_add_object(ctx, netlink_objects[i])) {
            fprintf(stderr, "Failed to add ubus object %s\n",
                    netlink_objects[i]->name);
            return ret;
        }
    }
    return 0;
}
