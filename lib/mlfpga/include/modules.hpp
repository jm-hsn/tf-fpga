#ifndef myMODULES_H
#define myMODULES_H

#include <iostream>
#include <sys/types.h>

#define MODS_DEF \
  MOD_DEF( dummyModule,  0xf218e0a2, "Dummy 4" ),  \
  MOD_DEF( conv2D_2x11_Module, 0x9323eb24, "2D Konvolution 2x11" ),   \
  MOD_DEF( neuronModule, 0x03b30000, "Neuron" ), \
  MOD_DEF( dummyBigModule, 0x2cb31e7c, "Dummy 1024"), \
  MOD_DEF( conv2D_5x5_Module, 0x4cd2e19c, "2D Konvolution 5x5")

#define MOD_DEF( identifier, id, name )  identifier
enum { MODS_DEF };
#undef MOD_DEF

extern const uint32_t moduleIds[];
extern const char *moduleNames[];
int main_udp();


#endif