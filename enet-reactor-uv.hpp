#ifndef ENET_REACTOR_UV_HPP
#define ENET_REACTOR_UV_HPP

#include "actor-uv.hpp"
#include "enet-reactor.hpp"

class ENetReactorUV: public ENetReactor {
	static void poll_callback(uv_poll_t* handle, int status, int events) {
		UV_INVOKE(status);
		if(handle->data != nullptr) {
			reinterpret_cast<ENetReactorUV*>(handle->data)->update();
		}
	}
	static void close_callback(uv_handle_t* handle) {
		delete reinterpret_cast<uv_poll_t*>(handle);
	}
public:
	explicit ENetReactorUV(ActorUV::SharedPtr self, ActorPtr observer): ENetReactor(self, observer), _loop(self->loop()) {
	}
	~ENetReactorUV() noexcept {
		poll_close();
	}
	void react(const EventPtr& event, uint64_t timestamp) override {
		ENetReactor::react(event, timestamp);
		switch(event->type) {
			case EvtListen::TYPE:
				poll_init();
				poll_restart(false);
				break;
			case EvtConnect::TYPE:
				poll_init();
				poll_restart(true);
				break;
			case EvtSend::TYPE:
			case EvtKick::TYPE:
				poll_restart(true);
				break;
			case EvtUpdate::TYPE:
				poll_restart(false);
				break;
			case EvtExit::TYPE:
				poll_close();
				break;
			default:
				break;
		}
	}
private:
	void poll_init() {
		if(_poll == nullptr) {
			_poll = new uv_poll_t();
			UV_INVOKE(uv_poll_init_socket(_loop.get(), _poll, _enet_host->socket));
			_poll->data = this;
		}
	}
	void poll_close() noexcept {
		if(_poll != nullptr) {
			_poll->data = nullptr;
			uv_close(reinterpret_cast<uv_handle_t*>(_poll), close_callback);
		}
		_poll = nullptr;
	}
	void poll_restart(bool write) {
		int events = UV_READABLE;
		if(write) {
			events |= UV_WRITABLE;
		}
		UV_INVOKE(uv_poll_start(_poll, events, poll_callback));
	}

	std::shared_ptr<uv_loop_t> _loop;
	uv_poll_t* _poll = nullptr;
};

#endif