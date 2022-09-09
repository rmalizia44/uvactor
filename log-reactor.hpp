#ifndef LOG_REACTOR_HPP
#define LOG_REACTOR_HPP

#include <cstdio>
#include "common-events.hpp"
#include "log-events.hpp"
#include "reactor.hpp"

class LogReactor: public Reactor {
public:
	explicit LogReactor(SelfPtr self): _self(self) {
		setbuf(stdout, nullptr);
	}
	void dump(Writer& writer) const override {}
	void react(const EventPtr& event, uint64_t timestamp) override {
		switch(event->type) {
			case EvtLog::TYPE:
				printf("[L] %s (%llu)\n", event->as<EvtLog>().text, (long long unsigned int)timestamp);
				break;
			case EvtExit::TYPE:
				printf("[L] exit (%llu)\n", (long long unsigned int)timestamp);
				_self->reset();
				break;
			default:
				printf("[L] unknown<%u> (%llu)\n", event->type, (long long unsigned int)timestamp);
				break;
		}
	}
private:
	SelfPtr _self;
};


#endif