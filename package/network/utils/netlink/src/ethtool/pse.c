#include "pse.h"
#include "ethtool.h"
#include "../nl.h"

#include <libubus.h>
#include <libubox/blobmsg.h>
#include <linux/ethtool_netlink.h>
#include <string.h>
#include <net/if.h>

/* Keep your pse_fields exactly as agreed */
static const struct nl_field pse_fields[] = {
    {
        .name = "admin_state",
        .attr = ETHTOOL_A_C33_PSE_ADMIN_STATE,
        .type = NL_T_U32,
        .dir  = NL_F_GET | NL_F_SET,
    },
    {
        .name = "power_limit_max",
        .attr = ETHTOOL_A_C33_PSE_PW_LIMIT_MAX,
        .type = NL_T_U32,
        .dir  = NL_F_GET,
    },
    {
        .name = "power_limit_min",
        .attr = ETHTOOL_A_C33_PSE_PW_LIMIT_MIN,
        .type = NL_T_U32,
        .dir  = NL_F_GET,
    },
    {
        .name = "actual_power",
        .attr = ETHTOOL_A_C33_PSE_ACTUAL_PW,
        .type = NL_T_U32,
        .dir  = NL_F_GET,
    },
    { 0 }
};

/* GET command */
static const struct nl_cmd pse_get_cmd = {
    .cmd    = ETHTOOL_MSG_PSE_GET,
    .flags  = NLM_F_REQUEST | NLM_F_ACK,
    .fields = pse_fields,
};

/* SET command (sparse updates) */
static const struct nl_cmd pse_set_cmd = {
    .cmd    = ETHTOOL_MSG_PSE_SET,
    .flags  = NLM_F_REQUEST | NLM_F_ACK,
    .fields = pse_fields,
};

/* UBUS policy: expect "ifname" string in GET/SET requests */
static const struct blobmsg_policy pse_policy[] = {
    { .name = "ifname", .type = BLOBMSG_TYPE_STRING },
};

/* GET method */
int pse_get(struct ubus_context *ctx,
            struct ubus_object *obj,
            struct ubus_request_data *req,
            const char *method,
            struct blob_attr *msg)
{
    struct blob_attr *tb[ARRAY_SIZE(pse_policy)];
    char ifname[IFNAMSIZ];
    struct blob_buf reply;

    blobmsg_parse(pse_policy, ARRAY_SIZE(pse_policy), tb, blob_data(msg), blob_len(msg));
    if (!tb[0])
        return UBUS_STATUS_INVALID_ARGUMENT;

    strncpy(ifname, blobmsg_get_string(tb[0]), sizeof(ifname) - 1);
    ifname[sizeof(ifname) - 1] = '\0';

    blob_buf_init(&reply, 0);

    if (nl_do_get(NULL, &ethtool_nl_family, &pse_get_cmd,
                  ETHTOOL_A_HEADER_DEV_NAME, ifname, &reply) < 0)
        return UBUS_STATUS_UNKNOWN_ERROR;

    ubus_send_reply(ctx, req, reply.head);
    blob_buf_free(&reply);

    return 0;
}

/* SET method */
int pse_set(struct ubus_context *ctx,
            struct ubus_object *obj,
            struct ubus_request_data *req,
            const char *method,
            struct blob_attr *msg)
{
    struct blob_attr *tb[ARRAY_SIZE(pse_policy)];
    char ifname[IFNAMSIZ];
    const void *vals[] = { msg };

    blobmsg_parse(pse_policy, ARRAY_SIZE(pse_policy), tb, blob_data(msg), blob_len(msg));
    if (!tb[0])
        return UBUS_STATUS_INVALID_ARGUMENT;

    strncpy(ifname, blobmsg_get_string(tb[0]), sizeof(ifname) - 1);
    ifname[sizeof(ifname) - 1] = '\0';

    if (nl_do_set(NULL, &ethtool_nl_family, &pse_set_cmd,
                  ETHTOOL_A_HEADER_DEV_NAME, ifname,
                  pse_fields, vals) < 0)
        return UBUS_STATUS_UNKNOWN_ERROR;

    return 0;
}

/* DUMP method (no arguments) */
int pse_dump(struct ubus_context *ctx,
             struct ubus_object *obj,
             struct ubus_request_data *req,
             const char *method,
             struct blob_attr *msg)
{
    struct blob_buf reply;
    blob_buf_init(&reply, 0);

    /* nl_do_dump: 5 arguments now, last is blob_buf* */
    if (nl_do_dump(NULL, &ethtool_nl_family, &pse_get_cmd,
                   ETHTOOL_A_HEADER_DEV_NAME, &reply) < 0) {
        blob_buf_free(&reply);
        return UBUS_STATUS_UNKNOWN_ERROR;
    }

    ubus_send_reply(ctx, req, reply.head);
    blob_buf_free(&reply);

    return 0;
}


/* UBUS methods array */
static struct ubus_method pse_methods[] = {
    UBUS_METHOD("get", pse_get, pse_policy),
    UBUS_METHOD("set", pse_set, pse_policy),
    UBUS_METHOD_NOARG("dump", pse_dump),
};

/* UBUS object type and object */
struct ubus_object_type pse_object_type =
    UBUS_OBJECT_TYPE("netlink.ethtool.pse", pse_methods);

struct ubus_object ethtool_pse_object = {
    .name = "netlink.ethtool.pse",
    .type = &pse_object_type,
    .methods = pse_methods,
    .n_methods = ARRAY_SIZE(pse_methods),
};
