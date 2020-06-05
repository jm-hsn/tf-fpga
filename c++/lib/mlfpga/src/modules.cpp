#include "../include/modules.hpp"

#define MOD_DEF( identifier, id, name, sendLen, recvLen )  id
const uint32_t moduleIds[] = { MODS_DEF };
#undef MOD_DEF

#define MOD_DEF( identifier, id, name, sendLen, recvLen )  name
const char *moduleNames[] = { MODS_DEF };
#undef MOD_DEF

#define MOD_DEF( identifier, id, name, sendLen, recvLen )  sendLen
const size_t moduleSendPayloadLength[] = { MODS_DEF };
#undef MOD_DEF

#define MOD_DEF( identifier, id, name, sendLen, recvLen )  recvLen
const size_t moduleRecvPayloadLength[] = { MODS_DEF };
#undef MOD_DEF