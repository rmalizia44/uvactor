#ifndef LOG_EVENTS_HPP
#define LOG_EVENTS_HPP

#include "event.hpp"

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