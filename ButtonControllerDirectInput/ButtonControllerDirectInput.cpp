#include "pch.h"
#include "ButtonControllerDirectInput.h"
#include <windows.h>
#include <dinput.h>
#include <string>
#include <sstream>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

struct JoystickHandle {
	LPDIRECTINPUTDEVICE8 device;
	DWORD capabilities;
	DWORD deviceType;
};

// Global DirectInput object
static LPDIRECTINPUT8 g_pDI = nullptr;

// Global Buffer for returning GUID string
static char g_GUIDBuffer[40];

// HELPER FUNCTIONS

// Helper function to convert WCHAR* to std::string
std::string wchar_to_string(const WCHAR* wstr) {
	if (wstr == nullptr)
		return std::string();
	int size_needed =
		WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

std::string trim_nulls(const std::string& str) {
	size_t pos = str.find('\0');
	return pos != std::string::npos ? str.substr(0, pos) : str;
}

std::string to_hex_string(unsigned int value) {
	std::stringstream ss;
	ss << "0x" << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << value;
	return ss.str();
}

// More detailed device type information
std::string GetDetailedDeviceType(DWORD dwDevType) {
	std::stringstream ss;
	DWORD primaryType = dwDevType & 0xFF;
	DWORD secondaryType = (dwDevType >> 8) & 0xFF;

	// Primary type
	ss << "Primary: ";
	switch (primaryType) {
	case 0x11: ss << "HID Device"; break;
	case 0x12: ss << "Mouse"; break;
	case 0x13: ss << "Keyboard"; break;
	case 0x14: ss << "Joystick"; break;
	case 0x15: ss << "Gamepad"; break;
	case 0x16: ss << "Driving"; break;
	case 0x17: ss << "Flight"; break;
	default: ss << "Unknown (0x" << std::hex << primaryType << ")";
	}

	// Secondary type
	ss << ", Secondary: ";
	switch (secondaryType) {
	case 0x00: ss << "None"; break;
	case 0x01: ss << "Limited"; break;
	case 0x02: ss << "Standard"; break;
	case 0x03: ss << "Enhanced"; break;
	default: ss << "Unknown (0x" << std::hex << secondaryType << ")";
	}

	return ss.str();
}

// Helper function to convert std::string to std::wstring
std::wstring string_to_wstring(const std::string& str) {
	if (str.empty())
		return std::wstring();
	int size_needed =
		MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0],
		size_needed);
	return wstrTo;
}


// Helper function to convert string GUID to GUID structure
static bool StringToGUID(const char* str, GUID& guid) {
	std::wstring wstr = string_to_wstring(str);
	return SUCCEEDED(CLSIDFromString(wstr.c_str(), &guid));
}

// Callback function for device enumeration
static BOOL CALLBACK EnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef) {
	json* deviceList = static_cast<json*>(pvRef);

	json device;
	device["index"] = deviceList->size();
	device["name"] = trim_nulls(wchar_to_string(lpddi->tszProductName));
	device["instanceName"] = trim_nulls(wchar_to_string(lpddi->tszInstanceName));

	// GUID information
	OLECHAR guidString[40];
	StringFromGUID2(lpddi->guidProduct, guidString, sizeof(guidString) / sizeof(OLECHAR));
	device["productGUID"] = trim_nulls(wchar_to_string(guidString));

	StringFromGUID2(lpddi->guidInstance, guidString, sizeof(guidString) / sizeof(OLECHAR));
	device["instanceGUID"] = trim_nulls(wchar_to_string(guidString));

	// Device type information
	device["dwDevType"] = to_hex_string(lpddi->dwDevType);
	device["dwDevType_primary"] = to_hex_string(lpddi->dwDevType & 0xFF);
	device["dwDevType_secondary"] = to_hex_string((lpddi->dwDevType >> 8) & 0xFF);
	device["typeName"] = GetDetailedDeviceType(lpddi->dwDevType);

	// Usage information
	device["usagePage"] = to_hex_string(lpddi->wUsagePage);
	device["usage"] = to_hex_string(lpddi->wUsage);

	// VID/PID
	device["vendorID"] = to_hex_string((lpddi->guidProduct.Data1 >> 16) & 0xFFFF);
	device["productID"] = to_hex_string(lpddi->guidProduct.Data1 & 0xFFFF);

	deviceList->push_back(device);
	return DIENUM_CONTINUE;
}

