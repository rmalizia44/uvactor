#ifndef ACTOR_UV_HPP
#define ACTOR_UV_HPP

#include "uv.hpp"
#include "actor.hpp"
#include "queue.hpp"
#include "stateful.hpp"

#define LOG_DEBUG(...) /*{\
	printf("[%s:%i] ", __FILE__, __LINE__);\
	printf(__VA_ARGS__);\
	printf("\n");\
}*/

class ActorUV: public ActorSelf, public std::enable_shared_from_this<ActorUV> {
	using ReactorPtr = ActorSelf::ReactorPtr;
	using EventPtr = Event::SharedPtr;
	using EventPair = std::pair<EventPtr, uint64_t>;
	using EventVector = std::vector<EventPair>;
public:
	using SharedPtr = std::shared_ptr<ActorUV>;
	explicit ActorUV(std::shared_ptr<uv_loop_t> loop): _loop(std::move(loop)) {
		LOG_DEBUG("ActorUV::ActorUV() [%p]", this);
		_ini_time = uv_now(_loop.get());
	}
	~ActorUV() noexcept {
		LOG_DEBUG("ActorUV::~ActorUV() [%p]", this);
	}
	std::shared_ptr<uv_loop_t> loop() {
		return _loop;
	}
	uint64_t timestamp() const noexcept {
		return static_cast<uint64_t>(uv_now(_loop.get()) - _ini_time);
	}
	uint64_t reactive_time() const noexcept override {
		return static_cast<uint64_t>(_react_time_total / 1000000);
	}
	// thread-safe
	void send(EventPtr event, uint32_t delay = 0) override {
		LOG_DEBUG("ActorUV::send() type=%u [%p]", event->type, this);
		uint64_t t = timestamp() + delay;
		if(delay > 0) {
			LOG_DEBUG("\tadd waiting timestamp=%llu", (long long unsigned int)t);
			if(_queue.add_waiting(std::move(event), t)) {
				notify();
			}
		} else {
			LOG_DEBUG("\tadd ready timestamp=%llu", (long long unsigned int)t);
			if(_queue.add_ready(std::move(event), t)) {
				notify();
			}
		}
	}
	void reset(ReactorPtr&& state) override {
		LOG_DEBUG("ActorUV::reset() [%p]", this);
		bool was_running = _stateful.is_running();
		_stateful.set(std::move(state));
		if(!was_running && _stateful.is_running()) {
			LOG_DEBUG("\tstarting");
			on_start();
		}
		if(was_running && !_stateful.is_running()) {
			LOG_DEBUG("\tstopping");
			on_stop();
		}
	}
	ActorSelf::SharedPtr spawn() override {
		LOG_DEBUG("ActorUV::spawn() [%p]", this);
		return std::make_shared<ActorUV>(_loop);
	}
private:
	static void async_callback(uv_async_t* handle) {
		LOG_DEBUG("ActorUV::async_callback() handle: %p", handle);
		if(handle->data != nullptr) {
			(*reinterpret_cast<SharedPtr*>(handle->data))->on_trigger();
		}
	}
	static void timer_callback(uv_timer_t* handle) {
		LOG_DEBUG("ActorUV::timer_callback() handle: %p", handle);
		if(handle->data != nullptr) {
			(*reinterpret_cast<SharedPtr*>(handle->data))->on_trigger();
		}
	}
	static void close_callback(uv_handle_t* handle) {
		LOG_DEBUG("ActorUV::close_callback() handle: %p", handle);
		if(handle->data != nullptr) {
			SharedPtr* shared = reinterpret_cast<SharedPtr*>(handle->data);
			handle->data = nullptr;
			delete shared;
		}
	}
	void on_start() {
		LOG_DEBUG("ActorUV::on_start() [%p]", this);
		UV_INVOKE(uv_async_init(_loop.get(), &_async, async_callback));
		UV_INVOKE(uv_timer_init(_loop.get(), &_timer));
		_async.data = new SharedPtr(this->shared_from_this());
		_timer.data = new SharedPtr(this->shared_from_this());
		_queue.set_open(true);
	}
	void on_stop() {
		LOG_DEBUG("ActorUV::on_stop() [%p]", this);
		_queue.set_open(false);
		uv_close(reinterpret_cast<uv_handle_t*>(&_async), close_callback);
		uv_close(reinterpret_cast<uv_handle_t*>(&_timer), close_callback);
	}
	// thread-safe
	void notify() {
		LOG_DEBUG("ActorUV::notify() [%p]", this);
		UV_INVOKE(uv_async_send(&_async));
	}
	void on_trigger() {
		LOG_DEBUG("ActorUV::on_trigger() [%p]", this);
		const uint32_t cur_timestamp = timestamp();
		const uint32_t next_timestamp = _queue.update(cur_timestamp);
		if(next_timestamp > cur_timestamp) {
			const uint32_t delay = next_timestamp - cur_timestamp;
			LOG_DEBUG("\ttimer start delay=%u", delay);
			UV_INVOKE(uv_timer_start(&_timer, timer_callback, delay, 0));
		} else {
			LOG_DEBUG("\ttimer stop");
			UV_INVOKE(uv_timer_stop(&_timer));
		}
		trigger_profile();
	}
	void trigger_profile() {
		LOG_DEBUG("ActorUV::trigger_profile() [%p]", this);
		_queue.get_events(_reacting);
		const uint64_t react_time_start = uv_hrtime();
		_stateful.trigger(_reacting);
		const uint64_t react_time_final = uv_hrtime();
		if(react_time_final >= react_time_start) {
			_react_time_total += react_time_final - react_time_start;
		} else {
			_react_time_total += (UINT64_MAX - react_time_start) + react_time_final;
		}
	}
	
	Queue _queue;
	Stateful _stateful;
	EventVector _reacting;
	std::shared_ptr<uv_loop_t> _loop;
	uv_async_t _async;
	uv_timer_t _timer;
	uint64_t _react_time_total = 0;
	uint64_t _ini_time = 0;
};

#endif