#include <clap/clap.h>

#include "plugin-entry.h"

extern "C" {
CLAP_EXPORT const clap_plugin_entry clap_entry = {CLAP_VERSION_INIT, sps_entry_init, sps_entry_deinit,
						  sps_entry_get_factory};
}
