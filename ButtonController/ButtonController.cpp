#include "pch.h"
#include "ButtonController.h"
#include <windows.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <setupapi.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <locale>
#include <devguid.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

//------------------------------ debug start ------------------------------
#include <fstream>
#include <chrono>
#include <ctime>
//------------------------------- debug end -------------------------------

#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")

//	HANDLE g_deviceHandle = INVALID_HANDLE_VALUE;
struct JoystickHandle {
	HANDLE deviceHandle;
	// any other device-specific fields can be added here
};

// Helper function to convert WCHAR* to std::string
std::string wchar_to_string(const WCHAR* wstr) {
	if (wstr == nullptr) return std::string();
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

// Helper function to convert std::string to std::wstring
std::wstring string_to_wstring(const std::string& str) {
	if (str.empty()) return std::wstring();
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

//------------------------------ debug start ------------------------------
static std::ofstream logFile;

void InitializeLog(const char* logFilePath) {
	logFile.open(logFilePath, std::ios::out | std::ios::app);
}

void CloseLog() {
	if (logFile.is_open()) {
		logFile.close();
	}
}

// C++ implementation
void WriteToLogCpp(const std::string& message) {
    if (logFile.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm bt;
        localtime_s(&bt, &in_time_t);
        logFile << std::put_time(&bt, "%Y-%m-%d %X") << ": " << message << std::endl;
    }
}

// C interface implementation
extern "C" void WriteToLog(const char* message) {
    if (message) {
        WriteToLogCpp(std::string(message));
    }
}

void LogDeviceCapabilities(HANDLE deviceHandle) {
	PHIDP_PREPARSED_DATA preparsedData = NULL;
	if (!HidD_GetPreparsedData(deviceHandle, &preparsedData)) {
		WriteToLog("Failed to get preparsed data");
		return;
	}

	HIDP_CAPS caps;
	if (HidP_GetCaps(preparsedData, &caps) == HIDP_STATUS_SUCCESS) {
		std::stringstream ss;
		ss << "Device Capabilities:" << std::endl
			<< "Input Report Length: " << caps.InputReportByteLength << std::endl
			<< "Output Report Length: " << caps.OutputReportByteLength << std::endl
			<< "Feature Report Length: " << caps.FeatureReportByteLength << std::endl
			<< "Number of Input Button Caps: " << caps.NumberInputButtonCaps << std::endl
			<< "Number of Input Value Caps: " << caps.NumberInputValueCaps;
		WriteToLog(ss.str().c_str());
	}

	HidD_FreePreparsedData(preparsedData);
}

// Helper function to convert unsigned int to hex string
std::string UIntToHexString(unsigned int value) {
	std::stringstream stream;
	stream << "0x" << std::hex << std::setfill('0') << std::setw(8) << value;
	return stream.str();
}
//------------------------------ debug end ------------------------------

// Helper function to trim whitespace from both ends of a string
std::wstring trim(const std::wstring& str) {
	auto start = str.find_first_not_of(L" \t\r\n");
	if (start == std::wstring::npos) return L"";
	auto end = str.find_last_not_of(L" \t\r\n");
	return str.substr(start, end - start + 1);
}

extern "C" {

	//************************** GetHIDDeviceList *****************************
	int GetHIDDeviceList(char* buffer, int bufferSize) {
		if (buffer == nullptr || bufferSize <= 0) {
			return -1;  // Invalid parameters
		}

		GUID hidGuid;
		HidD_GetHidGuid(&hidGuid);

		HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&hidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if (deviceInfoSet == INVALID_HANDLE_VALUE) {
			return -2;  // Failed to get device info set
		}

		json deviceList = json::array();
		SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
		deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

		for (DWORD deviceIndex = 0; SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &hidGuid, deviceIndex, &deviceInterfaceData); ++deviceIndex) {
			DWORD requiredSize = 0;
			SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, NULL, 0, &requiredSize, NULL);

			std::vector<BYTE> detailDataBuffer(requiredSize);
			PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(&detailDataBuffer[0]);
			detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

			json device;
			device["index"] = deviceIndex;
			device["path"] = "";
			device["vendorID"] = "";
			device["productID"] = "";
			device["versionNumber"] = "";
			device["product"] = "";
			device["manufacturer"] = "";
			device["serialNumber"] = "";
			device["usage"] = "";
			device["usagePage"] = "";
			device["inputReportByteLength"] = 0;
			device["outputReportByteLength"] = 0;
			device["featureReportByteLength"] = 0;
			device["numberOfLinkCollectionNodes"] = 0;
			device["numberOfInputButtonCaps"] = 0;
			device["numberOfInputValueCaps"] = 0;
			device["numberOfInputDataIndices"] = 0;
			device["numberOfOutputButtonCaps"] = 0;
			device["numberOfOutputValueCaps"] = 0;
			device["numberOfOutputDataIndices"] = 0;
			device["numberOfFeatureButtonCaps"] = 0;
			device["numberOfFeatureValueCaps"] = 0;
			device["numberOfFeatureDataIndices"] = 0;
			device["axesTotal"] = 0;
			device["buttonsTotal"] = 0;
			device["povTotal"] = 0;

			if (SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, detailData, requiredSize, NULL, NULL)) {
				HANDLE deviceHandle = CreateFile(detailData->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
				if (deviceHandle != INVALID_HANDLE_VALUE) {
					device["path"] = wchar_to_string(detailData->DevicePath);

					HIDD_ATTRIBUTES attributes;
					if (HidD_GetAttributes(deviceHandle, &attributes)) {
						std::stringstream ss;
						ss << "0x" << std::hex << std::setw(4) << std::setfill('0') << attributes.VendorID;
						device["vendorID"] = ss.str();
						ss.str("");
						ss << "0x" << std::hex << std::setw(4) << std::setfill('0') << attributes.ProductID;
						device["productID"] = ss.str();
						ss.str("");
						ss << "0x" << std::hex << std::setw(4) << std::setfill('0') << attributes.VersionNumber;
						device["versionNumber"] = ss.str();
					}

					wchar_t productString[256] = L"";
					if (HidD_GetProductString(deviceHandle, productString, sizeof(productString))) {
						device["product"] = wchar_to_string(productString);
					}

					wchar_t manufacturerString[256] = L"";
					if (HidD_GetManufacturerString(deviceHandle, manufacturerString, sizeof(manufacturerString))) {
						device["manufacturer"] = wchar_to_string(manufacturerString);
					}

					wchar_t serialNumberString[256] = L"";
					if (HidD_GetSerialNumberString(deviceHandle, serialNumberString, sizeof(serialNumberString))) {
						device["serialNumber"] = wchar_to_string(serialNumberString);
					}

					PHIDP_PREPARSED_DATA preparsedData = NULL;
					if (HidD_GetPreparsedData(deviceHandle, &preparsedData)) {
						HIDP_CAPS capabilities;
						if (HidP_GetCaps(preparsedData, &capabilities) == HIDP_STATUS_SUCCESS) {
							std::stringstream ss;
							ss << "0x" << std::hex << std::setw(4) << std::setfill('0') << capabilities.Usage;
							device["usage"] = ss.str();
							ss.str("");
							ss << "0x" << std::hex << std::setw(4) << std::setfill('0') << capabilities.UsagePage;
							device["usagePage"] = ss.str();
							device["inputReportByteLength"] = capabilities.InputReportByteLength;
							device["outputReportByteLength"] = capabilities.OutputReportByteLength;
							device["featureReportByteLength"] = capabilities.FeatureReportByteLength;
							device["numberOfLinkCollectionNodes"] = capabilities.NumberLinkCollectionNodes;
							device["numberOfInputButtonCaps"] = capabilities.NumberInputButtonCaps;
							device["numberOfInputValueCaps"] = capabilities.NumberInputValueCaps;
							device["numberOfInputDataIndices"] = capabilities.NumberInputDataIndices;
							device["numberOfOutputButtonCaps"] = capabilities.NumberOutputButtonCaps;
							device["numberOfOutputValueCaps"] = capabilities.NumberOutputValueCaps;
							device["numberOfOutputDataIndices"] = capabilities.NumberOutputDataIndices;
							device["numberOfFeatureButtonCaps"] = capabilities.NumberFeatureButtonCaps;
							device["numberOfFeatureValueCaps"] = capabilities.NumberFeatureValueCaps;
							device["numberOfFeatureDataIndices"] = capabilities.NumberFeatureDataIndices;

							// Get value caps
							USHORT valueCapLength = capabilities.NumberInputValueCaps;
							std::vector<HIDP_VALUE_CAPS> valueCaps(valueCapLength);

							if (HidP_GetValueCaps(HidP_Input, valueCaps.data(), &valueCapLength, preparsedData) == HIDP_STATUS_SUCCESS) {
								int axesTotal = 0;
								int povTotal = 0;

								for (const auto& cap : valueCaps) {
									if (cap.UsagePage == 0x01) { // Generic Desktop Controls
										switch (cap.NotRange.Usage) {
										case 0x30: // X
										case 0x31: // Y
										case 0x32: // Z
										case 0x33: // Rx
										case 0x34: // Ry
										case 0x35: // Rz
										case 0x36: // Slider
										case 0x37: // Dial
										case 0x38: // Wheel
											axesTotal++;
											break;
										case 0x39: // Hat switch
											povTotal++;
											break;
										}
									}
								}

								device["axesTotal"] = axesTotal;
								device["povTotal"] = povTotal;
							}

							// Get button caps for buttonsTotal
							USHORT buttonCapLength = capabilities.NumberInputButtonCaps;
							std::vector<HIDP_BUTTON_CAPS> buttonCaps(buttonCapLength);

							if (HidP_GetButtonCaps(HidP_Input, buttonCaps.data(), &buttonCapLength, preparsedData) == HIDP_STATUS_SUCCESS) {
								int buttonsTotal = 0;

								for (const auto& cap : buttonCaps) {
									if (cap.IsRange) {
										buttonsTotal += cap.Range.UsageMax - cap.Range.UsageMin + 1;
									}
									else {
										buttonsTotal++;
									}
								}

								device["buttonsTotal"] = buttonsTotal;
							}
						}
						HidD_FreePreparsedData(preparsedData);
					}
					CloseHandle(deviceHandle);
				}
			}

			deviceList.push_back(device);
		}

		SetupDiDestroyDeviceInfoList(deviceInfoSet);

		std::string result = deviceList.dump(-1);  // -1 for no indentation

		if (result.length() >= static_cast<size_t>(bufferSize)) {
			return -3;  // Buffer too small
		}

		strncpy_s(buffer, bufferSize, result.c_str(), _TRUNCATE);
		return 0;  // Success
	}
	//******************** FindJoystickByVendorAndProductID ********************
	int FindJoystickByVendorAndProductID(unsigned short vendorID, unsigned short productID) {
		GUID hidGuid;
		HidD_GetHidGuid(&hidGuid);

		HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&hidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if (deviceInfoSet == INVALID_HANDLE_VALUE) {
			return -1;
		}

		SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
		deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

		int deviceIndex = 0;
		while (SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &hidGuid, deviceIndex, &deviceInterfaceData)) {
			DWORD requiredSize = 0;
			SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, NULL, 0, &requiredSize, NULL);

			PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
			if (detailData == NULL) {
				SetupDiDestroyDeviceInfoList(deviceInfoSet);
				return -1;  // Error: memory allocation failed
			}
			detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

			if (SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, detailData, requiredSize, NULL, NULL)) {
				HANDLE deviceHandle = CreateFile(detailData->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
				if (deviceHandle != INVALID_HANDLE_VALUE) {
					HIDD_ATTRIBUTES attributes;
					if (HidD_GetAttributes(deviceHandle, &attributes)) {
						if (attributes.VendorID == vendorID && attributes.ProductID == productID) {
							CloseHandle(deviceHandle);
							free(detailData);
							SetupDiDestroyDeviceInfoList(deviceInfoSet);
							return deviceIndex;
						}
					}
					CloseHandle(deviceHandle);
				}
			}

			free(detailData);
			deviceIndex++;
		}

		SetupDiDestroyDeviceInfoList(deviceInfoSet);
		return -1;  // Device not found
	}

	//******************** FindJoystickByProductString ********************
	int FindJoystickByProductString(const char* name) {
		GUID hidGuid;
		HidD_GetHidGuid(&hidGuid);

		HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&hidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if (deviceInfoSet == INVALID_HANDLE_VALUE) {
			return -1;
		}

		SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
		deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

		std::wstring wName = trim(string_to_wstring(name));

		int deviceIndex = 0;
		while (SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &hidGuid, deviceIndex, &deviceInterfaceData)) {
			DWORD requiredSize = 0;
			SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, NULL, 0, &requiredSize, NULL);

			PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
			if (detailData == NULL) {
				SetupDiDestroyDeviceInfoList(deviceInfoSet);
				return -1;  // Error: memory allocation failed
			}
			detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

			if (SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, detailData, requiredSize, NULL, NULL)) {
				HANDLE deviceHandle = CreateFile(detailData->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
				if (deviceHandle != INVALID_HANDLE_VALUE) {
					wchar_t productString[256] = L"";
					if (HidD_GetProductString(deviceHandle, productString, sizeof(productString))) {
						if (trim(std::wstring(productString)) == wName) {
							CloseHandle(deviceHandle);
							free(detailData);
							SetupDiDestroyDeviceInfoList(deviceInfoSet);
							return deviceIndex;
						}
					}
					CloseHandle(deviceHandle);
				}
			}

			free(detailData);
			deviceIndex++;
		}

		SetupDiDestroyDeviceInfoList(deviceInfoSet);
		return -1;  // Device not found
	}
	//******************** OpenJoystick ********************
	void* OpenJoystick(int joystickId) {
		//------------------------------ debug start ------------------------------
		InitializeLog("c:\\nki\\buttons.log");
		//------------------------------- debug end -------------------------------
		GUID hidGuid;
		HidD_GetHidGuid(&hidGuid);

		HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&hidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if (deviceInfoSet == INVALID_HANDLE_VALUE) {
			return NULL;
		}

		SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
		deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		if (!SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &hidGuid, joystickId, &deviceInterfaceData)) {
			SetupDiDestroyDeviceInfoList(deviceInfoSet);
			return NULL;  // Error: couldn't find the specified joystick
		}
		DWORD requiredSize = 0;
		SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, NULL, 0, &requiredSize, NULL);

		PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
		if (!detailData) {
			SetupDiDestroyDeviceInfoList(deviceInfoSet);
			return NULL;  // Error: memory allocation failed
		}
		detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		if (!SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, detailData, requiredSize, NULL, NULL)) {
			free(detailData);
			SetupDiDestroyDeviceInfoList(deviceInfoSet);
			return NULL;  // Error: couldn't get device interface detail
		}

		HANDLE deviceHandle = CreateFile(
			detailData->DevicePath,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED,
			NULL
		);

		free(detailData);

		SetupDiDestroyDeviceInfoList(deviceInfoSet);

		if (deviceHandle == INVALID_HANDLE_VALUE) {
			//------------------------------ debug start ------------------------------
			WriteToLog("Failed to open the device.");
			//------------------------------- debug end -------------------------------
			return NULL;  // Error: couldn't open the device
		}

		JoystickHandle* handle = new (std::nothrow) JoystickHandle();
		if (!handle) {
			CloseHandle(deviceHandle);
			//------------------------------ debug start ------------------------------
			WriteToLog("Memory allocation failed for JoystickHandle.");
			//------------------------------- debug end -------------------------------
			return NULL;  // Error: couldn't allocate memory for JoystickHandle
		}

		handle->deviceHandle = deviceHandle;
		//------------------------------ debug start ------------------------------
		// Log the handle value
		std::stringstream ss;
		ss << "Opened joystick with handle: " << handle->deviceHandle;
		WriteToLog(ss.str().c_str());
		LogDeviceCapabilities(deviceHandle);
		//------------------------------- debug end -------------------------------

		return handle;
	}

	//******************** ReadButtons ********************
	unsigned int ReadButtons(void* handle) {
		JoystickHandle* joystickHandle = static_cast<JoystickHandle*>(handle);
		if (!joystickHandle || joystickHandle->deviceHandle == INVALID_HANDLE_VALUE) {
			//------------------------------ debug start ------------------------------
			WriteToLog("Invalid handle in ReadButtons");
			//------------------------------- debug end -------------------------------
			return static_cast<unsigned int>(-1);
		}

		// Get the capabilities first to determine the correct input report length
		PHIDP_PREPARSED_DATA preparsedData = NULL;
		if (!HidD_GetPreparsedData(joystickHandle->deviceHandle, &preparsedData)) {
			//------------------------------ debug start ------------------------------
			WriteToLog("Failed to get preparsed data");
			//------------------------------- debug end -------------------------------
			return static_cast<unsigned int>(-2);
		}

		HIDP_CAPS caps;
		if (HidP_GetCaps(preparsedData, &caps) != HIDP_STATUS_SUCCESS) {
			HidD_FreePreparsedData(preparsedData);
			//------------------------------ debug start ------------------------------
			WriteToLog("Failed to get capabilities");
			//------------------------------- debug end -------------------------------
			return static_cast<unsigned int>(-3);
		}

		// Use the actual input report length from the device
		DWORD bufferSize = caps.InputReportByteLength;
		std::vector<BYTE> buffer(bufferSize);

		//------------------------------ debug start ------------------------------
		WriteToLog(("Buffer size: " + std::to_string(bufferSize)).c_str());
		//------------------------------- debug end -------------------------------

		OVERLAPPED ol = { 0 };
		ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		if (ol.hEvent == NULL) {
			HidD_FreePreparsedData(preparsedData);
			//------------------------------ debug start ------------------------------
			WriteToLog("Failed to create event for overlapped I/O");
			//------------------------------- debug end -------------------------------
			return static_cast<unsigned int>(-4);
		}

		DWORD bytesRead = 0;
		//------------------------------ debug start ------------------------------
		WriteToLog("Attempting to read from device...");
		//------------------------------- debug end -------------------------------

		if (!ReadFile(joystickHandle->deviceHandle, buffer.data(), bufferSize, &bytesRead, &ol)) {
			DWORD error = GetLastError();
			if (error != ERROR_IO_PENDING) {
				CloseHandle(ol.hEvent);
				HidD_FreePreparsedData(preparsedData);
				//------------------------------ debug start ------------------------------
				WriteToLog(("ReadFile failed immediately with error: " + std::to_string(error)).c_str());
				//------------------------------- debug end -------------------------------
				return static_cast<unsigned int>(-5);
			}
		}

		// Wait for the read operation to complete
		DWORD waitResult = WaitForSingleObject(ol.hEvent, 1000); // 1 second timeout
		if (waitResult != WAIT_OBJECT_0) {
			CloseHandle(ol.hEvent);
			HidD_FreePreparsedData(preparsedData);
			//------------------------------ debug start ------------------------------
			WriteToLog(("Wait failed or timed out. WaitResult: " + std::to_string(waitResult)).c_str());
			//------------------------------- debug end -------------------------------
			CancelIo(joystickHandle->deviceHandle);
			return static_cast<unsigned int>(-6);
		}

		if (!GetOverlappedResult(joystickHandle->deviceHandle, &ol, &bytesRead, FALSE)) {
			DWORD error = GetLastError();
			CloseHandle(ol.hEvent);
			HidD_FreePreparsedData(preparsedData);
			//------------------------------ debug start ------------------------------
			WriteToLog(("GetOverlappedResult failed with error: " + std::to_string(error)).c_str());
			//------------------------------- debug end -------------------------------
			return static_cast<unsigned int>(-7);
		}

		CloseHandle(ol.hEvent);
		HidD_FreePreparsedData(preparsedData);

		//------------------------------ debug start ------------------------------
		// Log raw data
		std::stringstream ss;
		ss << "Raw data: ";
		for (DWORD i = 0; i < bytesRead; i++) {
			ss << std::hex << std::setfill('0') << std::setw(2)
				<< static_cast<int>(buffer[i]) << " ";
		}
		WriteToLog(ss.str().c_str());
		//------------------------------- debug end -------------------------------

		// Process the data
		unsigned int result = 0;
		for (DWORD i = 0; i < min(bytesRead, 4); i++) {
			result |= (static_cast<unsigned int>(buffer[i]) << (i * 8));
		}

		std::string logMessage = "Returning button state: " + UIntToHexString(result);
		//------------------------------ debug start ------------------------------
		WriteToLog(logMessage.c_str());
		//------------------------------- debug end -------------------------------

		return result;
	}

	//******************** CloseJoystick ********************
	int CloseJoystick(void* handle) {
		JoystickHandle* joystickHandle = static_cast<JoystickHandle*>(handle);
		if (joystickHandle && joystickHandle->deviceHandle != INVALID_HANDLE_VALUE) {
			//------------------------------ debug start ------------------------------
			//WriteToLog("Closing joystick handle.");
			//------------------------------- debug end -------------------------------
			CloseHandle(joystickHandle->deviceHandle);
			delete joystickHandle;
			//------------------------------ debug start ------------------------------
			CloseLog();
			//------------------------------- debug end -------------------------------
			return 0;
		}
		//------------------------------ debug start ------------------------------
		WriteToLog("Attempted to close an invalid or already closed handle.");
		CloseLog();
		//------------------------------- debug end -------------------------------
		return -1;  // No device was open
	}
}

