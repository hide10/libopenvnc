#ifndef OVNC_ENCODING_H
#define OVNC_ENCODING_H

#include "ovnc_internal.h"

ovnc_error_t ovnc__decode_rect(ovnc_client_t *client,
                               const ovnc_rect_t *rect,
                               int32_t encoding_type);

ovnc_error_t ovnc__decode_raw(ovnc_client_t *client,
                              const ovnc_rect_t *rect);

#endif /* OVNC_ENCODING_H */
