#ifndef COMMON_EVENTS_HPP
#define COMMON_EVENTS_HPP

#include "event.hpp"

class EvtExit: public EventType<0x4F09D95F> {
public:
	virtual void dump(Writer& writer) const override {}
};


#endif