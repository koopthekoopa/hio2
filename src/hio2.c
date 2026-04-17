#include <revolution/hio2.h>

#include <revolution/exi.h>
#include <revolution/os.h>

/*
    DEBUG SCRATCH: https://decomp.me/scratch/KLt0W
    RELEASE SCRATCH: https://decomp.me/scratch/aHKHZ
*/

s32 __HIO2LastErrorCode = HIO2_ERR_OK;
BOOL __HIO2Initialized = FALSE;

s32 __HIO2ConsoleType = 0;

#ifdef DEBUG
static const char* __HIO2Version = "<< RVL_SDK - HIO2 	debug build: Jul 30 2008 19:02:00 (0x4199_60831) >>";
#else
static const char* __HIO2Version = "<< RVL_SDK - HIO2 	release build: Jul 30 2008 19:24:26 (0x4199_60831) >>";
#endif

typedef struct HIO2ChanInfo {
    s32 exiChan;
    HIO2Chan chan;
    s32 dev;
    HIO2Callback receiveCallback;
    HIO2Callback txCallback;
    HIO2Callback rxCallback;
    HIO2Callback extCallback;
} HIO2ChanInfo;

#define IS_BAD_CHAN(chan) (!__HIO2IsInitialized() || !__HIO2IsValidHandle(chan))

HIO2ChanInfo __HIO2Control[HIO_CHAN_MAX] = {
    {EXI_CHAN_INVALID, HIO_CHAN_INVALID, 0, NULL, NULL, NULL, NULL},
    {EXI_CHAN_INVALID, HIO_CHAN_INVALID, 0, NULL, NULL, NULL, NULL},
};

static BOOL __HIO2IsInitialized();
static BOOL __HIO2IsValidHandle(HIO2Chan chan);

static void __HIO2ClearChanInfo(HIO2Chan chan) {
    __HIO2Control[chan].exiChan = EXI_CHAN_INVALID;
    __HIO2Control[chan].chan = HIO_CHAN_INVALID;
    __HIO2Control[chan].dev = 0;
    __HIO2Control[chan].receiveCallback = __HIO2Control[chan].txCallback = __HIO2Control[chan].rxCallback = __HIO2Control[chan].extCallback = NULL;
}

static void __HIO2ExtHandler(HIO2Chan chan) {
    if (__HIO2Control[chan].extCallback != NULL) {
        __HIO2Control[chan].extCallback(chan);
    }
    __HIO2ClearChanInfo(chan);
    EXISetExiCallback(chan, NULL);
}

static void __HIO2ExiHandler(HIO2Chan chan) {
    if (chan == EXI_CHAN_2) {
        chan = EXI_CHAN_0;
    }
    if (__HIO2Control[chan].receiveCallback != NULL) {
        __HIO2Control[chan].receiveCallback(chan);
    }
}

static void __HIO2TxHandler(HIO2Chan chan) {
    EXIDeselect(chan);
    EXIUnlock(chan);
    if (__HIO2Control[chan].txCallback != NULL) {
        __HIO2Control[chan].txCallback(chan);
    }
}

static void __HIO2RxHandler(HIO2Chan chan) {
    EXIDeselect(chan);
    EXIUnlock(chan);
    if (__HIO2Control[chan].rxCallback != NULL) {
        __HIO2Control[chan].rxCallback(chan);
    }
}

BOOL HIO2Init() {
    if (__HIO2Initialized) {
        HIO2Exit();
    }
    EXIWait();
    __HIO2ConsoleType = EXIGetConsoleType();
    if ((__HIO2ConsoleType & 7) == 0) {
        __HIO2LastErrorCode = HIO2_ERR_NOT_AVAILABLE;
        return FALSE;
    } else {
        __HIO2LastErrorCode = HIO2_ERR_OK;
        OSRegisterVersion(__HIO2Version);
        return __HIO2Initialized = TRUE;
    }
}

BOOL HIO2EnumDevices(HIO2Callback callback) {
    u32 id;
    HIO2Chan chan;

    ASSERTLINE(275, callback != NULL);

    if (!__HIO2IsInitialized()) {
        return FALSE;
    }
    if (__HIO2Control[HIO_CHAN_0].chan != HIO_CHAN_INVALID && __HIO2Control[HIO_CHAN_1].chan != HIO_CHAN_INVALID) {
        __HIO2LastErrorCode = HIO2_ERR_CANNOT_ENUM_DEVICES;
        return FALSE;
    }
    for (chan = 0; chan < HIO_CHAN_MAX; chan++) {
        while (!EXIProbeEx(chan)) {
        }
        if (EXIGetID(chan, 0, &id) && id == 0 && !callback(chan)) {
            return TRUE;
        }
    }
    if ((__HIO2ConsoleType & 1) != 0) {
        callback(EXI_CHAN_2);
    }
    return TRUE;
}

