#include "narf/net.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


using namespace narf;

net::Socket::Socket(int family, int type, int protocol) {
	sock = socket(family, type, protocol);
	// TODO: check return value
}


net::Socket::~Socket() {
	if (sock) { // TODO: what is invalid socket value? -1?
		close();
	}
}


void net::Socket::close() {
#ifdef _WIN32
	closesocket(sock);
#else
	::close(sock);
#endif
}


bool net::Socket::setNonBlocking(bool nonblocking) {
#ifdef _WIN32
	DWORD dw = nonblocking ? 1 : 0;
	return ioctlsocket(sock, FIONBIO, &dw) == 0;
#else
	auto flags = fcntl(sock, F_GETFL, 0);
	if (nonblocking) {
		flags |= O_NONBLOCK;
	} else {
		flags &= ~O_NONBLOCK;
	}
	return fcntl(sock, F_SETFL, flags) != -1;
#endif
}


bool net::Socket::sendto(const void* data, size_t size, const Address& addr) {
#ifdef _WIN32
	auto d = static_cast<const char*>(data);
	auto sz = static_cast<int>(size);
#else
	const void* d = data;
	size_t sz = size;
#endif
	ssize_t sent = ::sendto(sock, d, sz, 0, addr.sockaddr(), addr.sockaddrLen());
	return sent == static_cast<ssize_t>(size);
}


bool net::Socket::recvfrom(void* data, size_t availableSize, size_t& received, Address& fromAddr) {
	struct sockaddr_storage addr;
	auto sa = reinterpret_cast<struct sockaddr*>(&addr);
	socklen_t addrLen = sizeof(addr);

#ifdef _WIN32
	auto d = static_cast<char*>(data);
	auto sz = static_cast<int>(availableSize);
#else
	void* d = data;
	size_t sz = availableSize;
#endif

	ssize_t rc = ::recvfrom(sock, d, sz, 0, sa, &addrLen);

	received = static_cast<size_t>(rc);
	fromAddr = Address(addr);

	return rc >= 0;
}




#ifdef _WIN32
class WinsockInit {
public:
	WinsockInit() {
		WSAData wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
			abort();
		}
	}

	~WinsockInit() {
		WSACleanup();
	}
};

// make a file-scope object to run the constructor
static WinsockInit winsockInit;
#endif
