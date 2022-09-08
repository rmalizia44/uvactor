#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <list>
#include <vector>
#include <utility>
#include <mutex>
#include "event.hpp"

class Queue {
public:
	using EventPtr = Event::SharedPtr;
	using EventPair = std::pair<EventPtr, uint64_t>;
	using EventVector = std::vector<EventPair>;
	using EventList = std::list<EventPair>;
	// thread-safe
	void add_ready(EventPtr&& event, uint64_t timestamp) {
		std::lock_guard<std::mutex> lock(_mutex);
		_ready.emplace_back(std::move(event), timestamp);
	}
	// thread-safe
	bool add_waiting(EventPtr&& event, uint64_t timestamp) {
		std::lock_guard<std::mutex> lock(_mutex);
		if(_waiting.empty() || timestamp < _waiting.front().second) {
			_waiting.emplace_front(std::move(event), timestamp);
			return true;
		} else {
			auto itr = _waiting.begin();
			do {
				++itr;
			} while(itr != _waiting.end() && timestamp >= itr->second);
			_waiting.emplace(itr, std::move(event), timestamp);
			return false;
		}
	}
	// thread-safe
	void get_events(EventVector& out) {
		out.clear();
		/*lock context*/{
			std::lock_guard<std::mutex> lock(_mutex);
			std::swap(out, _ready);
		}
	}
	// thread-safe
	uint64_t update(uint64_t timestamp) {
		uint64_t next_timeout = 0;
		/*lock context*/{
			std::lock_guard<std::mutex> lock(_mutex);
			while(!_waiting.empty() && timestamp >= _waiting.front().second) {
				auto& pair = _waiting.front();
				_ready.emplace_back(std::move(pair.first), pair.second);
				_waiting.pop_front();
			}
			if(!_waiting.empty()) {
				next_timeout = _waiting.front().second;
			}
		}
		return next_timeout;
	}
private:
	std::mutex _mutex;
	EventVector _ready;
	EventList _waiting;
	
};

#endif