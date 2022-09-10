#ifndef ENET_REACTOR_HPP
#define ENET_REACTOR_HPP

#include <enet/enet.h>
#include <exception>
#include <utility>
#include <cstdio>
#include "reactor.hpp"
#include "common-events.hpp"
#include "network-events.hpp"

class ENetException: public std::exception {
public:
	template<typename... Ts>
	explicit ENetException(Ts&&... ts) noexcept {
		snprintf(_buf, sizeof(_buf), std::forward<Ts>(ts)...);
	}
	const char* what() const noexcept override {
		return _buf;
	}
private:
	char _buf[256];
};

class ENetReactor: public Reactor {
	class Library {
	public:
		static Library& require() {
			static Library singleton;
			return singleton;
		}
	private:
		Library() {
			if(enet_initialize() != 0) {
				throw ENetException("can't initialize enet");
			}
			atexit(enet_deinitialize);
		}
	};
	enum {
		INCOMING_BANDWIDTH = 0,
		OUTGOING_BANDWIDTH = 0,
	};
	enum { // same as godot
		CHANNEL_CONFIG,
		CHANNEL_RELIABLE,
		CHANNEL_UNRELIABLE,
		CHANNEL_COUNT,
	};
	struct ENetHostDestructor {
		void operator()(ENetHost* host) const noexcept { enet_host_destroy(host); }
	};
	struct ENetPacketDestructor {
		void operator()(ENetPacket* packet) const noexcept { enet_packet_destroy(packet); }
	};
	using HostPtr = std::unique_ptr<ENetHost, ENetHostDestructor>;
	using PacketPtr = std::unique_ptr<ENetPacket, ENetPacketDestructor>;
public:
	explicit ENetReactor(SelfPtr self, ActorPtr observer): _self(self), _observer(observer) {
		Library::require();
	}
	void dump(Writer& writer) const override {}
	void react(const EventPtr& event, uint64_t timestamp) override {
		switch(event->type) {
			case EvtListen::TYPE:
				on_listen(event->as<EvtListen>(), timestamp);
				break;
			case EvtConnect::TYPE:
				on_connect(event->as<EvtConnect>(), timestamp);
				break;
			case EvtSend::TYPE:
				on_send(event->as<EvtSend>(), timestamp);
				break;
			case EvtKick::TYPE:
				on_kick(event->as<EvtKick>(), timestamp);
				break;
			case EvtUpdate::TYPE:
				on_update(event->as<EvtUpdate>(), timestamp);
				break;
			case EvtExit::TYPE:
				_self->reset();
				break;
			default:
				break;
		}
	}
protected:
	void on_listen(const EvtListen& event, uint32_t timestamp) {
		if(_enet_host) {
			throw ENetException("can't listen: enet host already initialized");
		}
		ENetAddress addr = {
			.host = ENET_HOST_ANY,
			.port = static_cast<enet_uint16>(event.port)
		};
		if(!event.host.empty()) {
			if(enet_address_set_host(&addr, event.host.c_str()) != 0) {
				throw ENetException("can't listen: failed to set host");
			}
		}
		_enet_host.reset(
			enet_host_create(&addr, event.max_peers, CHANNEL_COUNT, INCOMING_BANDWIDTH, OUTGOING_BANDWIDTH)
		);
		if(!_enet_host) {
			throw ENetException("can't listen: can't create enet host");
		}
	}
	void on_connect(const EvtConnect& event, uint32_t timestamp) {
		if(_enet_host) {
			throw ENetException("can't connect: enet host already initialized");
		}
		_enet_host.reset(
			enet_host_create(nullptr, 1, CHANNEL_COUNT, INCOMING_BANDWIDTH, OUTGOING_BANDWIDTH)
		);
		if(!_enet_host) {
			throw ENetException("can't connect: can't create enet host");
		}
		ENetAddress addr = {
			.host = ENET_HOST_ANY,
			.port = static_cast<enet_uint16>(event.port)
		};
		if(enet_address_set_host(&addr, event.host.c_str()) != 0) {
			throw ENetException("can't connect: failed to set host");
		}
		if(enet_host_connect(_enet_host.get(), &addr, CHANNEL_COUNT, 0) == nullptr) {
			throw ENetException("can't connect: failed to connect peer");
		}
	}
	void on_send(const EvtSend& event, uint32_t timestamp) {
		if(!_enet_host) {
			throw ENetException("can't send: enet host not initialized");
		}
		if(event.dst >= _enet_host->peerCount) {
			throw ENetException("can't send: invalid id: %u", event.dst);
		}
		PacketPtr packet;
		packet.reset(enet_packet_create(event.buf.c_str(), event.buf.size(), event.reliable ? ENET_PACKET_FLAG_RELIABLE : ENET_PACKET_FLAG_UNSEQUENCED));
		if(!packet) {
			throw ENetException("can't send: can't create packet");
		}
		if(enet_peer_send(&_enet_host->peers[event.dst], event.reliable ? CHANNEL_RELIABLE : CHANNEL_UNRELIABLE, packet.get()) != 0) {
			throw ENetException("can't send: can't send packet to peer");
		}
		packet.release();
	}
	void on_kick(const EvtKick& event, uint32_t timestamp) {
		if(!_enet_host) {
			throw ENetException("can't kick: enet host not initialized");
		}
		if(event.dst >= _enet_host->peerCount) {
			throw ENetException("can't kick: invalid id: %u", event.dst);
		}
		enet_peer_disconnect_later(&_enet_host->peers[event.dst], 0);
	}
	void on_update(const EvtUpdate& event, uint32_t timestamp) {
		if(!_enet_host) {
			throw ENetException("can't poll: enet host not initialized");
		}
		ENetEvent enet_event;
		int ec;
		while((ec = enet_host_service(_enet_host.get(), &enet_event, 0)) > 0) {
			const uint16_t id = enet_event.peer - _enet_host->peers;
			PacketPtr packet(enet_event.packet);
			std::string data;
			switch(enet_event.type) {
				case ENET_EVENT_TYPE_CONNECT:
					_observer->send(
						std::make_shared<EvtConnected>(id)
					);
					break;
				case ENET_EVENT_TYPE_DISCONNECT:
					_observer->send(
						std::make_shared<EvtDisconnected>(id)
					);
					break;
				case ENET_EVENT_TYPE_RECEIVE:
					if(!packet) {
						throw ENetException("can't poll: enet invalid packet");
					}
					data = std::string(reinterpret_cast<const char*>(packet->data), packet->dataLength);
					_observer->send(
						std::make_shared<EvtReceived>(id, std::move(data))
					);
					break;
				default:
					throw ENetException("can't poll: enet unknown event");
			}
		}
		if(ec < 0) {
			throw ENetException("can't poll: enet service error");
		}
	}
	
	SelfPtr _self;
	ActorPtr _observer;
	HostPtr _enet_host;
};

#endif