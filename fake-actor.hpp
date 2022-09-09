#ifndef FAKE_ACTOR_HPP
#define FAKE_ACTOR_HPP

#include "actor.hpp"

class FakeActor: public Actor {
public:
	void send(EventPtr event, uint32_t delay) override {}
};

#endif