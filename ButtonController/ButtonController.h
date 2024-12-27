#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Maximum report size this library can handle
#define MAX_REPORT_SIZE 8

// Error bit and special values
#define BUTTON_ERROR_BIT              (1ULL << 63)  // Highest bit indicates error
#define BUTTONS_ERROR_INVALID_HANDLE  (BUTTON_ERROR_BIT | 1ULL)
#define BUTTONS_ERROR_READ_FAILED     (BUTTON_ERROR_BIT | 2ULL)
#define BUTTONS_ERROR_OVERSIZED_REPORT (BUTTON_ERROR_BIT | 3ULL)
#define BUTTONS_NO_NEW_DATA           0ULL  // All bits clear indicates no new data

// Helper macro to check for errors
#define IS_BUTTON_ERROR(x) ((x) & BUTTON_ERROR_BIT)

__declspec(dllexport) int FindJoystickByVendorAndProductID(unsigned short vendorID, unsigned short productID);
__declspec(dllexport) int FindJoystickByProductString(const char* name);
__declspec(dllexport) void* OpenJoystick(int joystickId);
__declspec(dllexport) uint64_t ReadButtons(void* handle);
__declspec(dllexport) int CloseJoystick(void* handle);
__declspec(dllexport) int GetHIDDeviceList(char* buffer, int bufferSize);
//------------------------------ debug start ------------------------------
void InitializeLog(const char* logFilePath);
void CloseLog();
void WriteToLog(const char* message);
//------------------------------ debug end ------------------------------
#ifdef __cplusplus
}
#endif