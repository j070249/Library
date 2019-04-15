/*
 * JSmartconfig.h
 *
 *  Created on: 2018年1月26日
 *      Author: JimmyTai
 */

#ifndef JSMARTCONFIG_H_
#define JSMARTCONFIG_H_

#include "JSmartconfigService.h"
#include "OTAService.h"

class JSmartconfig {
public:
	JSmartconfig(BLEServer *bleServer, GTech *gtech);
	~JSmartconfig();
	void begin(
			JSmartconfigService::ConfigDataCallback_t ConfigDataCallback,
			JSmartconfigService::ConfigDoneCallback_t configDoneCallback);
	void setStatusCallback(JSmartconfigService::ConfigStatusCallback_t callback);
	void setOTADoneCallback(OTAService::OTADoneCallback_t callback);

	void addAdvData(std::string data);

	void loop();

	void close();

	JSmartconfigService::Status getStatus();

private:

	GTech *_gtech;

	BLEServer *_bleServer;
	BLEAdvertisementData _advData;
	JSmartconfigService *jSmartconfigService;
	OTAService *ota_service;

	void float2Bytes(char* bytes_temp, float float_variable);

};

#endif /* JSMARTCONFIG_H_ */
