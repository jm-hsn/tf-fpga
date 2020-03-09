#include "../include/modules.hpp"

#define MOD_DEF( identifier, id, name )  id
const uint32_t moduleIds[] = { MODS_DEF };
#undef MOD_DEF

#define MOD_DEF( identifier, id, name )  name
const char *moduleNames[] = { MODS_DEF };
#undef MOD_DEF
