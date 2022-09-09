#ifndef ACTOR_HPP
#define ACTOR_HPP

#include "event.hpp"

class Reactor;

// must be thread-safe
class Actor {
public:
	using EventPtr = Event::SharedPtr;
	using SharedPtr = std::shared_ptr<Actor>;
	
	virtual ~Actor() = default;
	virtual void send(EventPtr event, uint32_t delay = 0) = 0;
};

// not thread-safe: use only in this thread-loop
class ActorSelf: public Actor {
public:
	using ReactorPtr = std::unique_ptr<Reactor>;
	using SharedPtr = std::shared_ptr<ActorSelf>;
	
	virtual ~ActorSelf() = default;
	virtual void reset(ReactorPtr&& state = ReactorPtr()) = 0;
	virtual SharedPtr spawn() = 0;
	virtual uint64_t reactive_time() const noexcept = 0;
};

#endif