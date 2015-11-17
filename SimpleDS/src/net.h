/*----------------------------------------------------------------------------*/
/* Copyright (c) Creighton 2015. All Rights Reserved.                         */
/* Open Source Software - May be modified and shared but must                 */
/* be accompanied by the license file in the root source directory            */
/*----------------------------------------------------------------------------*/

#ifndef _NET_H_
#define _NET_H_

#define BUFSIZE 1024

#include <atomic>
#include <string>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

class Net {
	private:
		std::atomic_bool initializedIn;
		std::atomic_bool initializedOut;
		int sockIn;
		int sockOut;
		sockaddr_in sockOut_addr;

		std::atomic_flag connecting;
		bool connected;

	public:
		Net();
		void initSocketIn();
		bool initSocketOut(std::string host);

		int send(std::string data);
		std::string recv();
};

#endif /* _NET_H_ */
