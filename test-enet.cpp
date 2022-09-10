#include "context-uv.hpp"
#include "enet-reactor-auto.hpp"
#include "enet-reactor-uv.hpp"
#include "log-reactor.hpp"

#include <ctime>
#include <cstring>

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
			Event::make<EvtLog>("[C] connected src=%u t=%llu", event.src, (long long unsigned int)timestamp)
		);
		_self->send(
			Event::make<EvtUpdate>()
		);
		_id = event.src;
	}
	void on_disconnect(const EvtDisconnected& event, uint64_t timestamp) {
		_logger->send(
			Event::make<EvtLog>("[C] disconnected src=%u t=%llu", event.src, (long long unsigned int)timestamp)
		);
	}
	void on_received(const EvtReceived& event, uint64_t timestamp) {
		const time_t* t = reinterpret_cast<const time_t*>(event.data.c_str());
		char* s = ctime(t);
		char* n = strchr(s, '\n');
		if(n != nullptr) {
			*n = '\0';
		}
		_logger->send(
			Event::make<EvtLog>("[C] received src=%u data=\"%s\" t=%llu", event.src, s, (long long unsigned int)timestamp)
		);
	}
	void on_update(const EvtUpdate& event, uint64_t timestamp) {
		_logger->send(
			Event::make<EvtLog>("[C] update t=%llu", (long long unsigned int)timestamp)
		);
		time_t t = time(nullptr);
		std::string data(reinterpret_cast<const char*>(&t), sizeof(t));
		_client->send(
			Event::make<EvtSend>(_id, std::move(data), true)
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
			Event::make<EvtLog>("[S] connected src=%u t=%llu", event.src, (long long unsigned int)timestamp)
		);
	}
	void on_disconnect(const EvtDisconnected& event, uint64_t timestamp) {
		_logger->send(
			Event::make<EvtLog>("[S] disconnected src=%u t=%llu", event.src, (long long unsigned int)timestamp)
		);
	}
	void on_received(const EvtReceived& event, uint64_t timestamp) {
		_logger->send(
			Event::make<EvtLog>("[S] received src=%u size=%u t=%llu", event.src, event.data.size(), (long long unsigned int)timestamp)
		);
		_server->send(
			Event::make<EvtSend>(event.src, std::string(event.data), true)
		);
	}
	
	SelfPtr _self;
	ActorPtr _server;
	ActorPtr _logger;
};

enum {
	NUM_THREADS = 8,
	NUM_CLIENTS = 100,
	APP_PORT = 8080,
};

int main() {
	printf("initializing\n");
	
	std::vector<ContextUV> contexts(NUM_THREADS);
	unsigned idx = 0;
	
	ActorSelf::SharedPtr logger = contexts[(idx++) % NUM_THREADS].spawn();
	ActorUV::SharedPtr enet_server = contexts[(idx++) % NUM_THREADS].spawn();
	ActorSelf::SharedPtr echo_server = contexts[(idx++) % NUM_THREADS].spawn();
	std::vector<ActorSelf::SharedPtr> enet_clients;
	std::vector<ActorSelf::SharedPtr> time_clients;
	
	logger->reset(
		Reactor::make<LogReactor>(logger)
	);
	enet_server->reset(
		Reactor::make<ENetReactorUV>(enet_server, echo_server)
	);
	echo_server->reset(
		Reactor::make<EchoReactor>(echo_server, enet_server, logger)
	);
	
	for(unsigned i = 0; i < NUM_CLIENTS; ++i) {
		ActorUV::SharedPtr enet_client = contexts[(idx++) % NUM_THREADS].spawn();
		ActorSelf::SharedPtr time_client = contexts[(idx++) % NUM_THREADS].spawn();
		enet_client->reset(
			Reactor::make<ENetReactorUV>(enet_client, time_client)
		);
		time_client->reset(
			Reactor::make<TimeReactor>(time_client, enet_client, logger)
		);
		enet_clients.push_back(enet_client);
		time_clients.push_back(time_client);
	}
	
	for(auto& enet_client : enet_clients) {
		enet_client->send(
			Event::make<EvtConnect>("localhost", APP_PORT)
		);
	}
	
	enet_server->send(
		Event::make<EvtListen>("localhost", APP_PORT, NUM_CLIENTS)
	);
	
	for(auto& ctx : contexts) {
		ctx.exec();
	}
	for(auto& ctx : contexts) {
		ctx.wait();
	}
	
	return 0;
}