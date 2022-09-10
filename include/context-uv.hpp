#ifndef CONTEXT_UV_HPP
#define CONTEXT_UV_HPP

#include "actor-uv.hpp"

class ContextUV {
public:
	ContextUV(): _loop(std::make_shared<uv_loop_t>()) {
		UV_INVOKE(uv_loop_init(_loop.get()));
	}
	~ContextUV() {
		// TODO: assert thread terminanted
	}
	ActorUV::SharedPtr spawn() {
		return std::make_shared<ActorUV>(_loop);
	}
	std::shared_ptr<uv_loop_t> loop() {
		return _loop;
	}
	void exec() {
		UV_INVOKE(uv_thread_create(&_thread, thread_callback, &_loop));
	}
	void wait() {
		UV_INVOKE(uv_thread_join(&_thread));
	}
private:
	static void thread_callback(void* arg) {
		std::shared_ptr<uv_loop_t> loop(*reinterpret_cast<const std::shared_ptr<uv_loop_t>*>(arg));
		UV_INVOKE(uv_run(loop.get(), UV_RUN_DEFAULT));
		UV_INVOKE(uv_loop_close(loop.get()));
	}
	uv_thread_t _thread;
	std::shared_ptr<uv_loop_t> _loop;
};

#endif