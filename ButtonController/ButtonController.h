#pragma once

#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllexport) int FindJoystickByVendorAndProductID(unsigned short vendorID, unsigned short productID);
__declspec(dllexport) int FindJoystickByProductString(const char* name);
__declspec(dllexport) void* OpenJoystick(int joystickId);
__declspec(dllexport) unsigned int ReadButtons(void* handle);
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