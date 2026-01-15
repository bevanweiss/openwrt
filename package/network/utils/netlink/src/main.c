#include <libubus.h>
#include <libubox/uloop.h>
#include "ubus.h"

int main(void)
{
    struct ubus_context *ctx;

    uloop_init();

    ctx = ubus_connect(NULL);
    if (!ctx)
        return 1;

    ubus_register_all(ctx);

    /* Integrate ubus with uloop */
    ubus_add_uloop(ctx);
    uloop_run();

    ubus_free(ctx);
    uloop_done();
    return 0;
}
