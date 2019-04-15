/*
 * JSmartconfigService.h
 *
 *  Created on: 2018年1月24日
 *      Author: JimmyTai
 */

#ifndef JSMARTCONFIGSERVICE_H_
#define JSMARTCONFIGSERVICE_H_

#include "Arduino.h"
#include "GTech.h"
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLE2902.h"
#include "WiFi.h"
#include "WiFiConfig.h"

namespace JSmartconfigUUID {
	const char SERVICE[] = "342300F0-4A54-0315-0082-472D54656368";
	const char CHAR_SMARTCONFIG_STATUS[] = "342300F1-4A54-0315-0082-472D54656368";
	const char CHAR_WIFI_SSID[] = "342300F2-4A54-0315-0082-472D54656368";
	const char CHAR_SMARTCONFIG[] = "342300F3-4A54-0315-0082-472D54656368";
}

class JSmartconfigServiceCallback: public BLECharacteristicCallbacks {

	void onRead(BLECharacteristic *characteristic);
	void onWrite(BLECharacteristic *characteristic);

public:

	String _attempt_ssid, _attempt_password;
	bool _isAttemptConfigData = false;
	std::string _attempt_config_data;

private:

};

class JSmartconfigService {
public:
	enum Status {
		IDLE, WIFI_CONNECTING, DATA_CONFIGURING
	};

	typedef std::function<void(Status status)> ConfigStatusCallback_t;
	typedef std::function<bool(std::string value)> ConfigDataCallback_t;
	typedef std::function<void(bool isSuc)> ConfigDoneCallback_t;

	JSmartconfigService(BLEServer *bleServer);

	void setConfigDataCallback(ConfigDataCallback_t callback);
	void setConfigStatusCallback(ConfigStatusCallback_t callback);
	void setConfigDoneCallback(ConfigDoneCallback_t callback);
	void loop();
	Status getSmartconfigStatus();

	static uint8_t configStep;
	static bool isSkipWifi;

private:

	ConfigDataCallback_t _configDataCallback;
	ConfigStatusCallback_t _configStatusCallback;
	ConfigDoneCallback_t _configDoneCallback;

	BLEService *bleService;
	BLECharacteristic *smartconfigStatusChar, *wifiSsidChar, *smartconfigChar;
	BLE2902 wifiSmartconfigDescriptor;
	JSmartconfigServiceCallback *serviceCallback;

	unsigned long startConfigMillis = 0;
	unsigned long startConfigReMillis = 0;
	bool _isSmartconfigSuc = false;

	void _notify(uint8_t val);

	Status _jsmartconfig_status;
	void _setSmartconfigStatus(Status status);

};


#endif /* JSMARTCONFIGSERVICE_H_ */
