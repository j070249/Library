/*
 * WiFiConfig.cpp
 *
 *  Created on: 2018年1月25日
 *      Author: JimmyTai
 */

#include "WiFiConfig.h"

void WiFiConfigClass::save(String ssid, String password) {
	if (!SPIFFS.begin(true)) {
		return;
	}

	File file = SPIFFS.open(SSID_FILE_PATH, "w");
	if (!file)
		return;
	file.write((const uint8_t*) ssid.c_str(), ssid.length());
	file = SPIFFS.open(PASSWORD_FILE_PATH, "w");
	if (!file)
		return;
	file.write((const uint8_t*) password.c_str(), password.length());
	file.close();
	SPIFFS.end();
}

void WiFiConfigClass::clear() {
	if (!SPIFFS.begin(true)) {
		return;
	}
	SPIFFS.remove(SSID_FILE_PATH);
	SPIFFS.remove(PASSWORD_FILE_PATH);
	SPIFFS.end();
}

bool WiFiConfigClass::isConfiged() {
	if (!SPIFFS.begin(true)) {
		return false;
	}
	if (!SPIFFS.exists(SSID_FILE_PATH)){
		SPIFFS.end();
		return false;
	}
	if (!SPIFFS.exists(PASSWORD_FILE_PATH)){
		SPIFFS.end();
		return false;
	}
	SPIFFS.end();
	return true;
}

void WiFiConfigClass::getSSID(String *ssid) {
	if (!SPIFFS.begin(true)) {
		*ssid = "";
		SPIFFS.end();
		return;
	}
	File file = SPIFFS.open(SSID_FILE_PATH, "r");
	if (!file) {
		*ssid = "";
		SPIFFS.end();
		return;
	}
	*ssid = file.readString();
	file.close();
	SPIFFS.end();
}

void WiFiConfigClass::getPassword(String *password) {
	if (!SPIFFS.begin(true)) {
		*password = "";
		SPIFFS.end();
		return;
	}
	File file = SPIFFS.open(PASSWORD_FILE_PATH, "r");
	if (!file) {
		*password = "";
		SPIFFS.end();
		return;
	}
	*password = file.readString();
	file.close();
	SPIFFS.end();
}

void WiFiConfigClass::getConfig(String *ssid, String *password) {
	if (!SPIFFS.begin(true)) {
		*ssid = "";
		*password = "";
		SPIFFS.end();
		return;
	}
	File file = SPIFFS.open(SSID_FILE_PATH, "r");
	if (!file) {
		*ssid = "";
	} else {
		*ssid = file.readString();
	}
	file.close();

	file = SPIFFS.open(PASSWORD_FILE_PATH, "r");
	if (!file) {
		*password = "";
	} else {
		*password = file.readString();
	}
	file.close();
	SPIFFS.end();
}

WiFiConfigClass WiFiConfig;

