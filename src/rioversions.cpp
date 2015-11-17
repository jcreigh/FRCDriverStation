/*----------------------------------------------------------------------------*/
/* Copyright (c) Creighton 2015. All Rights Reserved.                         */
/* Open Source Software - May be modified and shared but must                 */
/* be accompanied by the license file in the root source directory            */
/*----------------------------------------------------------------------------*/

#include "rioversions.h"

size_t callback(void* data, size_t size, size_t nmemb, void* p) {
	auto bytes = (char*)data;
	auto buf = (std::string*)p;
	int real_size = (int)(size * nmemb);
	for (int i = 0; i < real_size; i++) {
		if (bytes[i]) {
			(*buf) += bytes[i];
		}
	}
	return real_size;
}

std::string curlFirmwareVersion(uint16_t teamNum) {
	CURL* curl_handle = curl_easy_init();
	const char* postData = "Function=GetPropertiesOfItem&Plugins=nisyscfg&Items=system";
	std::string buf;
	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl_handle, CURLOPT_URL, narf::util::format("http://roborio-%d.local/nisysapi/server", teamNum).c_str());
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, postData);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, callback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &buf);
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT_MS, 1000);
	CURLcode res = curl_easy_perform(curl_handle);
	curl_easy_cleanup(curl_handle);
	if (res == CURLE_OK) {
		auto start = buf.find("D15C000") + 18;
		auto end = buf.find("<", start);
		return buf.substr(start, end - start);
	} else {
	}
	return "";
}

std::string curlLibVersion(uint16_t teamNum) {
	CURL* curl_handle = curl_easy_init();
	std::string buf;
	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl_handle, CURLOPT_URL, narf::util::format("ftp://roborio-%d.local/tmp/frc_versions/FRC_Lib_Version.ini", teamNum).c_str());
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, callback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &buf);
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT_MS, 1000);
	CURLcode res = curl_easy_perform(curl_handle);
	curl_easy_cleanup(curl_handle);
	if (res == CURLE_OK) {
		return buf;
	} else {
	}
	return "";
}

