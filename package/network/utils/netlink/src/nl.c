#include "nl.h"
#include <linux/genetlink.h>
#include <linux/netlink.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#define NL_BUFSZ 8192

struct nl_get_ctx {
    const struct nl_cmd *cmd;
    struct blob_buf *b;
};

/* Map attr to nl_field */
static const struct nl_field *
nl_field_by_attr(const struct nl_field *f, uint16_t attr)
{
    for (; f && f->name; f++)
        if (f->attr == attr)
            return f;
    return NULL;
}

/* Decode attribute into blobmsg */
static void nl_decode_attr(struct blob_buf *b,
                           const struct nl_field *f,
                           const struct nlattr *attr)
{
    switch (f->type) {
    case NL_T_U8:
        blobmsg_add_u8(b, f->name, mnl_attr_get_u8(attr));
        break;
    case NL_T_U16:
        blobmsg_add_u16(b, f->name, mnl_attr_get_u16(attr));
        break;
    case NL_T_U32:
    case NL_T_ENUM:
    case NL_T_BOOL:
        blobmsg_add_u32(b, f->name, mnl_attr_get_u32(attr));
        break;
    case NL_T_STRING:
        blobmsg_add_string(b, f->name, mnl_attr_get_str(attr));
        break;
    }
}

/* GET callback */
static int nl_get_cb(const struct nlmsghdr *nlh, void *data)
{
    struct nl_get_ctx *ctx = data;
    struct genlmsghdr *genl = mnl_nlmsg_get_payload(nlh);
    struct nlattr *attr;

    void *t = blobmsg_open_table(ctx->b, "data");

    mnl_attr_for_each(attr, nlh, sizeof(*genl)) {
        const struct nl_field *f = nl_field_by_attr(ctx->cmd->fields,
                                                     mnl_attr_get_type(attr));
        if (!f || !(f->dir & NL_F_GET))
            continue;

        nl_decode_attr(ctx->b, f, attr);
    }

    blobmsg_close_table(ctx->b, t);
    return MNL_CB_OK;
}

int nl_do_get(struct mnl_socket *nl,
              const struct nl_family *fam,
              const struct nl_cmd *cmd,
              uint16_t header_attr,
              const char *ifname,
              struct blob_buf *reply)
{
    char buf[NL_BUFSZ];
    struct nlmsghdr *nlh;
    struct genlmsghdr *genl;
    struct nl_get_ctx ctx = {
        .cmd = cmd,
        .b   = reply,
    };

    blob_buf_init(reply, 0);

    nlh = mnl_nlmsg_put_header(buf);
    nlh->nlmsg_type  = fam->id;
    nlh->nlmsg_flags = cmd->flags;
    nlh->nlmsg_seq   = 1;

    genl = mnl_nlmsg_put_extra_header(nlh, sizeof(*genl));
    genl->cmd = cmd->cmd;

    mnl_attr_put_strz(nlh, header_attr, ifname);

    if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0)
        return -errno;

    if (mnl_socket_recvfrom(nl, buf, sizeof(buf)) <= 0)
        return -errno;

    return mnl_cb_run(buf, sizeof(buf), 1, 0, nl_get_cb, &ctx);
}

/* SET helper (sparse) */
int nl_do_set(struct mnl_socket *nl,
              const struct nl_family *fam,
              const struct nl_cmd *cmd,
              uint16_t header_attr,
              const char *ifname,
              const struct nl_field *fields,
              const void **values)
{
    char buf[NL_BUFSZ];
    struct nlmsghdr *nlh;
    struct genlmsghdr *genl;

    nlh = mnl_nlmsg_put_header(buf);
    nlh->nlmsg_type  = fam->id;
    nlh->nlmsg_flags = cmd->flags;
    nlh->nlmsg_seq   = 1;

    genl = mnl_nlmsg_put_extra_header(nlh, sizeof(*genl));
    genl->cmd = cmd->cmd;

    mnl_attr_put_strz(nlh, header_attr, ifname);

    /* Add only provided fields */
    for (int i = 0; fields[i].name; i++) {
        if (!values[i])
            continue;

        switch (fields[i].type) {
        case NL_T_U8:    mnl_attr_put_u8(nlh, fields[i].attr, *(uint8_t*)values[i]); break;
        case NL_T_U16:   mnl_attr_put_u16(nlh, fields[i].attr, *(uint16_t*)values[i]); break;
        case NL_T_U32:
        case NL_T_ENUM:  mnl_attr_put_u32(nlh, fields[i].attr, *(uint32_t*)values[i]); break;
        case NL_T_STRING:mnl_attr_put_strz(nlh, fields[i].attr, (const char*)values[i]); break;
        default: break;
        }
    }

    if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0)
        return -errno;

    if (mnl_socket_recvfrom(nl, buf, sizeof(buf)) <= 0)
        return -errno;

    return 0;
}

