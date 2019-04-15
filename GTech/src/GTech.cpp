/*
 * GTech.cpp
 *
 *  Created on: 2018年1月24日
 *      Author: JimmyTai
 */

#include "GTech.h"

GTech::GTech(const char *product_name, float firmware_ver) :
	_product_name(product_name), _firmware_ver(firmware_ver)
{
	uint64_t mac;
	char deviceIdStr[50];
	memset(deviceIdStr, 0x00, 50);
	mac = ESP.getEfuseMac();
	sprintf(deviceIdStr, "%s-%02X%02X%02X%02X%02X%02X",
			this->_product_name, uint8_t((mac >> 40) & 0xff),
			uint8_t((mac >> 32) & 0xff), uint8_t((mac >> 24) & 0xff),
			uint8_t((mac >> 16) & 0xff), uint8_t((mac >> 8) & 0xff),
			uint8_t(mac & 0xff));
	this->_device_id = String(deviceIdStr);
	char smartconfig_name_str[20];
	memset(smartconfig_name_str, 0x00, 20);
	sprintf(smartconfig_name_str, "%s-%s", "GTech", product_name);
	this->_smartconfig_name = String(smartconfig_name_str);
}

String GTech::getDeviceId(){
	return this->_device_id;
}

void GTech::setDeviceId(String deviceId){
	this->_device_id = deviceId;
}

String GTech::getProductName(){
	return String(this->_product_name);
}

float GTech::getFirmwareVer(){
	return this->_firmware_ver;
}

String GTech::getSmartconfigName(){
	return this->_smartconfig_name;
}


