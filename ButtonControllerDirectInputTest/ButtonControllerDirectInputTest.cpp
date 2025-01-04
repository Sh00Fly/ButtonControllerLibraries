#include "pch.h"
#include <windows.h>
#include "CppUnitTest.h"
#include "../ButtonControllerDirectInput/ButtonControllerDirectInput.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ButtonControllerDirectInputTest
{
	TEST_CLASS(ButtonControllerDirectInputTest)
	{
	public:
		TEST_METHOD(TestGetDeviceList)
		{
			char buffer[65536];
			int result = GetDirectInputDeviceList(buffer, sizeof(buffer));

			// Check if the function succeeded
			Assert::AreEqual(0, result, L"GetDirectInputDeviceList failed");

			// Log the raw buffer content
			Logger::WriteMessage("Raw buffer content:");
			Logger::WriteMessage(buffer);

			try {
				json deviceList = json::parse(buffer);
				Assert::IsTrue(deviceList.is_array());

				Logger::WriteMessage(("Number of devices found: " +
					std::to_string(deviceList.size())).c_str());

				for (const auto& device : deviceList) {
					std::string deviceInfo = "\n\nDevice Details:\n"
						"  Index: " + std::to_string(device["index"].get<int>()) + "\n"
						"  Name: " + device["name"].get<std::string>() + "\n"
						"  Instance Name: " + device["instanceName"].get<std::string>() + "\n"
						"  Product GUID: " + device["productGUID"].get<std::string>() + "\n"
						"  Instance GUID: " + device["instanceGUID"].get<std::string>() + "\n"
						"  Device Type (full): " + device["dwDevType"].get<std::string>() + "\n"
						"  Device Type (primary): " + device["dwDevType_primary"].get<std::string>() + "\n"
						"  Device Type (secondary): " + device["dwDevType_secondary"].get<std::string>() + "\n"
						"  Type Name: " + device["typeName"].get<std::string>() + "\n"
						"  Usage Page: " + device["usagePage"].get<std::string>() + "\n"
						"  Usage: " + device["usage"].get<std::string>() + "\n"
						"  VendorID: " + device["vendorID"].get<std::string>() + "\n"
						"  ProductID: " + device["productID"].get<std::string>();
					Logger::WriteMessage(deviceInfo.c_str());
				}
			}
			catch (const json::exception& e) {
				std::string errorMsg = "JSON parse error: ";
				errorMsg += e.what();
				Logger::WriteMessage(errorMsg.c_str());
				Assert::Fail(L"Failed to parse JSON response");
			}
		}
		TEST_METHOD(TestFindByVendorAndProductIDAndUsage)
		{
			// Find the gamepad instance (0x0001, 0x0005) which is "USB FS IO"
			const char* guid = FindJoystickByVendorAndProductIDAndUsage(0xB080, 0x0FC5, 0x0001, 0x0005);
			Assert::IsNotNull(guid);
			Logger::WriteMessage(("Found device GUID: " + std::string(guid)).c_str());
		}

		TEST_METHOD(TestFindByProductStringAndUsage)
		{
			// Find the gamepad instance
			const char* guid = FindJoystickByProductStringAndUsage("USB FS IO", 0x0001, 0x0005);
			Assert::IsNotNull(guid);
			Logger::WriteMessage(("Found device GUID: " + std::string(guid)).c_str());
		}
		TEST_METHOD(TestOpenReadCloseJoystick)
		{
			const char* guid = FindJoystickByVendorAndProductIDAndUsage(0xB080, 0x0FC5, 0x0001, 0x0005);
			Assert::IsNotNull(guid);

			void* handle = OpenJoystickByInstanceGUID(guid);
			Assert::IsNotNull(handle);

			Logger::WriteMessage("\nPress buttons on your controller (running for 10 seconds)...\n");

			for (int i = 0; i < 100; i++) {  // 10 seconds total
				uint64_t buttons = ReadButtons(handle);
				Assert::IsFalse(IS_BUTTONDI_ERROR(buttons));

				// Create visual representation of button states
				std::string buttonDisplay = "\nButtons: ";
				for (int bit = 0; bit < 32; bit++) {
					buttonDisplay += (buttons & (1ULL << bit)) ? "1" : "0";
					if ((bit + 1) % 8 == 0) buttonDisplay += " ";  // Group by 8 bits
				}
				Logger::WriteMessage(buttonDisplay.c_str());

				Sleep(100);
			}

			int result = CloseJoystick(handle);
			Assert::AreEqual(0, result);
		}
	};
}
