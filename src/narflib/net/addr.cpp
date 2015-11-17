#include "narf/net.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

using namespace narf;

bool net::splitHostPort(const std::string &addr, std::string &host, uint16_t &port) {
	size_t portPos;
	host = addr;
	port = 0;
	if (addr[0] == '[') {
		// [host]:port
		size_t hostEnd = addr.find(']');
		if (hostEnd == std::string::npos) {
			// missing closing bracket
			return false;
		}

		host = addr.substr(1, hostEnd - 1);
		portPos = addr.find(':', hostEnd + 1);
	} else {
		portPos = addr.find(':');
		if (portPos != std::string::npos) {
			host = addr.substr(0, portPos);
		}
	}

	if (host.find_first_of("[]") != std::string::npos) {
		// stray brackets in address
		return false;
	}

	if (portPos == std::string::npos) {
		port = 0;
	} else {
		auto portStr = addr.substr(portPos + 1);
		if (portStr.find(':') != std::string::npos) {
			// too many colons
			// TODO: handle bare IPv6 addresses with no port?
			return false;
		}

		char *end;
		unsigned long p = strtoul(portStr.c_str(), &end, 10);
		if (!end || *end != '\0' || p == 0 || p > 65535ul) {
			// invalid port
			return false;
		}

		port = (uint16_t)p;
	}

	return true;
}


net::Address::Address(const struct sockaddr_in& ipv4) : ipv4(ipv4) {
}


net::Address::Address(const struct sockaddr_in6& ipv6) : ipv6(ipv6) {
}


net::Address::Address(const struct sockaddr_storage& saStorage) : saStorage(saStorage) {
}

net::Address::Address(const struct sockaddr* sa, socklen_t len) {
	assert(len <= sizeof(saStorage));
	memcpy(&saStorage, sa, len);
}

const struct sockaddr* net::Address::sockaddr() const {
	return reinterpret_cast<const struct sockaddr*>(&saStorage);
}


socklen_t net::Address::sockaddrLen() const {
	switch (saStorage.ss_family) {
	case AF_INET:   return sizeof(struct sockaddr_in);
	case AF_INET6:  return sizeof(struct sockaddr_in6);
	}
	assert(0);
	return 0;
}


bool net::Address::operator==(const Address& other) {
	if (saStorage.ss_family != other.saStorage.ss_family) {
		return false;
	}

	switch (saStorage.ss_family) {
	case AF_INET:
		return
			ipv4.sin_port == other.ipv4.sin_port &&
			ipv4.sin_addr.s_addr == other.ipv4.sin_addr.s_addr;
	case AF_INET6:
		return
			ipv6.sin6_port == other.ipv6.sin6_port &&
			memcmp(&ipv6.sin6_addr, &other.ipv6.sin6_addr, sizeof(ipv6.sin6_addr)) == 0 &&
			ipv6.sin6_scope_id == other.ipv6.sin6_scope_id;
	}

	assert(0);
	return false;
}


net::AddrInfo::AddrInfo(const struct addrinfo& ai) :
	socktype(ai.ai_socktype),
	protocol(ai.ai_protocol),
	addr(ai.ai_addr, static_cast<socklen_t>(ai.ai_addrlen)) {
}


bool net::getaddrinfo(const char* host, const char* service, const struct addrinfo* hints, std::vector<net::AddrInfo>& results) {
	results.clear();
	struct addrinfo* res;
	int rc = ::getaddrinfo(host, service, hints, &res);
	if (rc != 0) {
		return false;
	}

	for (auto r = res; r != nullptr; r = r->ai_next) {
		results.emplace_back(*r);
	}

	::freeaddrinfo(res);

	return true;
}
