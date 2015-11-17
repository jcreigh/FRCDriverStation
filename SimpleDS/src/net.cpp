/*----------------------------------------------------------------------------*/
/* Copyright (c) Creighton 2015. All Rights Reserved.                         */
/* Open Source Software - May be modified and shared but must                 */
/* be accompanied by the license file in the root source directory            */
/*----------------------------------------------------------------------------*/

#include "narf/tokenize.h"
#include "net.h"

Net::Net() : initializedIn(false), initializedOut(false) {
}

void Net::initSocketIn() {
	if (initializedIn) {
		close(sockIn);
		initializedIn = false;
	}

	if ((sockIn = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		perror("Socket");
		return;
	}

	sockaddr_in sockIn_addr;
	memset((char*) &sockIn_addr, 0, sizeof(sockIn_addr));
	sockIn_addr.sin_family = AF_INET;
	sockIn_addr.sin_port = htons(1150);
	sockIn_addr.sin_addr.s_addr = INADDR_ANY;

	int val = 1;
	setsockopt(sockIn, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int));
	fcntl(sockIn, F_SETFL, O_NONBLOCK);
	if (bind(sockIn, (sockaddr*)&sockIn_addr, sizeof(sockIn_addr)) == -1) {
		perror("Bind");
		close(sockIn);
		return;
	}
	initializedIn = true;
}

bool Net::initSocketOut(std::string host) {
	if (initializedOut) {
		close(sockOut);
		initializedOut = false;
	}

	addrinfo* result;
	addrinfo hints;

	memset((char*) &hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	while (getaddrinfo(host.c_str(), "1110", &hints, &result)) {
		//perror("getaddrinfo");
	}

	addrinfo* res;
	for (res = result; res != NULL; res = res->ai_next) {
		if ((sockOut = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
			continue;
		}
		break;
	}

	memset((char*) &sockOut_addr, 0, sizeof(sockOut_addr));
	if (res != NULL) {
		memcpy(&sockOut_addr, res->ai_addr, res->ai_addrlen);
		freeaddrinfo(result);
		initializedOut = true;
	}

	return initializedOut;
}

int Net::send(std::string data) {
	if (initializedOut) {
		return (int)sendto(sockOut, data.c_str(), data.size(), 0, (sockaddr*)&sockOut_addr, (uint32_t)sizeof(sockOut_addr));
	}
	return 0;
}

std::string Net::recv() {
	if (initializedIn) {
		char buf[BUFSIZE];
		sockaddr_in sockOther;
		socklen_t slen = sizeof(sockOther);
		auto rv = recvfrom(sockIn, buf, BUFSIZE, 0, (sockaddr*)&sockOther, (socklen_t*)&slen);
		if (rv > 0) {
			return std::string(buf, rv);
		}
	}
	return "";
}
