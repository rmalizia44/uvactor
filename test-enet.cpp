#include "context-uv.hpp"
#include "enet-reactor.hpp"
#include "log-reactor.hpp"

class TimeReactor: public Reactor {
	enum {
		UPDATE_TIME = 1000,
	};
public:
	explicit TimeReactor(SelfPtr self, ActorPtr client, ActorPtr logger): _self(self), _client(client), _logger(logger) {}
	void dump(Writer& writer) const override {}
	void react(const EventPtr& event, uint64_t timestamp) override {
		switch(event->type) {
			case EvtConnected::TYPE:
				on_connect(event->as<EvtConnected>(), timestamp);
				break;
			case EvtDisconnected::TYPE:
				on_disconnect(event->as<EvtDisconnected>(), timestamp);
				break;
			case EvtReceived::TYPE:
				on_received(event->as<EvtReceived>(), timestamp);
				break;
			case EvtUpdate::TYPE:
				on_update(event->as<EvtUpdate>(), timestamp);
				_self->send(event, UPDATE_TIME);
				break;
			default:
				break;
		}
	}
private:
	void on_connect(const EvtConnected& event, uint64_t timestamp) {
		_logger->send(
			std::make_shared<EvtLog>("[C] connected src=%u t=%llu", event.src, (long long unsigned int)timestamp)
		);
		_self->send(
			std::make_shared<EvtUpdate>()
		);
		_id = event.src;
	}
	void on_disconnect(const EvtDisconnected& event, uint64_t timestamp) {
		_logger->send(
			std::make_shared<EvtLog>("[C] disconnected src=%u t=%llu", event.src, (long long unsigned int)timestamp)
		);
	}
	void on_received(const EvtReceived& event, uint64_t timestamp) {
		_logger->send(
			std::make_shared<EvtLog>("[C] received src=%u data=%s t=%llu", event.src, event.data.c_str(), (long long unsigned int)timestamp)
		);
	}
	void on_update(const EvtUpdate& event, uint64_t timestamp) {
		_logger->send(
			std::make_shared<EvtLog>("[C] update t=%llu", (long long unsigned int)timestamp)
		);
		std::string data("hello");
		_client->send(
			std::make_shared<EvtSend>(_id, std::move(data), true)
		);
	}
	
	SelfPtr _self;
	ActorPtr _client;
	ActorPtr _logger;
	uint32_t _id;
};

class EchoReactor: public Reactor {
public:
	explicit EchoReactor(SelfPtr self, ActorPtr server, ActorPtr logger): _self(self), _server(server), _logger(logger) {}
	void dump(Writer& writer) const override {}
	void react(const EventPtr& event, uint64_t timestamp) override {
		switch(event->type) {
			case EvtConnected::TYPE:
				on_connect(event->as<EvtConnected>(), timestamp);
				break;
			case EvtDisconnected::TYPE:
				on_disconnect(event->as<EvtDisconnected>(), timestamp);
				break;
			case EvtReceived::TYPE:
				on_received(event->as<EvtReceived>(), timestamp);
				break;
			default:
				break;
		}
	}
private:
	void on_connect(const EvtConnected& event, uint64_t timestamp) {
		_logger->send(
			std::make_shared<EvtLog>("[S] connected src=%u t=%llu", event.src, (long long unsigned int)timestamp)
		);
	}
	void on_disconnect(const EvtDisconnected& event, uint64_t timestamp) {
		_logger->send(
			std::make_shared<EvtLog>("[S] disconnected src=%u t=%llu", event.src, (long long unsigned int)timestamp)
		);
	}
	void on_received(const EvtReceived& event, uint64_t timestamp) {
		_logger->send(
			std::make_shared<EvtLog>("[S] received src=%u data=%s t=%llu", event.src, event.data.c_str(), (long long unsigned int)timestamp)
		);
		_server->send(
			std::make_shared<EvtSend>(event.src, std::string(event.data), true)
		);
	}
	
	SelfPtr _self;
	ActorPtr _server;
	ActorPtr _logger;
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
	
	ActorSelf::SharedPtr logger = contexts[0 % NUM_THREADS].spawn();
	ActorSelf::SharedPtr enet_client = contexts[1 % NUM_THREADS].spawn();
	ActorSelf::SharedPtr enet_server = contexts[2 % NUM_THREADS].spawn();
	ActorSelf::SharedPtr time_client = contexts[3 % NUM_THREADS].spawn();
	ActorSelf::SharedPtr echo_server = contexts[4 % NUM_THREADS].spawn();
	
	logger->reset(
		Reactor::make<LogReactor>(logger)
	);
	enet_client->reset(
		Reactor::make<ENetReactor>(enet_client, time_client)
	);
	enet_server->reset(
		Reactor::make<ENetReactor>(enet_server, echo_server)
	);
	time_client->reset(
		Reactor::make<TimeReactor>(time_client, enet_client, logger)
	);
	echo_server->reset(
		Reactor::make<EchoReactor>(echo_server, enet_server, logger)
	);
	
	enet_client->send(
		std::make_shared<EvtConnect>("localhost", 8080)
	);
	enet_server->send(
		std::make_shared<EvtListen>("localhost", 8080, 100)
	);
	
	for(auto& ctx : contexts) {
		ctx.exec();
	}
	for(auto& ctx : contexts) {
		ctx.wait();
	}
	
	printf("logger t=%llu\n", (long long unsigned int)logger->reactive_time());
	printf("enet_client t=%llu\n", (long long unsigned int)enet_client->reactive_time());
	printf("enet_server t=%llu\n", (long long unsigned int)enet_server->reactive_time());
	printf("time_client t=%llu\n", (long long unsigned int)time_client->reactive_time());
	printf("echo_server t=%llu\n", (long long unsigned int)echo_server->reactive_time());
	
	return 0;
}