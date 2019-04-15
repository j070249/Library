/*
 * JSmartconfigService.cpp
 *
 *  Created on: 2018年1月24日
 *      Author: JimmyTai
 */

#include "JSmartconfigService.h"

uint8_t JSmartconfigService::configStep = 0xff;
bool JSmartconfigService::isSkipWifi = false;

JSmartconfigService::JSmartconfigService(BLEServer *bleServer) :
	wifiSmartconfigDescriptor()
{
	bleService = bleServer->createService(JSmartconfigUUID::SERVICE);
	serviceCallback = new JSmartconfigServiceCallback();
	smartconfigStatusChar = bleService->createCharacteristic
			(JSmartconfigUUID::CHAR_SMARTCONFIG_STATUS, BLECharacteristic::PROPERTY_READ);
	_setSmartconfigStatus(Status::IDLE);
	wifiSsidChar = bleService->createCharacteristic
			(JSmartconfigUUID::CHAR_WIFI_SSID, BLECharacteristic::PROPERTY_READ);
	String ssid;
	WiFiConfig.getSSID(&ssid);
	wifiSsidChar->setValue(ssid.c_str());
	smartconfigChar = bleService->createCharacteristic
			(JSmartconfigUUID::CHAR_SMARTCONFIG
			, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
	smartconfigChar->addDescriptor(&wifiSmartconfigDescriptor);
	smartconfigChar->setCallbacks(serviceCallback);
	bleService->start();
}

void JSmartconfigService::setConfigDataCallback(ConfigDataCallback_t callback) {
	_configDataCallback = callback;
}

void JSmartconfigService::setConfigStatusCallback(ConfigStatusCallback_t callback) {
	_configStatusCallback = callback;
}

void JSmartconfigService::setConfigDoneCallback(ConfigDoneCallback_t callback) {
	_configDoneCallback = callback;
}

BLECharacteristic *_temp_smartconfig_char;

// 0x01: wifi connecting, 0x02: wifi connected, 0x03: config success
// 0xff: wifi cannot connect, 0xfe: config failure
void JSmartconfigService::loop() {
	if (configStep == 0xff)
		return;
	if (configStep > 0x00 && millis() - startConfigMillis > 300000) {
		configStep = 0x03;
		_notify(0xfe);
		this->_isSmartconfigSuc = false;
		return;
	}
	if(configStep == 0x00) {
		if (JSmartconfigService::isSkipWifi) {
			startConfigMillis = millis();
			startConfigReMillis = millis();
			configStep = 0x01;
			return;
		}
		_setSmartconfigStatus(Status::WIFI_CONNECTING);
		startConfigMillis = millis();
		startConfigReMillis = millis();
		configStep = 0x01;
		if (WiFi.isConnected())
			WiFi.disconnect(0);
		WiFi.begin(serviceCallback->_attempt_ssid.c_str(), serviceCallback->_attempt_password.c_str());
		Serial.printf("%s/%s",serviceCallback->_attempt_ssid.c_str(),serviceCallback->_attempt_password.c_str());
		_notify(0x01);
	} else if(configStep == 0x01) {
		if (JSmartconfigService::isSkipWifi) {
			Serial.print("isSkipWifi");
			delay(500);
			configStep = 0x02;
			_notify(0x02);
			delay(500);
			return;
		}
		if(millis() - startConfigReMillis > 10000){
			Serial.print("startConfigReMillis\r\n");
			startConfigReMillis = millis();
			WiFi.disconnect();
			WiFi.begin(serviceCallback->_attempt_ssid.c_str(), serviceCallback->_attempt_password.c_str());
		}
		if(millis() - startConfigMillis > 60000){
			Serial.print("startConfigMillis");
			configStep = 0x03;
			_notify(0xff);
			this->_isSmartconfigSuc = false;
			return;
		}
		if (WiFi.isConnected()) {
			Serial.print("isConnected");
			_setSmartconfigStatus(Status::DATA_CONFIGURING);
			configStep = 0x02;
			_notify(0x02);
		}
	} else if (configStep == 0x02) {
		if (serviceCallback->_isAttemptConfigData) {
			serviceCallback->_isAttemptConfigData = false;
			configStep = 0x03;
			uint8_t response =
					(_configDataCallback == NULL
							|| _configDataCallback(
									serviceCallback->_attempt_config_data)) ?
							0x03 : 0xfe;
			if(response == 0x03 && !JSmartconfigService::isSkipWifi){
				WiFiConfig.save(serviceCallback->_attempt_ssid, serviceCallback->_attempt_password);
				wifiSsidChar->setValue(serviceCallback->_attempt_ssid.c_str());
			}
			_notify(response);
			_isSmartconfigSuc = response == 0x03;
			return;
		}
	} else if(configStep == 0x03){
		_setSmartconfigStatus(Status::IDLE);
		configStep = 0xff;
		isSkipWifi = false;
		if (_configDoneCallback != NULL)
			_configDoneCallback(_isSmartconfigSuc);
	}
}

void JSmartconfigService::_notify(uint8_t val){
	_temp_smartconfig_char->setValue(&val, 1);
	_temp_smartconfig_char->notify();
}

JSmartconfigService::Status JSmartconfigService::getSmartconfigStatus() {
	return _jsmartconfig_status;
}

void JSmartconfigService::_setSmartconfigStatus(Status status) {
	this->_jsmartconfig_status = status;
	uint8_t val = status;
	smartconfigStatusChar->setValue(&val, 1);
	if (_configStatusCallback != NULL)
		_configStatusCallback(status);
}

void JSmartconfigServiceCallback::onRead(BLECharacteristic *characteristic){
	Serial.printf("[JSmartconfig] onRead: uuid -> %s\r\n", characteristic->getUUID().toString().c_str());
}

void JSmartconfigServiceCallback::onWrite(BLECharacteristic *characteristic){
	Serial.printf("[JSmartconfig] onWrite: uuid -> %s\r\n", characteristic->getUUID().toString().c_str());
	if(characteristic->getUUID().equals(
			BLEUUID(JSmartconfigUUID::CHAR_SMARTCONFIG))) {
		_temp_smartconfig_char = characteristic;
		Serial.printf("[JSmartconfig] \tpayload: ");
		for (uint8_t i = 0; i < characteristic->getValue().size(); i++) {
			Serial.printf(" %02X", characteristic->getValue().data()[i]);
		}
		Serial.printf("\r\n");
		if(JSmartconfigService::configStep == 0xff){
			if ((characteristic->getValue().size() != 3 && characteristic->getValue().size() < 12)
					|| characteristic->getValue().data()[0] != 0x00) {
				Serial.printf("[JSmartconfig] format is wrong\r\n");
				return;
			}
			if (characteristic->getValue().size() == 3) {
				if (characteristic->getValue().data()[0] == 0x00 &&
						characteristic->getValue().data()[1] == 0x00 &&
						characteristic->getValue().data()[2] == 0x00) {
					Serial.printf("[JSmartconfig] start without WiFi setting\r\n");
					JSmartconfigService::configStep = 0x00;
					JSmartconfigService::isSkipWifi = true;
				}
				return;
			}
			uint8_t len_ssid = characteristic->getValue().data()[1];
			uint8_t len_password = characteristic->getValue().data()[len_ssid + 2];
			_attempt_ssid = String(
					characteristic->getValue().substr(2, len_ssid).c_str());
			_attempt_password = String(
					characteristic->getValue().substr(3 + len_ssid,
							len_password).c_str());
			Serial.printf("[JSmartconfig] start with ssid: %s, password: %s\r\n"
					, _attempt_ssid.c_str(), _attempt_password.c_str());
			JSmartconfigService::configStep = 0x00;
		} else if (JSmartconfigService::configStep == 0x02) {
			_attempt_config_data = characteristic->getValue();
			_isAttemptConfigData = true;
		}
	}
}
