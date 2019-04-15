/*
 * OTA.cpp
 *
 *  Created on: 2018年3月16日
 *      Author: JimmyTai
 */

#include <OTAService.h>

const char *OTAService::SERVICE = "342300E0-4A54-0315-0082-472D54656368";
const char *OTAService::CHAR_OTA = "342300E1-4A54-0315-0082-472D54656368";
const char *OTAService::CHAR_OTA_PROGRESS = "342300E2-4A54-0315-0082-472D54656368";

BLECharacteristic *_ota_progress_char;
BLE2902 _ota_progress_char_desc;

/* wifi */
uint8_t _ota_config_step = 0xff;
String _ota_attempt_ssid = "", _ota_attempt_password = "";
/* ota url */
bool _ota_is_attempt_url = false;
std::string _ota_attempt_url = "";

uint8_t _ota_percentage = 0, _ota_percentage_old = 0;

void onOTAProgress(size_t progress, size_t size) {
	Serial.printf("total: %d, progress: %d\r\n", size, progress);
	_ota_percentage = uint8_t(double(progress) / double(size) * 100.0);
	Serial.printf("ota progress percentage: %d\r\n", _ota_percentage);
	if (_ota_percentage != _ota_percentage_old) {
		_ota_progress_char->setValue(&_ota_percentage, 1);
		_ota_progress_char->notify();
	}
	_ota_percentage_old = _ota_percentage;
}

