#ifndef REVOLUTION_EXI_H
#define REVOLUTION_EXI_H

#include <revolution/types.h>

#ifdef __cplusplus
extern "C" {
#endif

s32 EXIAttach(s32, void (*)(s32));
void EXIDeselect(s32);
void EXIDetach(s32);
s32 EXIGetConsoleType();
s32 EXIGetID(s32, u32, u32*);
s32 EXIProbeEx(s32);
s32 EXIReadRam(s32, u32, u32, void*, s32, void (*)(s32));
s32 EXIReadReg(s32, u32, u32, void*, u32)
void EXISetExiCallback(s32, void (*)(s32));
void EXIUnlock(s32);
void EXIWait();
s32 EXIWriteRam(s32, u32, u32, void*, s32, void (*)(s32));
s32 EXIWriteReg(s32, u32, u32, void*, u32);

#ifdef __cplusplus
}
#endif

#endif  // REVOLUTION_EXI_H
