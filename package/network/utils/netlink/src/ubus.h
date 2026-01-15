#pragma once
#include <libubus.h>

/* Generic function to register all netlink topics */
int ubus_register_all(struct ubus_context *ctx);
