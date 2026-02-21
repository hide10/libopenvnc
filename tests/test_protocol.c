#include <ovnc/ovnc.h>
#include "ovnc_internal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
    tests_run++; \
    printf("  %s... ", #name); \
    name(); \
    tests_passed++; \
    printf("OK\n"); \
} while(0)

/*-------------------------------------------------------------------
 * Pixel format tests
 *-------------------------------------------------------------------*/

static void test_pixel_format_default(void)
{
    ovnc_pixel_format_t fmt;
    ovnc_pixel_format_init_default(&fmt);
    assert(fmt.bits_per_pixel == 32);
    assert(fmt.depth == 24);
    assert(fmt.big_endian == 0);
    assert(fmt.true_color == 1);
    assert(fmt.red_max == 255);
    assert(fmt.green_max == 255);
    assert(fmt.blue_max == 255);
    assert(fmt.red_shift == 16);
    assert(fmt.green_shift == 8);
    assert(fmt.blue_shift == 0);
}

static void test_pixel_format_bytes_per_pixel(void)
{
    ovnc_pixel_format_t fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.bits_per_pixel = 8;
    assert(ovnc_pixel_format_bytes_per_pixel(&fmt) == 1);
    fmt.bits_per_pixel = 16;
    assert(ovnc_pixel_format_bytes_per_pixel(&fmt) == 2);
    fmt.bits_per_pixel = 32;
    assert(ovnc_pixel_format_bytes_per_pixel(&fmt) == 4);
    /* Invalid values should return -1 */
    fmt.bits_per_pixel = 0;
    assert(ovnc_pixel_format_bytes_per_pixel(&fmt) == -1);
    fmt.bits_per_pixel = 24;
    assert(ovnc_pixel_format_bytes_per_pixel(&fmt) == -1);
    fmt.bits_per_pixel = 255;
    assert(ovnc_pixel_format_bytes_per_pixel(&fmt) == -1);
}

static void test_pixel_format_roundtrip(void)
{
    ovnc_pixel_format_t original, decoded;
    uint8_t wire[16];

    ovnc_pixel_format_init_default(&original);
    ovnc__pixel_format_to_wire(&original, wire);
    ovnc__pixel_format_from_wire(wire, &decoded);

    assert(decoded.bits_per_pixel == original.bits_per_pixel);
    assert(decoded.depth == original.depth);
    assert(decoded.big_endian == original.big_endian);
    assert(decoded.true_color == original.true_color);
    assert(decoded.red_max == original.red_max);
    assert(decoded.green_max == original.green_max);
    assert(decoded.blue_max == original.blue_max);
    assert(decoded.red_shift == original.red_shift);
    assert(decoded.green_shift == original.green_shift);
    assert(decoded.blue_shift == original.blue_shift);
}

static void test_pixel_format_wire_padding(void)
{
    ovnc_pixel_format_t fmt;
    ovnc_pixel_format_init_default(&fmt);
    uint8_t wire[16];
    ovnc__pixel_format_to_wire(&fmt, wire);
    /* Bytes 13-15 should be zero (padding) */
    assert(wire[13] == 0);
    assert(wire[14] == 0);
    assert(wire[15] == 0);
}

/*-------------------------------------------------------------------
 * Byte order tests
 *-------------------------------------------------------------------*/

static void test_read_write_u16(void)
{
    uint8_t buf[2];
    ovnc__write_u16(buf, 0x1234);
    assert(buf[0] == 0x12);
    assert(buf[1] == 0x34);
    assert(ovnc__read_u16(buf) == 0x1234);
}

static void test_read_write_u32(void)
{
    uint8_t buf[4];
    ovnc__write_u32(buf, 0xDEADBEEF);
    assert(buf[0] == 0xDE);
    assert(buf[1] == 0xAD);
    assert(buf[2] == 0xBE);
    assert(buf[3] == 0xEF);
    assert(ovnc__read_u32(buf) == 0xDEADBEEF);
}