/* DUMP helper */
struct nl_dump_ctx {
    const struct nl_cmd *cmd;
    struct blob_buf *b;
};

static int nl_dump_cb(const struct nlmsghdr *nlh, void *data)
{
    struct nl_dump_ctx *ctx = data;
    struct genlmsghdr *genl = mnl_nlmsg_get_payload(nlh);
    struct nlattr *attr;

    void *t = blobmsg_open_table(ctx->b, "data");

    mnl_attr_for_each(attr, nlh, sizeof(*genl)) {
        const struct nl_field *f = nl_field_by_attr(ctx->cmd->fields,
                                                     mnl_attr_get_type(attr));
        if (!f || !(f->dir & NL_F_DUMP))
            continue;
        nl_decode_attr(ctx->b, f, attr);
    }

    blobmsg_close_table(ctx->b, t);
    return MNL_CB_OK;
}

int nl_do_dump(struct mnl_socket *nl,
               const struct nl_family *fam,
               const struct nl_cmd *cmd,
               uint16_t header_attr,
               struct blob_buf *reply)
{
    char buf[NL_BUFSZ];
    struct nlmsghdr *nlh;
    struct genlmsghdr *genl;
    struct nl_dump_ctx ctx = {
        .cmd = cmd,
        .b   = reply,
    };

    blob_buf_init(reply, 0);

    nlh = mnl_nlmsg_put_header(buf);
    nlh->nlmsg_type  = fam->id;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    nlh->nlmsg_seq   = 1;

    genl = mnl_nlmsg_put_extra_header(nlh, sizeof(*genl));
    genl->cmd = cmd->cmd;

    mnl_attr_put_strz(nlh, header_attr, "");

    if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0)
        return -errno;

    int ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
    if (ret <= 0)
        return -errno;

    return mnl_cb_run(buf, ret, 1, 0, nl_dump_cb, &ctx);
}

/* Lookup family ID by name */
struct family_lookup_ctx {
    const char *name;
    uint16_t *id;
};

static int cb_family_name(const struct nlattr *attr, void *data)
{
    struct family_lookup_ctx *ctx = data;
    if (strcmp(ctx->name, mnl_attr_get_str(attr)) == 0) {
        *ctx->id = mnl_attr_get_u16(attr - 1); // CTRL_ATTR_FAMILY_ID
    }
    return MNL_CB_OK;
}

int nl_lookup_family(struct mnl_socket *nl, const char *name, uint16_t *id)
{
    char buf[NL_BUFSZ];
    struct nlmsghdr *nlh;
    struct genlmsghdr *genl;
    struct family_lookup_ctx ctx = {
        .name = name,
        .id   = id,
    };

    nlh = mnl_nlmsg_put_header(buf);
    nlh->nlmsg_type  = GENL_ID_CTRL;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
    nlh->nlmsg_seq   = 1;

    genl = mnl_nlmsg_put_extra_header(nlh, sizeof(*genl));
    genl->cmd = CTRL_CMD_GETFAMILY;

    mnl_attr_put_strz(nlh, CTRL_ATTR_FAMILY_NAME, name);

    if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0)
        return -errno;

    int ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
    if (ret <= 0)
        return -errno;

    /* TODO: iterate attrs, set *id */
    return 0;
}
