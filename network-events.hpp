#ifndef ENET_EVENTS_HPP
#define ENET_EVENTS_HPP

#include <string>
#include "event.hpp"

class EvtListen: public EventType<0x745EFDA1> {
public:
	explicit EvtListen(std::string&& _host, uint16_t _port, uint16_t _max_peers): host(std::move(_host)), port(_port), max_peers(_max_peers) {}
	virtual void dump(Writer& writer) const override {}
	std::string host;
	uint16_t port;
	uint16_t max_peers;
};
class EvtConnect: public EventType<0x16A49E5A> {
public:
	explicit EvtConnect(std::string&& _host, uint16_t _port): host(std::move(_host)), port(_port) {}
	virtual void dump(Writer& writer) const override {}
	std::string host;
	uint16_t port;
};
class EvtSend: public EventType<0x7C73679C> {
public:
	explicit EvtSend(uint32_t _dst, std::string&& _buf, bool _reliable): dst(_dst), buf(std::move(_buf)), reliable(_reliable) {}
	virtual void dump(Writer& writer) const override {}
	uint32_t dst;
	std::string buf;
	bool reliable;
};

class EvtKick: public EventType<0xC5D58254> {
public:
	explicit EvtKick(uint32_t _dst): dst(_dst) {}
	virtual void dump(Writer& writer) const override {}
	uint32_t dst;
};

class EvtConnected: public EventType<0x446F3565> {
public:
	explicit EvtConnected(uint16_t _id): id(_id) {}
	virtual void dump(Writer& writer) const override {}
	uint16_t id;
};
class EvtDisconnected: public EventType<0xBF733E42> {
public:
	explicit EvtDisconnected(uint16_t _id): id(_id) {}
	virtual void dump(Writer& writer) const override {}
	uint16_t id;
};
class EvtReceived: public EventType<0xCFB8E7BF> {
public:
	explicit EvtReceived(uint16_t _id, std::string&& _data): id(_id), data(std::move(_data)) {}
	virtual void dump(Writer& writer) const override {}
	uint16_t id;
	std::string data;
};

class EvtPoll: public EventType<0x5FBCA714> {
public:
	virtual void dump(Writer& writer) const override {}
};



#endif