/*
 * WiFiConfig.h
 *
 *  Created on: 2018年1月25日
 *      Author: JimmyTai
 */

#ifndef WIFICONFIG_H_
#define WIFICONFIG_H_

#include "Arduino.h"
#include "FS.h"
#include "SPIFFS.h"

class WiFiConfigClass {
public:

	void save(String ssid, String password);
	void clear();
	bool isConfiged();

	void getSSID(String *ssid);
	void getPassword(String *password);
	void getConfig(String *ssid, String *password);

private:

	const char *SSID_FILE_PATH = "/wifi_ssid_config.txt";
	const char *PASSWORD_FILE_PATH = "/wifi_password_config.txt";

};

extern WiFiConfigClass WiFiConfig;


#endif /* WIFICONFIG_H_ */