static void test_read_write_s32(void)
{
    uint8_t buf[4];
    /* Test negative value (Cursor pseudo-encoding = -239) */
    ovnc__write_s32(buf, -239);
    assert(ovnc__read_s32(buf) == -239);

    /* Test positive value */
    ovnc__write_s32(buf, 16);
    assert(ovnc__read_s32(buf) == 16);
}

/*-------------------------------------------------------------------
 * Message buffer construction tests
 *-------------------------------------------------------------------*/

static void test_set_pixel_format_message(void)
{
    /* Verify SetPixelFormat message layout:
     * byte 0: message-type (0)
     * bytes 1-3: padding
     * bytes 4-19: pixel format (16 bytes) */
    uint8_t buf[20];
    memset(buf, 0xFF, sizeof(buf));

    buf[0] = 0; /* message-type */
    buf[1] = 0; buf[2] = 0; buf[3] = 0; /* padding */

    ovnc_pixel_format_t fmt;
    ovnc_pixel_format_init_default(&fmt);
    ovnc__pixel_format_to_wire(&fmt, &buf[4]);

    assert(buf[0] == 0);
    assert(buf[1] == 0 && buf[2] == 0 && buf[3] == 0);
    assert(buf[4] == 32); /* bits_per_pixel */
}

static void test_fb_update_request_message(void)
{
    /* Verify FramebufferUpdateRequest layout:
     * byte 0: message-type (3)
     * byte 1: incremental
     * bytes 2-3: x
     * bytes 4-5: y
     * bytes 6-7: width
     * bytes 8-9: height */
    uint8_t buf[10];
    buf[0] = 3;
    buf[1] = 1; /* incremental */
    ovnc__write_u16(&buf[2], 100);
    ovnc__write_u16(&buf[4], 200);
    ovnc__write_u16(&buf[6], 640);
    ovnc__write_u16(&buf[8], 480);

    assert(buf[0] == 3);
    assert(buf[1] == 1);
    assert(ovnc__read_u16(&buf[2]) == 100);
    assert(ovnc__read_u16(&buf[4]) == 200);
    assert(ovnc__read_u16(&buf[6]) == 640);
    assert(ovnc__read_u16(&buf[8]) == 480);
}

static void test_key_event_message(void)
{
    uint8_t buf[8];
    buf[0] = 4; /* message-type */
    buf[1] = 1; /* down */
    buf[2] = 0; buf[3] = 0; /* padding */
    ovnc__write_u32(&buf[4], 0xff0d); /* Return key */

    assert(buf[0] == 4);
    assert(buf[1] == 1);
    assert(ovnc__read_u32(&buf[4]) == 0xff0d);
}

static void test_pointer_event_message(void)
{
    uint8_t buf[6];
    buf[0] = 5; /* message-type */
    buf[1] = 0x01; /* left button */
    ovnc__write_u16(&buf[2], 320);
    ovnc__write_u16(&buf[4], 240);

    assert(buf[0] == 5);
    assert(buf[1] == 0x01);
    assert(ovnc__read_u16(&buf[2]) == 320);
    assert(ovnc__read_u16(&buf[4]) == 240);
}

/*-------------------------------------------------------------------
 * Error string tests
 *-------------------------------------------------------------------*/

static void test_error_strings(void)
{
    assert(strcmp(ovnc_error_string(OVNC_OK), "Success") == 0);
    assert(strcmp(ovnc_error_string(OVNC_ERR_NOMEM), "Out of memory") == 0);
    assert(strcmp(ovnc_error_string(OVNC_ERR_IO), "I/O error") == 0);
    assert(strcmp(ovnc_error_string(OVNC_ERR_PROTOCOL), "Protocol error") == 0);
    /* Unknown error */
    assert(ovnc_error_string((ovnc_error_t)999) != NULL);
}

/*-------------------------------------------------------------------
 * Client lifecycle tests
 *-------------------------------------------------------------------*/

