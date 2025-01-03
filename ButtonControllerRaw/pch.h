#pragma once

// Windows headers
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <wtypes.h>

// HID and Setup headers
#include <setupapi.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <hidclass.h>

// Additional common headers that might be needed
#include <devguid.h>
#include <stdint.h>