static BOOL __HIO2IsInitialized() {
    if (!__HIO2Initialized) {
        __HIO2LastErrorCode = HIO2_ERR_NOT_INITIALIZED;
    }
    return __HIO2Initialized;
}

HIO2Chan HIO2Open(s32 exiChan, HIO2Callback receiveCallback, HIO2Callback extCallback) {
    HIO2Chan chan;
    u32 dev;

    if (!__HIO2IsInitialized()) {
        return HIO_CHAN_INVALID;
    }
    switch (exiChan) {
        case EXI_CHAN_0:
        case EXI_CHAN_1: {
            chan = exiChan;
            dev = 0;
            break;
        }
        case EXI_CHAN_2: {
            if ((__HIO2ConsoleType & 1) != 0) {
                chan = HIO_CHAN_0;
                dev = 1;
            } else {
                __HIO2LastErrorCode = HIO2_ERR_NOT_AVAILABLE;
                return HIO_CHAN_INVALID;
            }
            break;
        }
        default: {
            __HIO2LastErrorCode = HIO2_ERR_INVALID_EXI_CHAN;
            return HIO_CHAN_INVALID;
        }
    }
    if (__HIO2Control[chan].chan != HIO_CHAN_INVALID) {
        __HIO2LastErrorCode = HIO2_ERR_ALREADY_OPEN;
        return HIO_CHAN_INVALID;
    } else {
        u8 reg;
        BOOL enabled;
        if (dev == 0) {
            u32 id = 0;
            while (!EXIProbeEx(chan)) {
            }
            if (!EXIAttach(chan, __HIO2ExtHandler) || !EXIGetID(chan, 0, &id) || id != 0) {
                __HIO2LastErrorCode = HIO2_ERR_MISSING;
                return HIO_CHAN_INVALID;
            }
        }
        reg = 0xD8;
        enabled = OSDisableInterrupts();
        if (!EXIWriteReg(chan, dev, 0xB4000000, (u32*)&reg, 1)) {
            if (dev == 0) {
                EXIDetach(chan);
            }
            __HIO2LastErrorCode = HIO2_ERR_EXI;
            OSRestoreInterrupts(enabled);
            return HIO_CHAN_INVALID;
        }
        OSRestoreInterrupts(enabled);
        __HIO2Control[chan].exiChan = exiChan;
        __HIO2Control[chan].dev = dev;
        __HIO2Control[chan].chan = chan;
        __HIO2Control[chan].receiveCallback = receiveCallback;
        __HIO2Control[chan].txCallback = NULL;
        __HIO2Control[chan].rxCallback = NULL;
        __HIO2Control[chan].extCallback = extCallback;
        if (receiveCallback != NULL) {
            if (exiChan == 2) {
                EXISetExiCallback(2, __HIO2ExiHandler);
            } else {
                EXISetExiCallback(chan, __HIO2ExiHandler);
            }
        }
    }

    return chan;
}

s32 HIO2GetDeviceType(HIO2Chan chan) {
    if (IS_BAD_CHAN(chan)) {
        return -1;
    } else {
        return __HIO2Control[chan].dev == 0 ? __HIO2Control[chan].chan : 2;
    }
}

static BOOL __HIO2IsValidHandle(HIO2Chan chan) {
    if ((chan == HIO_CHAN_0 || chan == HIO_CHAN_1) && __HIO2Control[chan].chan != HIO_CHAN_INVALID) {
        return TRUE;
    } else {
        __HIO2LastErrorCode = HIO2_ERR_INVALID_CHAN_HANDLE;
        return FALSE;
    }
}

BOOL HIO2Close(HIO2Chan chan) {
    if (IS_BAD_CHAN(chan)) {
        return FALSE;
    } else {
        EXISetExiCallback(__HIO2Control[chan].chan, NULL);
        if (__HIO2Control[chan].dev == 0) {
            EXIDetach(__HIO2Control[chan].chan);
        }
        __HIO2ClearChanInfo(chan);
    }
    return TRUE;
}

BOOL HIO2ReadMailbox(HIO2Chan chan, u32* mail) {
    BOOL result;

    if (IS_BAD_CHAN(chan)) {
        return FALSE;
    } else {
        ASSERTLINE(509, mail != NULL);
        result = EXIReadReg(chan, __HIO2Control[chan].dev, 0x34000200, mail, 4);
        if (!result) {
            __HIO2LastErrorCode = HIO2_ERR_EXI;
        }
    }
    return result;
}

