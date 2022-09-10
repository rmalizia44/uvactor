#ifndef COMMON_EVENTS_HPP
#define COMMON_EVENTS_HPP

#include "event.hpp"

class EvtExit: public EventType<0x4F09D95F> {
public:
	virtual void dump(Writer& writer) const override {}
};

class EvtUpdate: public EventType<0x2E5FAF24> {
public:
	virtual void dump(Writer& writer) const override {}
};

class EvtLog: public EventType<0x34ABEFEF> {
public:
	template<typename... Ts>
	explicit EvtLog(Ts&&... ts) {
		snprintf(text, sizeof(text), std::forward<Ts>(ts)...);
	}
	virtual void dump(Writer& writer) const override {}
	char text[256];
};

#endif