#pragma once

// Implemented in the static library and exported by plugin-entry.cpp, as clap-wrapper's clap-first layout expects.
bool sps_entry_init(const char *pluginPath);
void sps_entry_deinit(void);
const void *sps_entry_get_factory(const char *factoryId);
