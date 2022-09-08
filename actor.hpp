#ifndef ACTOR_HPP
#define ACTOR_HPP

#include "event.hpp"

class Reactor;

class Actor {
public:
	using EventPtr = Event::SharedPtr;
	using SharedPtr = std::shared_ptr<Actor>;
	
	virtual ~Actor() = default;
	// must be thread-safe
	virtual void send(EventPtr event, uint32_t delay = 0) = 0;
};

class ActorSelf: public Actor {
public:
	using ReactorPtr = std::unique_ptr<Reactor>;
	using SharedPtr = std::shared_ptr<ActorSelf>;
	
	// not thread-safe: reset actor in this thread-loop
	virtual void reset(ReactorPtr&& state = ReactorPtr()) = 0;
	// not thread-safe: spawn actor in this thread-loop
	virtual SharedPtr spawn() = 0;
};

#endif