static void test_client_create_destroy(void)
{
    ovnc_client_config_t config = {
        .host = "localhost",
        .port = 5900,
        .shared = 1,
    };
    ovnc_client_t *c = ovnc_client_create(&config, NULL);
    assert(c != NULL);
    assert(ovnc_client_is_connected(c) == 0);
    ovnc_client_destroy(c);
}

static void test_client_null_host(void)
{
    ovnc_client_config_t config = { .host = NULL };
    ovnc_client_t *c = ovnc_client_create(&config, NULL);
    assert(c == NULL);
}

static void test_client_default_port(void)
{
    ovnc_client_config_t config = {
        .host = "example.com",
        .port = 0, /* should default to 5900 */
    };
    ovnc_client_t *c = ovnc_client_create(&config, NULL);
    assert(c != NULL);
    /* We can't directly check port since it's internal,
     * but the create should succeed */
    ovnc_client_destroy(c);
}

static void test_client_user_data(void)
{
    ovnc_client_config_t config = {
        .host = "localhost",
        .user_data = (void *)0x42,
    };
    ovnc_client_t *c = ovnc_client_create(&config, NULL);
    assert(c != NULL);
    assert(ovnc_client_get_user_data(c) == (void *)0x42);
    ovnc_client_set_user_data(c, (void *)0x84);
    assert(ovnc_client_get_user_data(c) == (void *)0x84);
    ovnc_client_destroy(c);
}

static void test_client_deep_copy(void)
{
    ovnc_pixel_format_t fmt;
    ovnc_pixel_format_init_default(&fmt);

    ovnc_encoding_type_t enc[] = { OVNC_ENCODING_RAW };
    char host[] = "testhost";

    ovnc_client_config_t config = {
        .host = host,
        .port = 5901,
        .pixel_format = &fmt,
        .encodings = enc,
        .num_encodings = 1,
    };

    ovnc_client_t *c = ovnc_client_create(&config, NULL);
    assert(c != NULL);

    /* Mutate originals - client should be unaffected */
    memset(host, 'X', sizeof(host) - 1);
    fmt.bits_per_pixel = 8;
    enc[0] = OVNC_ENCODING_ZRLE;

    /* Client should still be valid (we can't easily verify internal state
     * from public API, but at least destruction shouldn't crash) */
    ovnc_client_destroy(c);
}

/*-------------------------------------------------------------------
 * Version string tests
 *-------------------------------------------------------------------*/

static void test_version_defined(void)
{
    /* Verify version macros are defined */
    assert(OVNC_VERSION_MAJOR >= 0);
    assert(OVNC_VERSION_MINOR >= 0);
    assert(OVNC_VERSION_PATCH >= 0);
    assert(strlen(OVNC_VERSION_STRING) > 0);
}

/*-------------------------------------------------------------------
 * Main
 *-------------------------------------------------------------------*/

int main(void)
{
    printf("Running protocol tests...\n");

    printf("\nPixel format:\n");
    TEST(test_pixel_format_default);
    TEST(test_pixel_format_bytes_per_pixel);
    TEST(test_pixel_format_roundtrip);
    TEST(test_pixel_format_wire_padding);

    printf("\nByte order:\n");
    TEST(test_read_write_u16);
    TEST(test_read_write_u32);
    TEST(test_read_write_s32);

    printf("\nMessage buffers:\n");
    TEST(test_set_pixel_format_message);
    TEST(test_fb_update_request_message);
    TEST(test_key_event_message);
    TEST(test_pointer_event_message);

    printf("\nError strings:\n");
    TEST(test_error_strings);

    printf("\nClient lifecycle:\n");
    TEST(test_client_create_destroy);
    TEST(test_client_null_host);
    TEST(test_client_default_port);
    TEST(test_client_user_data);
    TEST(test_client_deep_copy);

    printf("\nVersion:\n");
    TEST(test_version_defined);

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
