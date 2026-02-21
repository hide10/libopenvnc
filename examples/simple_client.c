#include <ovnc/ovnc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int update_count = 0;

static void on_framebuffer_update(ovnc_client_t *client, const ovnc_rect_t *rect)
{
    (void)client;
    printf("  rect: (%u,%u) %ux%u\n", rect->x, rect->y, rect->width, rect->height);
}

static void on_update_finished(ovnc_client_t *client)
{
    update_count++;
    printf("Update #%d finished\n", update_count);

    /* Request next incremental update */
    ovnc_client_request_update(client, NULL, 1);
}

static void on_bell(ovnc_client_t *client)
{
    (void)client;
    printf("Bell!\n");
}

static void on_server_cut_text(ovnc_client_t *client, const char *text,
                               uint32_t length)
{
    (void)client;
    printf("Server clipboard: %.*s\n", (int)length, text);
}

static void on_disconnected(ovnc_client_t *client, ovnc_error_t reason)
{
    (void)client;
    printf("Disconnected: %s\n", ovnc_error_string(reason));
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <host> [port]\n", argv[0]);
        return 1;
    }

    const char *host = argv[1];
    uint16_t port = 5900;
    if (argc >= 3)
        port = (uint16_t)atoi(argv[2]);

    ovnc_encoding_type_t encodings[] = {
        OVNC_ENCODING_RAW,
        OVNC_ENCODING_DESKTOP_SIZE,
        OVNC_ENCODING_CURSOR,
    };

    ovnc_client_config_t config = {
        .host = host,
        .port = port,
        .shared = 1,
        .encodings = encodings,
        .num_encodings = sizeof(encodings) / sizeof(encodings[0]),
    };

    ovnc_client_callbacks_t callbacks = {
        .framebuffer_update = on_framebuffer_update,
        .framebuffer_update_finished = on_update_finished,
        .bell = on_bell,
        .server_cut_text = on_server_cut_text,
        .disconnected = on_disconnected,
    };

    ovnc_client_t *client = ovnc_client_create(&config, &callbacks);
    if (!client) {
        fprintf(stderr, "Failed to create client\n");
        return 1;
    }

    printf("Connecting to %s:%u...\n", host, port);
    ovnc_error_t err = ovnc_client_connect(client);
    if (err != OVNC_OK) {
        fprintf(stderr, "Connect failed: %s\n", ovnc_error_string(err));
        ovnc_client_destroy(client);
        return 1;
    }

    ovnc_connection_info_t info;
    ovnc_client_get_connection_info(client, &info);
    printf("Connected: %s (%ux%u) RFB %d.%d\n",
           info.name, info.width, info.height,
           info.rfb_version / 10, info.rfb_version % 10);

    /* Request initial full update */
    ovnc_client_request_update(client, NULL, 0);

    /* Process messages until disconnection */
    ovnc_client_run(client);

    ovnc_client_destroy(client);
    printf("Total updates received: %d\n", update_count);
    return 0;
}
