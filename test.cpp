#include "context-uv.hpp"
#include <cstdio>

class EvtText: public EventType<0> {
public:
	template<typename... Ts>
	explicit EvtText(Ts&&... ts) {
		snprintf(text, sizeof(text), std::forward<Ts>(ts)...);
	}
	virtual void dump(Writer& writer) const override {}
	char text[256];
};

class EvtTest: public EventType<1> {
public:
	explicit EvtTest(int _e): e(_e) {}
	virtual void dump(Writer& writer) const override {}
	int e;
};

class EvtExit: public EventType<2> {
public:
	virtual void dump(Writer& writer) const override {}
};

class EvtReset: public EventType<3> {
public:
	virtual void dump(Writer& writer) const override {}
};

class ReactorLogger: public Reactor {
public:
	explicit ReactorLogger(SelfPtr self): _self(self) {
		setbuf(stdout, nullptr);
	}
	void dump(Writer& writer) const override {}
	void react(const EventPtr& event, uint64_t timestamp) override {
		switch(event->type) {
			case EvtText::TYPE:
				printf("[L] %s (%llu)\n", event->as<EvtText>().text, (long long unsigned int)timestamp);
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

class ReactorTest: public Reactor {
public:
	explicit ReactorTest(SelfPtr self, ActorPtr logger, int n): _self(self), _logger(logger), _n(n) {}
	void dump(Writer& writer) const override {}
	void react(const EventPtr& event, uint64_t timestamp) override {
		switch(event->type) {
			case EvtTest::TYPE:
				_logger->send(
					std::make_shared<EvtText>("%i: %i (%llu)", _n, event->as<EvtTest>().e, (long long unsigned int)timestamp)
				);
				break;
			case EvtExit::TYPE:
				_logger->send(
					std::make_shared<EvtText>("%i: exit (%llu)", _n, (long long unsigned int)timestamp)
				);
				_self->reset();
				break;
			case EvtReset::TYPE:
				_logger->send(
					std::make_shared<EvtText>("%i: reset (%llu)", _n, (long long unsigned int)timestamp)
				);
				_self->reset(
					std::unique_ptr<Reactor>(
						new ReactorTest(_self, _logger, _n)
					)
				);
				break;
			default:
				_logger->send(
					std::make_shared<EvtText>("%i: unknown<%u> (%llu)", _n, event->type, (long long unsigned int)timestamp)
				);
				break;
		}
	}
private:
	SelfPtr _self;
	ActorPtr _logger;
	int _n;
};

enum {
	NUM_THREADS = 4,
	NUM_ACTORS = 7,
	NUM_EVENTS = 10,
	TIME_STEP = 20,
};

int main() {
	printf("initializing\n");
	
	std::vector<ContextUV> contexts(NUM_THREADS);
	std::vector<ActorSelf::SharedPtr> actors;
	ActorSelf::SharedPtr logger;
	
	logger = contexts[0].spawn();
	logger->reset(
		std::unique_ptr<Reactor>(
			new ReactorLogger(logger)
		)
	);
	logger->send(
		std::make_shared<EvtExit>(),
		(NUM_EVENTS + 1) * TIME_STEP
	);
	
	for(unsigned i = 0; i < NUM_ACTORS; ++i) {
		ActorSelf::SharedPtr actor;
		if(i < NUM_THREADS) {
			actor = contexts[i].spawn();
		} else {
			actor = actors[i - NUM_THREADS]->spawn();
		}
		actor->reset(
			std::unique_ptr<Reactor>(
				new ReactorTest(actor, logger, i)
			)
		);
		actors.push_back(actor);
	}
	
	for(unsigned i = 0; i < NUM_ACTORS; ++i) {
		for(unsigned e = 0; e < NUM_EVENTS; ++e) {
			actors[i]->send(
				std::make_shared<EvtTest>(e),
				e * TIME_STEP
			);
			if(i == e) {
				actors[i]->send(
					std::make_shared<EvtReset>(),
					e * TIME_STEP
				);
			}
		}
		actors[i]->send(
			std::make_shared<EvtExit>(),
			(NUM_EVENTS) * TIME_STEP
		);
	}
	
	for(auto& ctx : contexts) {
		ctx.exec();
	}
	for(auto& ctx : contexts) {
		ctx.wait();
	}
	
	printf("logger t=%llu\n", (long long unsigned int)logger->reactive_time());
	for(unsigned i = 0; i < NUM_ACTORS; ++i) {
		printf("actor[%u] t=%llu\n", i, (long long unsigned int)actors[i]->reactive_time());
	}
	
	return 0;
}