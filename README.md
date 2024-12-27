# HID Button Controller Library

A Windows library for communicating with HID devices, particularly button controllers that identify as joysticks.

## Features
- Device enumeration
- Button state reading
- Support for multiple devices
- Vendor/Product ID filtering
- Product string matching

## Requirements
- Visual Studio 2022
- Windows SDK
- C++17 or later

## Building
1. Open ButtonController.sln in Visual Studio 2022
2. Select configuration (Debug/Release)
3. Build Solution (F7)

## Button State Format
The ReadButtons function returns a 64-bit value containing the raw HID report data.
Each byte of the report is packed into the result with byte 0 in the least significant
byte position.

### Limitations
- Maximum report size is 8 bytes
- Devices with larger reports will be truncated
- Error codes are returned as special 64-bit values (see error codes section)

### Example Button Mappings
For USB FS IO (7-byte report):
- Left Button: bit 12 (byte[1] bit 4)
- Middle Button: bit 11 (byte[1] bit 3)
- Right Button: bit 13 (byte[1] bit 5)

For Kinesis Controller (4-byte report):
- Button 1: bit 24 (byte[3] bit 0)
- Button 2: bit 25 (byte[3] bit 1)
- Button 3: bit 26 (byte[3] bit 2)

## Usage Example
```cpp
// Basic usage example here
