#include <windows.h>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdint.h>
#include "ButtonControllerRaw.h"

using json = nlohmann::json;

#pragma comment(lib, "ButtonControllerRaw.lib")

void printButtonStates(uint64_t buttonStates) {
  std::cout << "Binary: ";
  for (int i = 63; i >= 0; i--) { // Changed from 31 to 63 for 64 bits
    std::cout << ((buttonStates & (1ULL << i)) ? "1" : "0"); // Note the ULL
    if (i % 8 == 0)
      std::cout << " ";
  }
  std::cout << "\n";
  // std::cout << "Hex: 0x" << std::hex << std::setw(16) << std::setfill('0') <<
  // buttonStates << std::dec << std::endl;
}

void printDeviceInfo(const json &device) {
  std::cout << "\nDevice #" << device["index"].get<int>() << ":\n";
  std::cout << "  Manufacturer: " << device["manufacturer"].get<std::string>()
            << "\n";
  std::cout << "  Product: " << device["product"].get<std::string>() << "\n";
  std::cout << "  Vendor ID: " << device["vendorID"].get<std::string>() << "\n";
  std::cout << "  Product ID: " << device["productID"].get<std::string>()
            << "\n";
  std::cout << "  Version: " << device["versionNumber"].get<std::string>()
            << "\n";
  std::cout << "  Serial Number: " << device["serialNumber"].get<std::string>()
            << "\n";
  std::cout << "  Usage Page: " << device["usagePage"].get<std::string>()
            << "\n";
  std::cout << "  Usage: " << device["usage"].get<std::string>() << "\n";
  std::cout << "  Buttons: " << device["buttonsTotal"].get<int>() << "\n";
  std::cout << "  Axes: " << device["axesTotal"].get<int>() << "\n";
  std::cout << "  POV Hats: " << device["povTotal"].get<int>() << "\n";
  std::cout << "  Input Report Byte Length: "
            << device["inputReportByteLength"].get<int>() << "\n";
  std::cout << "  Output Report Byte Length: "
            << device["outputReportByteLength"].get<int>() << "\n";
  std::cout << "  Feature Report Byte Length: "
            << device["featureReportByteLength"].get<int>() << "\n";
}

const char *getErrorMessage(int error) {
  switch (error) {
  case -1:
    return "Invalid parameters";
  case -2:
    return "Failed to get device info set";
  case -3:
    return "Buffer too small";
  default:
    return "Unknown error";
  }
}

int main() {
  std::cout << "Button Controller Test\n";
  std::cout << "=====================\n\n";

  // Try with progressively larger buffer sizes
  const int BUFFER_SIZES[] = {16384, 32768, 65536, 131072};
  char *buffer = nullptr;
  int result = -3; // Initialize to buffer too small

  for (int bufferSize : BUFFER_SIZES) {
    buffer = new char[bufferSize];
    if (!buffer) {
      std::cout << "Failed to allocate buffer of size " << bufferSize
                << std::endl;
      continue;
    }

    std::cout << "Trying with buffer size: " << bufferSize << " bytes..."
              << std::endl;
    result = GetHIDDeviceList(buffer, bufferSize);

    if (result == 0) {
      std::cout << "Success with buffer size: " << bufferSize << " bytes\n";
      break;
    } else {
      std::cout << "Failed with error: " << getErrorMessage(result)
                << std::endl;
      delete[] buffer;
      buffer = nullptr;
    }
  }

  if (result == 0 && buffer != nullptr) {
    try {
      json devices = json::parse(buffer);
      std::cout << "Found " << devices.size() << " HID devices:\n";
      std::cout << "Raw JSON (first 200 chars):\n";
      std::cout << std::string(buffer).substr(0, 200)
                << "...\n"; // Print first 200 chars of JSON

      for (const auto &device : devices) {
        printDeviceInfo(device);
      }
    } catch (const json::parse_error &e) {
      std::cout << "Failed to parse device list: " << e.what() << std::endl;
      delete[] buffer;
      return 1;
    }
    delete[] buffer;
  } else {
    std::cout << "Failed to list HID devices with all buffer sizes.\n";
    std::cout << "Last error: " << getErrorMessage(result) << std::endl;
    return 1;
  }

  // Find joystick by product string
  // const char* deviceName = "Three Button Controller";
  const char *deviceName = "USB FS IO";
  // const char* deviceName = "Kinesis JoyStick Controller";
  int joystickId = FindJoystickByProductString(deviceName);

  if (joystickId >= 0) {
    std::cout << "Found joystick with ID: " << joystickId << std::endl;
  } else {
    std::cout << "Joystick not found by product string." << std::endl;
  }

  // Find joystick by vendor and product ID
  // unsigned short vendorID = 0x04d8;  // Three Button Controller
  // unsigned short productID = 0x005e; // Three Button Controller
  // unsigned short vendorID = 0x0FC5;  // Pedals
  // unsigned short productID = 0xB030; // Pedals
  unsigned short vendorID = 0x0FC5;  // USB FS IO
  unsigned short productID = 0xB080; // USB FS IO

  joystickId = FindJoystickByVendorAndProductID(vendorID, productID);

  if (joystickId >= 0) {
    std::cout << "Found joystick with ID: " << joystickId << std::endl;
  } else {
    std::cout << "Joystick not found by vendor and product ID." << std::endl;
  }

  // Open and read from joystick
  void *joystickHandle = OpenJoystick(joystickId);
  if (joystickHandle) {
    std::cout << "Successfully opened joystick." << std::endl;

    for (int i = 0; i < 500; i++) {
      uint64_t buttonStates = ReadButtons(joystickHandle);
      if (buttonStates != BUTTONRAW_ERROR_INVALID_HANDLE &&
          buttonStates != BUTTONRAW_ERROR_READ_FAILED &&
          buttonStates != BUTTONRAW_ERROR_OVERSIZED_REPORT) {
        std::cout << "Button states: ";
        printButtonStates(buttonStates);
      } else {
        std::cout << "Failed to read button states. Error code: 0x" << std::hex
                  << std::setw(16) << std::setfill('0') << buttonStates
                  << std::dec << std::endl;
      }
      Sleep(10);
    }

    if (CloseJoystick(joystickHandle) == 0) {
      std::cout << "Successfully closed joystick." << std::endl;
    } else {
      std::cout << "Failed to close joystick." << std::endl;
    }
  } else {
    std::cout << "Failed to open joystick." << std::endl;
  }
  return 0;
}
