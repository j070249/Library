/*
 * GTech.h
 *
 *  Created on: 2018年1月24日
 *      Author: JimmyTai
 */

#ifndef GTECH_H_
#define GTECH_H_

#include "Arduino.h"
#include "WiFiConfig.h"

class GTech {
public:

	GTech(const char *productName, float firmware_ver);

	String getDeviceId();
	void setDeviceId(String deviceId);
	String getProductName();
	float getFirmwareVer();
	String getSmartconfigName();

private:

	const char *_product_name;
	float _firmware_ver;
	String _device_id, _smartconfig_name;

};


#endif /* GTECH_H_ */