extern "C" {

	int GetDirectInputDeviceList(char* buffer, int bufferSize) {
		if (buffer == nullptr || bufferSize <= 0) {
			return -1;  // Invalid parameters
		}

		// Initialize DirectInput if not already done
		if (!g_pDI) {
			HRESULT hr = DirectInput8Create(GetModuleHandle(NULL),
				DIRECTINPUT_VERSION,
				IID_IDirectInput8,
				(void**)&g_pDI,
				NULL);
			if (FAILED(hr)) {
				return -2;  // DirectInput initialization failed
			}
		}

		// Create JSON array for device list
		json deviceList = json::array();

		// Enumerate devices
		//HRESULT hr = g_pDI->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumDevicesCallback, &deviceList, DIEDFL_ATTACHEDONLY);
		HRESULT hr = g_pDI->EnumDevices(DI8DEVCLASS_ALL, EnumDevicesCallback, &deviceList, DIEDFL_ATTACHEDONLY);
		if (FAILED(hr)) {
			return -3;  // Enumeration failed
		}

		// Convert to string
		std::string result = deviceList.dump();
		if (result.length() >= static_cast<size_t>(bufferSize)) {
			return -4;  // Buffer too small
		}

		strncpy_s(buffer, bufferSize, result.c_str(), _TRUNCATE);
		return 0;  // Success
	}

	const char* FindJoystickByVendorAndProductIDAndUsage(unsigned short vendorID, unsigned short productID, unsigned short usagePage, unsigned short usage)
	{
		if (!g_pDI) {
			HRESULT hr = DirectInput8Create(GetModuleHandle(NULL),
				DIRECTINPUT_VERSION,
				IID_IDirectInput8,
				(void**)&g_pDI,
				NULL);
			if (FAILED(hr)) {
				return nullptr;
			}
		}

		struct FindContext {
			unsigned short vendorID;
			unsigned short productID;
			unsigned short usagePage;
			unsigned short usage;
			bool found;
			GUID instanceGUID;
		};

		FindContext context = {
			vendorID,
			productID,
			usagePage,
			usage,
			false
		};

		auto enumCallback = [](LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef) -> BOOL {
			FindContext* ctx = static_cast<FindContext*>(pvRef);

			// Check VID/PID
			unsigned short vid = (lpddi->guidProduct.Data1 >> 16) & 0xFFFF;
			unsigned short pid = lpddi->guidProduct.Data1 & 0xFFFF;

			if (vid == ctx->vendorID &&
				pid == ctx->productID &&
				lpddi->wUsagePage == ctx->usagePage &&
				lpddi->wUsage == ctx->usage)
			{
				ctx->instanceGUID = lpddi->guidInstance;
				ctx->found = true;
				return DIENUM_STOP;
			}
			return DIENUM_CONTINUE;
			};

		HRESULT hr = g_pDI->EnumDevices(DI8DEVCLASS_ALL,
			enumCallback,
			&context,
			DIEDFL_ATTACHEDONLY);

		if (FAILED(hr) || !context.found) {
			return nullptr;
		}

		// Convert GUID to string
		OLECHAR guidString[40];
		StringFromGUID2(context.instanceGUID, guidString, sizeof(guidString) / sizeof(OLECHAR));
		std::string result = wchar_to_string(guidString);
		strncpy_s(g_GUIDBuffer, sizeof(g_GUIDBuffer), result.c_str(), _TRUNCATE);

		return g_GUIDBuffer;
	}

	const char* FindJoystickByProductStringAndUsage(const char* name, unsigned short usagePage, unsigned short usage)
	{
		if (!g_pDI) {
			HRESULT hr = DirectInput8Create(GetModuleHandle(NULL),
				DIRECTINPUT_VERSION,
				IID_IDirectInput8,
				(void**)&g_pDI,
				NULL);
			if (FAILED(hr)) {
				return nullptr;
			}
		}

		struct FindContext {
			const char* name;
			unsigned short usagePage;
			unsigned short usage;
			bool found;
			GUID instanceGUID;
		};

		FindContext context = {
			name,
			usagePage,
			usage,
			false
		};

		auto enumCallback = [](LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef) -> BOOL {
			FindContext* ctx = static_cast<FindContext*>(pvRef);

			std::string productName = trim_nulls(wchar_to_string(lpddi->tszProductName));
			//trim spaces
			productName.erase(0, productName.find_first_not_of(" "));
			productName.erase(productName.find_last_not_of(" ") + 1);

			// Get search name and trim it
			std::string searchName = ctx->name;
			searchName.erase(0, searchName.find_first_not_of(" "));
			searchName.erase(searchName.find_last_not_of(" ") + 1);

			if (productName == searchName &&
				lpddi->wUsagePage == ctx->usagePage &&
				lpddi->wUsage == ctx->usage)
			{
				ctx->instanceGUID = lpddi->guidInstance;
				ctx->found = true;
				return DIENUM_STOP;
			}
			return DIENUM_CONTINUE;
			};

		HRESULT hr = g_pDI->EnumDevices(DI8DEVCLASS_ALL,
			enumCallback,
			&context,
			DIEDFL_ATTACHEDONLY);

		if (FAILED(hr) || !context.found) {
			return nullptr;
		}

		// Convert GUID to string
		OLECHAR guidString[40];
		StringFromGUID2(context.instanceGUID, guidString, sizeof(guidString) / sizeof(OLECHAR));
		std::string result = wchar_to_string(guidString);
		strncpy_s(g_GUIDBuffer, sizeof(g_GUIDBuffer), result.c_str(), _TRUNCATE);

		return g_GUIDBuffer;
	}

	void* OpenJoystickByInstanceGUID(const char* instanceGUID) {
		if (!instanceGUID) return nullptr;

		// Initialize DirectInput if not already done
		if (!g_pDI) {
			HRESULT hr = DirectInput8Create(GetModuleHandle(NULL),
				DIRECTINPUT_VERSION,
				IID_IDirectInput8,
				(void**)&g_pDI,
				NULL);
			if (FAILED(hr)) return nullptr;
		}

		// Convert string GUID to GUID structure
		GUID guid;
		std::wstring wstr = string_to_wstring(instanceGUID);
		if (FAILED(CLSIDFromString(wstr.c_str(), &guid))) {
			return nullptr;
		}

		// Create device
		LPDIRECTINPUTDEVICE8 device = nullptr;
		HRESULT hr = g_pDI->CreateDevice(guid, &device, NULL);
		if (FAILED(hr)) return nullptr;

		// Set data format (we'll use gamepad format as it's most suitable for button controllers)
		hr = device->SetDataFormat(&c_dfDIJoystick2);
		if (FAILED(hr)) {
			device->Release();
			return nullptr;
		}

		// Set cooperative level
		HWND hwnd = GetForegroundWindow();
		hr = device->SetCooperativeLevel(hwnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
		if (FAILED(hr)) {
			device->Release();
			return nullptr;
		}

		// Get device capabilities
		DIDEVCAPS caps;
		caps.dwSize = sizeof(DIDEVCAPS);
		hr = device->GetCapabilities(&caps);
		if (FAILED(hr)) {
			device->Release();
			return nullptr;
		}

		// Create and initialize our handle structure
		JoystickHandle* handle = new JoystickHandle();
		handle->device = device;
		handle->capabilities = caps.dwFlags;
		handle->deviceType = caps.dwDevType;

		// Acquire the device
		device->Acquire();

		return handle;
	}

	uint64_t ReadButtons(void* handle) {
		JoystickHandle* joystickHandle = static_cast<JoystickHandle*>(handle);
		if (!joystickHandle || !joystickHandle->device) {
			return BUTTONDI_ERROR_INVALID_HANDLE;
		}

		DIJOYSTATE2 js;
		HRESULT hr = joystickHandle->device->Poll();

		if (FAILED(hr)) {
			// Device might need to be reacquired
			hr = joystickHandle->device->Acquire();
			if (FAILED(hr)) {
				return BUTTONDI_ERROR_READ_FAILED;
			}
			hr = joystickHandle->device->Poll();
			if (FAILED(hr)) {
				return BUTTONDI_ERROR_READ_FAILED;
			}
		}

		hr = joystickHandle->device->GetDeviceState(sizeof(DIJOYSTATE2), &js);
		if (FAILED(hr)) {
			return BUTTONDI_ERROR_READ_FAILED;
		}

		// Pack the button states into uint64_t
		uint64_t buttonStates = 0;
		for (int i = 0; i < 32 && i < sizeof(js.rgbButtons); i++) {
			if (js.rgbButtons[i] & 0x80) {
				buttonStates |= (1ULL << i);
			}
		}

		return buttonStates;
	}

	int CloseJoystick(void* handle) {
		JoystickHandle* joystickHandle = static_cast<JoystickHandle*>(handle);
		if (!joystickHandle) {
			return -1;
		}

		if (joystickHandle->device) {
			joystickHandle->device->Unacquire();
			joystickHandle->device->Release();
		}

		delete joystickHandle;
		return 0;
	}

}