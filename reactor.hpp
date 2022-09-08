#ifndef REACTOR_HPP
#define REACTOR_HPP

#include "event.hpp"
#include "actor.hpp"

class Reactor {
public:
	using EventPtr = Event::SharedPtr;
	using ActorPtr = Actor::SharedPtr;
	using SelfPtr = ActorSelf::SharedPtr;
	
	virtual ~Reactor() = default;
	virtual void dump(Writer& writer) const = 0;
	virtual void react(const EventPtr& event, uint64_t timestamp) = 0;
};

#endif