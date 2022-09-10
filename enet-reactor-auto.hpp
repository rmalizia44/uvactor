#ifndef ENET_REACTOR_AUTO_HPP
#define ENET_REACTOR_AUTO_HPP

#include "enet-reactor.hpp"

class ENetReactorAuto: public ENetReactor {
	enum {
		UPDATE_TIME = 50,
	};
public:
	using ENetReactor::ENetReactor;
	void react(const EventPtr& event, uint64_t timestamp) override {
		ENetReactor::react(event, timestamp);
		switch(event->type) {
			case EvtListen::TYPE:
			case EvtConnect::TYPE:
				_self->send(
					Event::make<EvtUpdate>()
				);
				break;
			case EvtUpdate::TYPE:
				_self->send(event, UPDATE_TIME);
				break;
			default:
				break;
		}
	}
private:
	
};

#endif