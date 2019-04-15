/*
 * OTA.h
 *
 *  Created on: 2018年3月16日
 *      Author: JimmyTai
 */

#ifndef LIBRARIES_GTECH_SRC_OTASERVICE_H_
#define LIBRARIES_GTECH_SRC_OTASERVICE_H_

#include "Arduino.h"
#include "GTech.h"
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLE2902.h"
#include "WiFi.h"
#include "Update.h"

#define DEBUG_OTA

class OTAServiceCallback : public BLECharacteristicCallbacks {

	void onRead(BLECharacteristic *characteristic);
	void onWrite(BLECharacteristic *characteristic);
private:

};

class OTAService {
public:

	typedef std::function<void(bool is_suc)> OTADoneCallback_t;

	static const char *SERVICE;
	static const char *CHAR_OTA;
	static const char *CHAR_OTA_PROGRESS;

	OTAService(BLEServer *ble_server, GTech *gtech);
	void setOTADoneCallback(OTADoneCallback_t callback);
	void loop();

private:

	OTADoneCallback_t _ota_done_callback;

	BLEServer *_ble_server;
	GTech *_gtech;
	OTAServiceCallback *_service_callback;
	BLEService *_ble_service;
	BLECharacteristic *_ota_char;
	BLE2902 _ota_char_desc;

	void _notify(uint8_t val);
	void _notify(uint8_t *val, uint8_t len);

	unsigned long _start_config_millis = 0;
	bool _is_ota_suc = false;

	//https://s3-us-west-2.amazonaws.com/whiztouch-firmware-binary/LED_Blink.bin
	const char *HOST = "s3-us-west-2.amazonaws.com";
	const int PORT = 80;
	const char *PATH = "GET /whiztouch-firmware-binary/%s.bin HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";
	const int TIMEOUT = 10000;
	WiFiClient _wifi_client;
	uint8_t executeOTA();
	uint8_t dealResponse();

};

#endif /* LIBRARIES_GTECH_SRC_OTASERVICE_H_ */
