#include <ovnc/ovnc_common.h>

const char* ovnc_error_string(ovnc_error_t error)
{
    switch (error) {
    case OVNC_OK:                      return "Success";
    case OVNC_ERR_NOMEM:               return "Out of memory";
    case OVNC_ERR_INVALID_ARG:         return "Invalid argument";
    case OVNC_ERR_IO:                  return "I/O error";
    case OVNC_ERR_TIMEOUT:             return "Timeout";
    case OVNC_ERR_CONNECTION_REFUSED:   return "Connection refused";
    case OVNC_ERR_CONNECTION_CLOSED:    return "Connection closed";
    case OVNC_ERR_VERSION_MISMATCH:    return "RFB version mismatch";
    case OVNC_ERR_AUTH_FAILED:         return "Authentication failed";
    case OVNC_ERR_AUTH_UNSUPPORTED:    return "Unsupported authentication type";
    case OVNC_ERR_PROTOCOL:            return "Protocol error";
    case OVNC_ERR_ENCODING_UNSUPPORTED: return "Unsupported encoding";
    case OVNC_ERR_ZLIB:               return "zlib error";
    }
    return "Unknown error";
}
