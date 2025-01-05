# HID Button Controller Librarys
This repository contains two libraries for interfacing with button controller devices:
- Raw Input Library (ButtonControllerRaw) - Uses Windows Raw Input API
- DirectInput Library (ButtonControllerDirectInput) - Uses DirectInput API

# Raw Input Library

## Features
- Device enumeration with detailed capabilities
- Support for multiple device types
- Device search by vendor/product ID or product string
- Button state reading with error handling
- Event-based button state reporting
- 64-bit button state reporting with error indication

## Device Support
Tested with:
1. USB FS IO (VID: 0x0FC5, PID: 0xB080)
   - 7-byte reports
   - Event-based reporting (reports only state changes)
   - First byte (0xDD) is report ID
   - Button states in byte 1:
     - 0x10: Left button
     - 0x08: Middle button
     - 0x20: Right button
     - Combinations are bitwise OR (e.g., 0x18 for Left+Middle)
   - Remaining bytes (2-6) are always 0x00

2. Three Button Controller (VID: 0x04D8, PID: 0x005E)
   - 3-byte reports
   - Base state is 0xC0
   - Button states in byte 1:
     - 0xD0: Left button (0xC0 | 0x10)
     - 0xC8: Middle button (0xC0 | 0x08)
     - 0xE0: Right button (0xC0 | 0x20)

3. Kinesis JoyStick Controller (VID: 0x0FC5, PID: 0xB030)
   - 4-byte reports
   - Button states in byte 3:
     - 0x01: Button 1
     - 0x02: Button 2
     - 0x04: Button 3
     - Combinations are bitwise OR (e.g., 0x03 for Buttons 1+2)

## Requirements
- Visual Studio 2022
- Windows SDK
- C++17 or later
- Windows platform with HID support

## API Reference

### GetHIDDeviceList
`int GetHIDDeviceList(char* buffer, int bufferSize)`
Returns JSON-formatted list of available HID devices with their capabilities.
Returns 0 on success, negative values for errors.

### FindJoystickByVendorAndProductID
`int FindJoystickByVendorAndProductID(unsigned short vendorID, unsigned short productID)`
Returns device index or -1 if device not found.

### FindJoystickByProductString
`int FindJoystickByProductString(const char* name)`
Returns device index or -1 if device not found.

### OpenJoystick
`void* OpenJoystick(int joystickId)`
Returns handle to the device or NULL on error.

### ReadButtons
`uint64_t ReadButtons(void* handle)`
Returns 64-bit value containing:
Raw button states for successful read
Error indication (bit 63 set) for errors
BUTTONS_NO_NEW_DATA (0) when no new events

### CloseJoystick
`int CloseJoystick(void* handle)`
Returns 0 on success, -1 on error.

## Error Handling
Bit 63 (BUTTON_ERROR_BIT) indicates error condition
Error codes:
    BUTTONS_ERROR_INVALID_HANDLE (BUTTON_ERROR_BIT | 1)
    BUTTONS_ERROR_READ_FAILED (BUTTON_ERROR_BIT | 2)
    BUTTONS_ERROR_OVERSIZED_REPORT (BUTTON_ERROR_BIT | 3)
    BUTTONS_NO_NEW_DATA (0) indicates no new events

## Building
1. Open ButtonController.sln in Visual Studio 2022
2. Select configuration (Debug/Release)
3. Build Solution (F7)
4. Find the DLL in Debug/Release directory

## Implementation Notes
Maximum report size is 8 bytes
Devices with larger reports will be truncated
Some devices (like USB FS IO) only report button state changes
Some devices (like USB FS IO) create multiple HID interfaces. In this case, to identify and select a specific interface, UsagePage and Usage fields from the device capabilities should be used (this functionality is not currently implemented).
Report sizes and button mappings vary by device
Default timeout value of 100ms used for event reading
Uses overlapped I/O for non-blocking reads
Supports both event-based and polled devices

## Usage Example
```cpp
// List available devices
char buffer[4096];
GetHIDDeviceList(buffer, sizeof(buffer));

// Find device
int deviceIndex = FindJoystickByProductString("USB FS IO");
// or
int deviceIndex = FindJoystickByVendorAndProductID(0x0FC5, 0xB080);

// Open device
void* handle = OpenJoystick(deviceIndex);

// Read button states
uint64_t state = ReadButtons(handle);
if (!(state & BUTTON_ERROR_BIT)) {
    // Button states are in byte 1 (bits 8-15)
    uint8_t buttons = (state >> 8) & 0xFF;
    bool leftButton = (buttons & 0x10) != 0;
    bool middleButton = (buttons & 0x08) != 0;
    bool rightButton = (buttons & 0x20) != 0;
}

// Close device
CloseJoystick(handle);
```

## Logging
The library includes debug logging capability:
    Log file location: c:\nki\buttons.log
    Logs device operations and raw data
    Timestamps for all events
    Error conditions and codes

