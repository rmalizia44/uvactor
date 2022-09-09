#include "context-uv.hpp"
#include "common-events.hpp"
#include "log-reactor.hpp"

class EvtTest: public EventType<1> {
public:
	explicit EvtTest(int _e): e(_e) {}
	virtual void dump(Writer& writer) const override {}
	int e;
};

class EvtReset: public EventType<3> {
public:
	virtual void dump(Writer& writer) const override {}
};

class ReactorTest: public Reactor {
public:
	explicit ReactorTest(SelfPtr self, ActorPtr logger, int n): _self(self), _logger(logger), _n(n) {}
	void dump(Writer& writer) const override {}
	void react(const EventPtr& event, uint64_t timestamp) override {
		switch(event->type) {
			case EvtTest::TYPE:
				_logger->send(
					std::make_shared<EvtLog>("%i: %i (%llu)", _n, event->as<EvtTest>().e, (long long unsigned int)timestamp)
				);
				break;
			case EvtExit::TYPE:
				_logger->send(
					std::make_shared<EvtLog>("%i: exit (%llu)", _n, (long long unsigned int)timestamp)
				);
				_self->reset();
				break;
			case EvtReset::TYPE:
				_logger->send(
					std::make_shared<EvtLog>("%i: reset (%llu)", _n, (long long unsigned int)timestamp)
				);
				_self->reset(
					std::unique_ptr<Reactor>(
						new ReactorTest(_self, _logger, _n)
					)
				);
				break;
			default:
				_logger->send(
					std::make_shared<EvtLog>("%i: unknown<%u> (%llu)", _n, event->type, (long long unsigned int)timestamp)
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
			new LogReactor(logger)
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