#include "context-uv.hpp"
#include "sqlite-reactor.hpp"
#include "log-reactor.hpp"
#include "fake-actor.hpp"

enum {
	NUM_THREADS = 8,
};

int main() {
	printf("initializing\n");
	
	std::vector<ContextUV> contexts(NUM_THREADS);
	unsigned idx = 0;
	
	ActorSelf::SharedPtr logger = contexts[(idx++) % NUM_THREADS].spawn();
	ActorSelf::SharedPtr database = contexts[(idx++) % NUM_THREADS].spawn();
	Actor::SharedPtr observer = std::make_shared<FakeActor>();
	
	logger->reset(
		Reactor::make<LogReactor>(logger)
	);
	database->reset(
		Reactor::make<SqliteReactor>(database, observer, logger)
	);
	
	database->send(
		Event::make<EvtPut>("foo", "bar")
	);
	database->send(
		Event::make<EvtGet>("foo")
	);
	database->send(
		Event::make<EvtGet>("bar")
	);
	database->send(
		Event::make<EvtExit>(), 100
	);
	logger->send(
		Event::make<EvtExit>(), 100
	);
	
	for(auto& ctx : contexts) {
		ctx.exec();
	}
	for(auto& ctx : contexts) {
		ctx.wait();
	}
	
	return 0;
}