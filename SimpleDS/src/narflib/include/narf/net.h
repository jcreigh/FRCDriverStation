#ifndef NARF_NET_H
#define NARF_NET_H

#include <stdint.h>
#include <string>

#include <unistd.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
typedef int SOCKET;
#endif

#include <vector>


namespace narf {

	namespace net {

		// split net address into host and (optional) port
		bool splitHostPort(const std::string &addr, std::string &host, uint16_t &port);

		class Address {
		public:
			Address(const struct sockaddr_in& ipv4);
			Address(const struct sockaddr_in6& ipv6);
			Address(const struct sockaddr_storage& saStorage);
			Address(const struct sockaddr* sa, socklen_t len);

			const struct sockaddr* sockaddr() const;
			socklen_t sockaddrLen() const;

			bool operator==(const Address& other);

		private:
			union {
				struct sockaddr_storage saStorage;
				struct sockaddr_in ipv4;
				struct sockaddr_in6 ipv6;
			};
		};

		class AddrInfo {
		public:
			AddrInfo(const struct addrinfo& ai);

		private:
			int socktype;
			int protocol;
			Address addr;
			// TODO: canonname?
		};

		bool getaddrinfo(const char* host, const char* service, const struct addrinfo* hints, std::vector<AddrInfo>& results);

		class Socket {
		public:
			Socket(int family, int type, int protocol);
			~Socket();

			void close();
			bool setNonBlocking(bool nonBlocking);
			bool sendto(const void* data, size_t size, const Address& addr);
			bool recvfrom(void* data, size_t availableSize, size_t& received, Address& addr);

			SOCKET sock;
		};
	}
}

#endif // NARF_NET_H
