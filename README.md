# HID Button Controller Library

A Windows library for communicating with HID devices, particularly button controllers that identify as joysticks in Windows.

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