BOOL HIO2WriteMailbox(HIO2Chan chan, u32 mail) {
    BOOL result;

    if (IS_BAD_CHAN(chan)) {
        return FALSE;
    } else {
        result = EXIWriteReg(chan, __HIO2Control[chan].dev, 0xB4000100, &mail, 4);
        if (!result) {
            __HIO2LastErrorCode = HIO2_ERR_EXI;
        }
    }
    return result;
}

BOOL HIO2Read(HIO2Chan chan, u32 addr, void* buf, s32 size) {
    BOOL result;

    if (IS_BAD_CHAN(chan)) {
        return FALSE;
    } else {
        ASSERTLINE(564, (addr & 3) == 0);
        result = EXIReadRam(chan, __HIO2Control[chan].dev, ((addr + 0xD10000) << 6) & 0x3FFFFF00, buf, size, NULL);
        if (!result) {
            __HIO2LastErrorCode = HIO2_ERR_EXI;
        }
    }
    return result;
}

BOOL HIO2Write(HIO2Chan chan, u32 addr, void* buf, s32 size) {
    BOOL result;

    if (IS_BAD_CHAN(chan)) {
        return FALSE;
    } else {
        ASSERTLINE(593, (addr & 3) == 0);
        result = EXIWriteRam(chan, __HIO2Control[chan].dev, (((addr + 0xD10000) << 6) & 0x3FFFFF00) | 0x80000000, buf, size, NULL);
        if (!result) {
            __HIO2LastErrorCode = HIO2_ERR_EXI;
        }
    }
    return result;
}

BOOL HIO2ReadAsync(HIO2Chan chan, u32 addr, void* buf, s32 size, HIO2Callback callback) {
    BOOL result;

    if (IS_BAD_CHAN(chan)) {
        return FALSE;
    } else {
        ASSERTLINE(624, (addr & 3) == 0);
        __HIO2Control[chan].rxCallback = callback;
        result = EXIReadRam(chan, __HIO2Control[chan].dev, ((addr + 0xD10000) << 6) & 0x3FFFFF00, buf, size, __HIO2RxHandler);
        if (!result) {
            __HIO2LastErrorCode = HIO2_ERR_EXI;
        }
    }
    return result;
}

BOOL HIO2WriteAsync(HIO2Chan chan, u32 addr, void* buf, s32 size, HIO2Callback callback) {
    BOOL result;

    if (IS_BAD_CHAN(chan)) {
        return FALSE;
    } else {
        ASSERTLINE(658, (addr & 3) == 0);
        __HIO2Control[chan].txCallback = callback;
        result = EXIWriteRam(chan, __HIO2Control[chan].dev, (((addr + 0xD10000) << 6) & 0x3FFFFF00) | 0x80000000, buf, size, __HIO2TxHandler);
        if (!result) {
            __HIO2LastErrorCode = HIO2_ERR_EXI;
        }
    }
    return result;
}

BOOL HIO2ReadStatus(HIO2Chan chan, s32* status) {
    BOOL result;

    if (IS_BAD_CHAN(chan)) {
        return FALSE;
    } else {
        u8 reg;
        ASSERTLINE(688, status != NULL);
        result = EXIReadReg(chan, __HIO2Control[chan].dev, 0x34000000, (u32*)&reg, 1);
        *status = !(reg & 8) /*== 0*/ ? 1 : 0;
        *status |= !(reg & 4) /*== 0*/ ? 0 : 2;
        *status &= 3;
        if (!result) {
            __HIO2LastErrorCode = HIO2_ERR_EXI;
        }
    }
    return result;
}

void HIO2Exit() {
    HIO2Chan chan;

    for (chan = 0; chan < HIO_CHAN_MAX; chan++) {
        if (__HIO2Control[chan].chan != HIO_CHAN_INVALID) {
            HIO2Close(chan);
        }
    }
    __HIO2Initialized = FALSE;
    __HIO2ConsoleType = 0;
}

s32 HIO2GetLastError() {
    return __HIO2LastErrorCode;
}

HIO2Callback HIO2GetReceiveCallback(HIO2Chan chan) {
    return (IS_BAD_CHAN(chan)) ? NULL : __HIO2Control[chan].receiveCallback;
}

BOOL HIO2SetReceiveCallback(HIO2Chan chan, HIO2Callback receiveCallback) {
    BOOL enabled;

    if (IS_BAD_CHAN(chan)) {
        return FALSE;
    } else {
        enabled = OSDisableInterrupts();
        __HIO2Control[chan].receiveCallback = receiveCallback;
        OSRestoreInterrupts(enabled);
    }
    return TRUE;
}
