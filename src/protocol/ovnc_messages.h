#ifndef OVNC_MESSAGES_H
#define OVNC_MESSAGES_H

#include "ovnc_internal.h"
#include "transport/ovnc_transport.h"

/*-------------------------------------------------------------------
 * Client -> Server messages
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc__send_set_pixel_format(ovnc_client_t *client,
                                         const ovnc_pixel_format_t *fmt);

ovnc_error_t ovnc__send_set_encodings(ovnc_client_t *client,
                                      const ovnc_encoding_type_t *encodings,
                                      size_t num_encodings);

ovnc_error_t ovnc__send_fb_update_request(ovnc_client_t *client,
                                          uint8_t incremental,
                                          uint16_t x, uint16_t y,
                                          uint16_t width, uint16_t height);

ovnc_error_t ovnc__send_key_event(ovnc_client_t *client,
                                  uint32_t key, uint8_t down);

ovnc_error_t ovnc__send_pointer_event(ovnc_client_t *client,
                                      uint16_t x, uint16_t y,
                                      uint8_t button_mask);

ovnc_error_t ovnc__send_client_cut_text(ovnc_client_t *client,
                                        const char *text, uint32_t length);

/*-------------------------------------------------------------------
 * Server -> Client message dispatch
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc__recv_server_message(ovnc_client_t *client);

#endif /* OVNC_MESSAGES_H */
