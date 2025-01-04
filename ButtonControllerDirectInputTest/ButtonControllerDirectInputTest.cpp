#include "pch.h"
#include <windows.h>
#include "CppUnitTest.h"
#include "../ButtonControllerDirectInput/ButtonControllerDirectInput.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ButtonControllerDirectInputTest
{
	// Device configurations
	struct TestDeviceConfig {
		const char* name;
		unsigned short vendorID;
		unsigned short productID;
		unsigned short usagePage;
		unsigned short usage;
	};

	// Define configurations for the devices
	static const TestDeviceConfig g_TestDevices[] = {
	{
		"USB FS IO", // name
		0xB080,      // vendorID
		0x0FC5,      // productID
		0x0001,      // usagePage
		0x0005,      // usage
	},
	{
		"Three Button Controller", // name
		0x005E,                    // vendorID
		0x04D8,                    // productID
		0x0001,                    // usagePage
		0x0005,                    // usage
	},
	{
		"Kinesis JoyStick Controller", // name
		0xB030,                        // vendorID
		0x0FC5,                        // productID
		0x0001,                        // usagePage
		0x0004,                        // usage
	}
	};

	TEST_CLASS(CommonFunctionalityTests)
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

	};

	// Base test class with common functionality
	class ButtonControllerTestBase
	{
	public:
		void ValidateDeviceInList(const TestDeviceConfig& config) {
			char buffer[16384];
			int result = GetDirectInputDeviceList(buffer, sizeof(buffer));
			Assert::AreEqual(0, result);

			std::string deviceList(buffer);
			Logger::WriteMessage(buffer);
			Assert::IsTrue(deviceList.find(config.name) != std::string::npos);
		}

		void* OpenConfiguredDevice(const TestDeviceConfig& config) {
			const char* guid = FindJoystickByVendorAndProductIDAndUsage(
				config.vendorID,
				config.productID,
				config.usagePage,
				config.usage
			);
			Assert::IsNotNull(guid);

			void* handle = OpenJoystickByInstanceGUID(guid);
			Assert::IsNotNull(handle);
			return handle;
		}

		void TestFindByVendorAndProductIDAndUsage(const TestDeviceConfig& config)
		{
			const char* guid = FindJoystickByVendorAndProductIDAndUsage(
				config.vendorID,
				config.productID,
				config.usagePage,
				config.usage);
			Assert::IsNotNull(guid);
			Logger::WriteMessage(("\nFound device GUID: " + std::string(guid)).c_str());
		}

		void TestFindByProductStringAndUsage(const TestDeviceConfig& config)
		{
			const char* guid = FindJoystickByProductStringAndUsage(
				config.name,
				config.usagePage,
				config.usage);
			Assert::IsNotNull(guid);
			Logger::WriteMessage(("\nFound device GUID: " + std::string(guid)).c_str());
		}

		void TestOpenReadCloseJoystick(const TestDeviceConfig& config, void*& handle)
		{
			const char* guid = FindJoystickByVendorAndProductIDAndUsage(
				config.vendorID,
				config.productID,
				config.usagePage,
				config.usage);
			Assert::IsNotNull(guid);

			handle = OpenJoystickByInstanceGUID(guid);
			Assert::IsNotNull(handle);

			Logger::WriteMessage("\nPress buttons on your controller (running for 10 seconds)...\n");

			for (int i = 0; i < 100; i++) {
				uint64_t buttons = ReadButtons(handle);
				Assert::IsFalse(IS_BUTTONDI_ERROR(buttons));

				std::string buttonDisplay = "\nButtons: ";
				for (int bit = 0; bit < 32; bit++) {
					buttonDisplay += (buttons & (1ULL << bit)) ? "1" : "0";
					if ((bit + 1) % 8 == 0) buttonDisplay += " ";
				}
				Logger::WriteMessage(buttonDisplay.c_str());

				Sleep(100);
			}
		}
	};

	// First device test class
	TEST_CLASS(USBFSIOTests)
	{
	private:
		ButtonControllerTestBase base;
		const TestDeviceConfig& GetConfig() { return g_TestDevices[0]; }
		void* m_handle;

	public:
		TEST_METHOD_INITIALIZE(MethodInitialize)
		{
			Logger::WriteMessage("Method Initialize: Setting up USB FS IO device");
			m_handle = nullptr;
		}

		TEST_METHOD_CLEANUP(MethodCleanup)
		{
			if (m_handle) {
				CloseJoystick(m_handle);
				m_handle = nullptr;
			}
		}

		TEST_METHOD(TestFindByVendorAndProductIDAndUsage)
		{
			base.TestFindByVendorAndProductIDAndUsage(GetConfig());
		}

		TEST_METHOD(TestFindByProductStringAndUsage)
		{
			base.TestFindByProductStringAndUsage(GetConfig());
		}

		TEST_METHOD(TestOpenReadCloseJoystick)
		{
			base.TestOpenReadCloseJoystick(GetConfig(), m_handle);
		}
	};
	TEST_CLASS(ThreeButtonControllerTests)
	{
	private:
		ButtonControllerTestBase base;
		const TestDeviceConfig& GetConfig() { return g_TestDevices[1]; }
		void* m_handle;

	public:
		TEST_METHOD_INITIALIZE(MethodInitialize)
		{
			Logger::WriteMessage("Method Initialize: Setting up Three Button Controller device");
			m_handle = nullptr;
		}

		TEST_METHOD_CLEANUP(MethodCleanup)
		{
			if (m_handle) {
				CloseJoystick(m_handle);
				m_handle = nullptr;
			}
		}

		TEST_METHOD(TestFindByVendorAndProductIDAndUsage)
		{
			base.TestFindByVendorAndProductIDAndUsage(GetConfig());
		}

		TEST_METHOD(TestFindByProductStringAndUsage)
		{
			base.TestFindByProductStringAndUsage(GetConfig());
		}

		TEST_METHOD(TestOpenReadCloseJoystick)
		{
			base.TestOpenReadCloseJoystick(GetConfig(), m_handle);
		}
	};

	TEST_CLASS(KinessisJoystickTests)
	{
	private:
		ButtonControllerTestBase base;
		const TestDeviceConfig& GetConfig() { return g_TestDevices[2]; }
		void* m_handle;

	public:
		TEST_METHOD_INITIALIZE(MethodInitialize)
		{
			Logger::WriteMessage("Method Initialize: Setting up Kinessis Joystick Controller device");
			m_handle = nullptr;
		}

		TEST_METHOD_CLEANUP(MethodCleanup)
		{
			if (m_handle) {
				CloseJoystick(m_handle);
				m_handle = nullptr;
			}
		}

		TEST_METHOD(TestFindByVendorAndProductIDAndUsage)
		{
			base.TestFindByVendorAndProductIDAndUsage(GetConfig());
		}

		TEST_METHOD(TestFindByProductStringAndUsage)
		{
			base.TestFindByProductStringAndUsage(GetConfig());
		}

		TEST_METHOD(TestOpenReadCloseJoystick)
		{
			base.TestOpenReadCloseJoystick(GetConfig(), m_handle);
		}
	};
}