## Known Limitations
Windows platform only
Maximum 8 bytes of report data
No force feedback support
No axis or POV hat support implemented

# DirectInput Library

## Features
- Device enumeration with detailed capabilities
- Support for multiple device types
- Device search by vendor/product ID and HID usage
- Device search by product string and HID usage
- Unique instance GUID for reliable device identification
- Button state reading with error handling
- 64-bit button state reporting with error indication

## Device Support
Tested with:
1. USB FS IO (VID: 0xB080, PID: 0x0FC5)
   - Usage Page: 0x0001 (Generic Desktop Controls)
   - Usage: 0x0005 (Game Pad)
   - Instance GUID format: {AAD3B060-C398-11EF-8001-444553540000}
   - Reports as multiple interfaces:
     - Game Pad interface for button controls
     - System Control interface
     - Consumer Control interface
     - Vendor-defined interface

2. Three Button Controller (VID: 0x04D8, PID: 0x005E)
   - Usage Page: 0x0001 (Generic Desktop Controls)
   - Usage: 0x0005 (Game Pad)
   - Instance GUID format: {005E04D8-0000-0000-0000-504944564944}
   - Device Type: Unknown type 0x1C (Standard)
   - Reports as single interface
   - DirectInput recognizes as standard game controller

3. Kinesis JoyStick Controller (VID: 0xB030, PID: 0x0FC5)
   - Usage Page: 0x0001 (Generic Desktop Controls)
   - Usage: 0x0004 (Joystick)
   - Instance GUID format: {B0300FC5-0000-0000-0000-504944564944}
   - Device Type: Joystick (Limited)
   - Reports as single interface
   - Note: Device name includes trailing space in DirectInput enumeration

## Requirements
- Visual Studio 2022
- Windows SDK
- C++17 or later
- Windows platform with HID support


## API Reference

### GetDirectInputDeviceList
`int GetDirectInputDeviceList(char* buffer, int bufferSize)`
Returns JSON-formatted list of available DirectInput devices with their capabilities.
Returns 0 on success, negative values for errors.

### FindJoystickByVendorAndProductIDAndUsage
`const char* FindJoystickByVendorAndProductIDAndUsage(unsigned short vendorID, unsigned short productID, unsigned short usagePage, unsigned short usage)`
Returns device instance GUID string or nullptr if device not found.

### FindJoystickByProductStringAndUsage
`const char* FindJoystickByProductStringAndUsage(const char* name, unsigned short usagePage, unsigned short usage)`
Returns device instance GUID string or nullptr if device not found.

### OpenJoystickByInstanceGUID
`void* OpenJoystickByInstanceGUID(const char* instanceGUID)`
Returns handle to the device or nullptr on error.

### ReadButtons
`uint64_t ReadButtons(void* handle)`
Returns 64-bit value containing:
- Button states for successful read
- Error indication (bit 63 set) for errors
- BUTTONDI_NO_NEW_DATA (0) when no new data

### CloseJoystick
`int CloseJoystick(void* handle)`
Returns 0 on success, -1 on error.

## Error Handling
- Bit 63 (BUTTONDI_ERROR_BIT) indicates error condition
- Error codes:
  - BUTTONDI_ERROR_INVALID_HANDLE (BUTTONDI_ERROR_BIT | 1)
  - BUTTONDI_ERROR_READ_FAILED (BUTTONDI_ERROR_BIT | 2)
  - BUTTONDI_NO_NEW_DATA (0)

## Building
1. Open ButtonController.sln in Visual Studio 2022
2. Select configuration (Debug/Release)
3. Build Solution (F7)
4. Find the DLL in Debug/Release directory

## Implementation Notes
Uses DirectInput8 interface
Supports multiple instances of same device
Device identification through Instance GUIDs
Background and non-exclusive device access
Automatic device reacquisition if lost

## Usage Example
```cpp
// List available devices
char buffer[16384];
GetDirectInputDeviceList(buffer, sizeof(buffer));

// Find device by VID/PID and usage
const char* guid = FindJoystickByVendorAndProductIDAndUsage(
    0xB080,  // VendorID
    0x0FC5,  // ProductID
    0x0001,  // UsagePage (Generic Desktop)
    0x0005   // Usage (Game Pad)
);

// Or find by name and usage
const char* guid = FindJoystickByProductStringAndUsage(
    "USB FS IO",
    0x0001,  // UsagePage
    0x0005   // Usage
);

if (guid) {
    // Open device
    void* handle = OpenJoystickByInstanceGUID(guid);
    if (handle) {
        // Read button states
        uint64_t buttons = ReadButtons(handle);
        if (!(buttons & BUTTONDI_ERROR_BIT)) {
            // Process button states...
        }
        CloseJoystick(handle);
    }
}
```

