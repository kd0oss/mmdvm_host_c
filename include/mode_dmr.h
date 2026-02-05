#pragma once
#include "mode.h"

#ifdef __cplusplus
extern "C" {
#endif

mode_handler_t* dmr_mode_new(void* ctx);

// Cancel any pending echo frame in the DMR mode handler.
void dmr_mode_cancel_tx(mode_handler_t* h);

#ifdef __cplusplus
}
#endif
