#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Error bit and special values
#define BUTTONDI_ERROR_BIT (1ULL << 63)
#define BUTTONDI_ERROR_INVALID_HANDLE (BUTTONDI_ERROR_BIT | 1ULL)
#define BUTTONDI_ERROR_READ_FAILED (BUTTONDI_ERROR_BIT | 2ULL)

// Helper macro to check for errors
#define IS_BUTTONDI_ERROR(x) ((x) & BUTTONDI_ERROR_BIT)

__declspec(dllexport) int GetDirectInputDeviceList(char *buffer,
                                                   int bufferSize);

__declspec(dllexport) const char *FindJoystickByVendorAndProductIDAndUsage(
    unsigned short vendorID, unsigned short productID, unsigned short usagePage,
    unsigned short usage);

__declspec(dllexport) const char *
FindJoystickByProductStringAndUsage(const char *name, unsigned short usagePage,
                                    unsigned short usage);

__declspec(dllexport) void *
OpenJoystickByInstanceGUID(const char *instanceGUID);

__declspec(dllexport) uint64_t ReadButtons(void *handle);

__declspec(dllexport) int CloseJoystick(void *handle);

#ifdef __cplusplus
}
#endif
