/*
 * JSmartconfig.cpp
 *
 *  Created on: 2018年1月26日
 *      Author: JimmyTai
 */

#include "JSmartconfig.h"
#include "BleServer.h"

// todo: should combine two service into one: GTechService

JSmartconfig::JSmartconfig(BLEServer *bleServer, GTech *gtech)
	: _gtech(gtech), _bleServer(bleServer)
{
	jSmartconfigService = new JSmartconfigService(bleServer);
	ota_service = new OTAService(bleServer, gtech);
	_advData.setFlags(
			ESP_BLE_ADV_FLAG_BREDR_NOT_SPT | ESP_BLE_ADV_FLAG_GEN_DISC);
	_advData.setManufacturerData("4#GTech");
	char versionData[2];
	char firmware_ver[4];
	float2Bytes(firmware_ver, _gtech->getFirmwareVer());
	versionData[0] = 5;
	versionData[1] = 0x89;
	_advData.addData(std::string(versionData, 2) + std::string(firmware_ver, 4));
	_bleServer->getAdvertising()->setAdvertisementData(_advData);
	BLEAdvertisementData scanResponseData;
	scanResponseData.setName(_gtech->getSmartconfigName().c_str());
	uint64_t mac = ESP.getEfuseMac();
	char mac_addr[6] = {
			uint8_t((mac >> 40) & 0xff), uint8_t((mac >> 32) & 0xff),
			uint8_t((mac >> 24) & 0xff), uint8_t((mac >> 16) & 0xff),
			uint8_t((mac >> 8) & 0xff), uint8_t(mac & 0xff)
	};
	char id_data[2] = {7, 0x90};
	scanResponseData.addData(std::string(id_data, 2) + std::string(mac_addr, 6));
	_bleServer->getAdvertising()->setScanResponseData(scanResponseData);
}
// 108312
// 113260
JSmartconfig::~JSmartconfig() {
	delete jSmartconfigService;
	delete ota_service;
}

void JSmartconfig::addAdvData(std::string data) {
	_advData.addData(data);
	_bleServer->getAdvertising()->setAdvertisementData(_advData);
}

void JSmartconfig::begin(
		JSmartconfigService::ConfigDataCallback_t configDataCallback,
		JSmartconfigService::ConfigDoneCallback_t configDoneCallback) {
	jSmartconfigService->setConfigDataCallback(configDataCallback);
	jSmartconfigService->setConfigDoneCallback(configDoneCallback);

	_bleServer->getAdvertising()->start();
}

void JSmartconfig::setStatusCallback(JSmartconfigService::ConfigStatusCallback_t callback) {
	jSmartconfigService->setConfigStatusCallback(callback);
}

void JSmartconfig::setOTADoneCallback(OTAService::OTADoneCallback_t callback) {
	ota_service->setOTADoneCallback(callback);
}

void JSmartconfig::loop() {
	jSmartconfigService->loop();
	ota_service->loop();
}

void JSmartconfig::close() {
	_bleServer->getAdvertising()->stop();
}

void JSmartconfig::float2Bytes(char* bytes_temp, float float_variable){
  union {
    float a;
    unsigned char bytes[4];
  } thing;
  thing.a = float_variable;
  memcpy(bytes_temp, thing.bytes, 4);
}

JSmartconfigService::Status JSmartconfig::getStatus(){
	return jSmartconfigService->getSmartconfigStatus();
}
