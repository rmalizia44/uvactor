#ifndef REACTOR_HPP
#define REACTOR_HPP

#include "event.hpp"
#include "actor.hpp"

class Reactor {
public:
	using UniquePtr = std::unique_ptr<Reactor>;
	using EventPtr = Event::SharedPtr;
	using ActorPtr = Actor::SharedPtr;
	using SelfPtr = ActorSelf::SharedPtr;
	
	template<typename T, typename... Ts>
	static UniquePtr make(Ts&&... ts) {
		return UniquePtr(new T(std::forward<Ts>(ts)...));
	}
	
	virtual ~Reactor() = default;
	virtual void dump(Writer& writer) const = 0;
	virtual void react(const EventPtr& event, uint64_t timestamp) = 0;
};

#endif