#pragma once

#include <stdint.h>
#include <net/if.h>
#include <libmnl/libmnl.h>
#include <libubox/blobmsg.h>

#define NL_F_GET   (1U << 0)
#define NL_F_SET   (1U << 1)
#define NL_F_DUMP  (1U << 2)

enum nl_field_type {
    NL_T_U8,
    NL_T_U16,
    NL_T_U32,
    NL_T_BOOL,
    NL_T_STRING,
    NL_T_ENUM,
};

struct nl_field {
    const char *name;
    uint16_t attr;
    enum nl_field_type type;
    uint32_t dir;
};

struct nl_cmd {
    uint8_t cmd;
    uint16_t flags;
    const struct nl_field *fields;
};

struct nl_family {
    const char *name;
    uint16_t id;
};

/* GET helper */
int nl_do_get(struct mnl_socket *nl,
              const struct nl_family *fam,
              const struct nl_cmd *cmd,
              uint16_t header_attr,
              const char *ifname,
              struct blob_buf *reply);

/* SET helper (sparse update patch-like) */
int nl_do_set(struct mnl_socket *nl,
              const struct nl_family *fam,
              const struct nl_cmd *cmd,
              uint16_t header_attr,
              const char *ifname,
              const struct nl_field *fields,
              const void **values);

/* DUMP helper */
int nl_do_dump(struct mnl_socket *nl,
               const struct nl_family *fam,
               const struct nl_cmd *cmd,
               uint16_t header_attr,
               struct blob_buf *reply);

/* Lookup family ID from kernel by name */
int nl_lookup_family(struct mnl_socket *nl, const char *name, uint16_t *id);