OTAService::OTAService(BLEServer *ble_server, GTech *gtech) :
		_ota_char_desc()
{
	this->_gtech = gtech;
	this->_ble_server = ble_server;
	this->_service_callback = new OTAServiceCallback();
	this->_ble_service = ble_server->createService(BLEUUID(SERVICE));
	this->_ota_char = _ble_service->createCharacteristic(BLEUUID(CHAR_OTA)
			, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
	this->_ota_char->addDescriptor(&_ota_char_desc);
	this->_ota_char->setCallbacks(_service_callback);
	_ota_progress_char = _ble_service->createCharacteristic(BLEUUID(CHAR_OTA_PROGRESS)
			, BLECharacteristic::PROPERTY_NOTIFY);
	_ota_progress_char->addDescriptor(&_ota_progress_char_desc);
	this->_ble_service->start();
	Update.onProgress(onOTAProgress);
}

void OTAService::setOTADoneCallback(OTADoneCallback_t callback) {
	this->_ota_done_callback = callback;
}

void OTAService::_notify(uint8_t val) {
	this->_ota_char->setValue(&val, 1);
	this->_ota_char->notify();
}

void OTAService::_notify(uint8_t *val, uint8_t len) {
	this->_ota_char->setValue(val, len);
	this->_ota_char->notify();
}

void OTAService::loop() {
	if (_ota_config_step == 0xff)
		return;
	if (_ota_config_step > 0x00 && millis() - this->_start_config_millis > 300000) {
		_ota_config_step = 0x03;
		_notify(0xfe);
		this->_is_ota_suc = false;
		return;
	}
	if (_ota_config_step == 0x00) {
		this->_start_config_millis = millis();
		this->_is_ota_suc = false;
		_ota_percentage = 0;
		_ota_config_step = 0x01;
		if (WiFi.isConnected())
			WiFi.disconnect(0);
		Serial.printf("Connect to wifi (%s, %s)\r\n", _ota_attempt_ssid.c_str(), _ota_attempt_password.c_str());
		WiFi.begin(_ota_attempt_ssid.c_str(), _ota_attempt_password.c_str());
		_notify(0x01);
		Serial.printf("notify 0x01\r\n");
	} else if (_ota_config_step == 0x01) {
		/* --- wifi connection timeout --- */
		if (millis() - this->_start_config_millis > 30000) {
			_ota_config_step = 0x03;
			_notify(0xff);
			this->_is_ota_suc = false;
			return;
		}
		if (WiFi.isConnected()) {
			_ota_config_step = 0x02;
			_notify(0x02);
		}
	} else if (_ota_config_step == 0x02) {
		if (_ota_is_attempt_url) {
			this->_start_config_millis = 0;
			_ota_is_attempt_url = false;
			_ota_config_step = 0x03;
			// todo: run ota and get result
			uint8_t response = executeOTA();
			Serial.printf("ota response: %d\r\n", response);
			uint8_t val[2] = {0x03, response};
			_notify(val, 2);
			this->_is_ota_suc = (response == 0x01);
			return;
		}
	} else if (_ota_config_step == 0x03) {
		_ota_config_step = 0xff;
		if (_ota_done_callback != NULL)
			_ota_done_callback(_is_ota_suc);
	}
}

uint8_t OTAService::executeOTA() {
	if (WiFi.status() != WL_CONNECTED)
		return false;
	_wifi_client.setTimeout(5000);
	if (!_wifi_client.connect(HOST, PORT)) {
		Serial.printf("[OTA] cannot connect to host....\r\n");
		return false;
	}
	char uri[200];
	memset(uri, 0x00, 200);
	sprintf(uri, PATH, _ota_attempt_url.c_str(), HOST);
	Serial.printf("[OTA] uri: %s\r\n", uri);
	_wifi_client.print(uri);

	return dealResponse();
}

/*
 * return	0xff: 連線為建立或無效
 * 			0xfe: 屬性無效，或不足
 * 			0xfd: 檔案大小錯誤
 * 			0x00: OTA過程錯誤
 * 			0x01: OTA成功
 *
 */
uint8_t OTAService::dealResponse() {
	uint64_t lastDataTime = millis();
	uint8_t code = 0;
	bool is_start_ota = false;
	String firmware_ver = "";
	int content_len = 0;
	bool is_content_type_valid = false;
	if (!(_wifi_client.connected() || _wifi_client.available() > 0)) {
		Serial.printf("client not connected\r\n");
		return 0xff;
	}
	while ((_wifi_client.connected() || _wifi_client.available() > 0)) {
		size_t len = _wifi_client.available();
		if (len > 0) {
			lastDataTime = millis();
			String headerLine = _wifi_client.readStringUntil('\n');
			headerLine.trim();
			if (!headerLine.length()) {
				is_start_ota = true;
				break;
			}
			if (headerLine.startsWith("HTTP/1.")) {
				code = headerLine.substring(9, headerLine.indexOf(' ', 9)).toInt();
			} else if (headerLine.indexOf(':')) {
				String headerName = headerLine.substring(0, headerLine.indexOf(':'));
				String headerValue = headerLine.substring(headerLine.indexOf(':') + 2);
				Serial.printf("header name: %s, header value: %s\r\n", headerName.c_str(),
						headerValue.c_str());
				if (headerName.equals("x-amz-meta-firmware")) {
					firmware_ver = headerValue;
				}
				if (headerName.equals("Content-Type")) {
					for(uint8_t i = 0; i < headerValue.length(); i++) {
						Serial.printf("%d ", headerValue.c_str()[i]);
					}
					Serial.printf("\r\n");
				}
				if (headerName.equals("Content-Type") && headerValue.equals("application/macbinary"))
					is_content_type_valid = true;
				if (headerName.equals("Content-Length")) {
					content_len = headerValue.toInt();
				}
			}
		} else {
			if ((millis() - lastDataTime) > TIMEOUT) {
				return 0xff;
			}
			delay(0);
		}
	}
	Serial.printf("deal response end\r\n");
	if (!is_start_ota) {
		Serial.printf("ota not start\r\n");
		_wifi_client.flush();
		return 0xff;
	}
	if (code != 200) {
		Serial.printf("ota code is not 200\r\n");
		_wifi_client.flush();
		return 0xff;
	}
	if (!is_content_type_valid) {
		Serial.printf("ota content type invalid\r\n");
		_wifi_client.flush();
		return 0xfe;
	}
	if (String(_gtech->getFirmwareVer()).equals(firmware_ver)) {
		Serial.printf("ota firmware is the same\r\n");
		_wifi_client.flush();
		return 0xfe;
	}
	Serial.printf("content length: %d\r\n", content_len);
	if (!Update.begin(content_len)) {
		Serial.printf("ota content length is not okay\r\n");
		_wifi_client.flush();
		return 0xfd;
	}
    size_t written = Update.writeStream(_wifi_client);
    Serial.printf("written len: %d\r\n", written);

	if (written == content_len) {
		Serial.println("Written : " + String(written) + " successfully");
	} else {
		_wifi_client.flush();
		return 0xfd;
	}
	if (Update.end()) {
		Serial.println("OTA done!");
		if (Update.isFinished()) {
			Serial.println("Update successfully completed. Rebooting.");
			_wifi_client.flush();
			return 0x01;
		} else {
			Serial.println("Update not finished? Something went wrong!");
		}
	} else {
		Serial.println("Error Occurred. Error #: " + String(Update.getError()));
	}
	_wifi_client.flush();
	return 0x00;
}

void OTAServiceCallback::onRead(BLECharacteristic *characteristic) {
}

void OTAServiceCallback::onWrite(BLECharacteristic *characteristic) {
	Serial.printf("[OTA] onWrite: uuid -> %s\r\n", characteristic->getUUID().toString().c_str());
	if (characteristic->getUUID().equals(BLEUUID(OTAService::CHAR_OTA))) {
#ifdef DEBUG_OTA
		Serial.printf("[OTA] \tpayload: ");
		for (uint8_t i = 0; i < characteristic->getValue().size(); i++) {
			Serial.printf(" %02X", characteristic->getValue().data()[i]);
		}
		Serial.printf("\r\n");
#endif
		if (_ota_config_step == 0xff) {
			if (characteristic->getValue().size() < 12
					|| characteristic->getValue().data()[0] != 0x00) {
#ifdef DEBUG_OTA
				Serial.printf("[OTA] format is wrong\r\n");
#endif
				return;
			}
			uint8_t len_ssid = characteristic->getValue().data()[1];
			uint8_t len_password = characteristic->getValue().data()[len_ssid + 2];
			_ota_attempt_ssid = String(
					characteristic->getValue().substr(2,  len_ssid).c_str());
			_ota_attempt_password = String(
					characteristic->getValue().substr(3 + len_ssid, len_password).c_str());
#ifdef DEBUG_OTA
			Serial.printf("[OTA] start with ssid: %s, password: %s\r\n"
					, _ota_attempt_ssid.c_str(), _ota_attempt_password.c_str());
#endif
			_ota_config_step = 0x00;
		} else if (_ota_config_step == 0x02) {
			_ota_attempt_url = characteristic->getValue();
			_ota_is_attempt_url = true;
		}
	}
}
