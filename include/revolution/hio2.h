#ifndef REVOLUTION_HIO2_H
#define REVOLUTION_HIO2_H

#include <revolution/types.h>

typedef s32 HIO2Chan;
typedef BOOL (*HIO2Callback)(HIO2Chan chan);

enum {
    HIO_CHAN_INVALID = -1,
    HIO_CHAN_0 = 0,
    HIO_CHAN_1,
    HIO_CHAN_MAX
};

enum {
    HIO2_ERR_OK = 0,
    HIO2_ERR_NOT_INITIALIZED,
    HIO2_ERR_INVALID_CHAN_HANDLE,
    HIO2_ERR_INVALID_EXI_CHAN,
    HIO2_ERR_ALREADY_OPEN,
    HIO2_ERR_CANNOT_ENUM_DEVICES,
    HIO2_ERR_NOT_AVAILABLE,
    HIO2_ERR_MISSING,
    HIO2_ERR_EXI,
};

BOOL HIO2Init();

BOOL HIO2EnumDevices(HIO2Callback callback);

HIO2Chan HIO2Open(s32 exiChan, HIO2Callback receiveCallback, HIO2Callback extCallback);

s32 HIO2GetDeviceType(HIO2Chan chan);

BOOL HIO2Close(HIO2Chan chan);

BOOL HIO2ReadMailbox(HIO2Chan chan, u32* mail);
BOOL HIO2WriteMailbox(HIO2Chan chan, u32 mail);

BOOL HIO2Read(HIO2Chan chan, u32 addr, void* buf, s32 size);
BOOL HIO2Write(HIO2Chan chan, u32 addr, void* buf, s32 size);

BOOL HIO2ReadAsync(HIO2Chan chan, u32 addr, void* buf, s32 size, HIO2Callback callback);
BOOL HIO2WriteAsync(HIO2Chan chan, u32 addr, void* buf, s32 size, HIO2Callback callback);

BOOL HIO2ReadStatus(HIO2Chan chan, s32* status);

void HIO2Exit();

s32 HIO2GetLastError();

HIO2Callback HIO2GetReceiveCallback(HIO2Chan chan);
BOOL HIO2SetReceiveCallback(HIO2Chan chan, HIO2Callback receiveCallback);

#endif  // REVOLUTION_HIO2_H
