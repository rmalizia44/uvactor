#ifndef STATEFUL_HPP
#define STATEFUL_HPP

#include <vector>
#include <utility>
#include "event.hpp"
#include "reactor.hpp"

class Stateful {
	using EventPtr = Event::SharedPtr;
	using EventPair = std::pair<EventPtr, uint64_t>;
	using EventVector = std::vector<EventPair>;
	using EventItr = EventVector::const_iterator;
	using ReactorPtr = std::unique_ptr<Reactor>;
public:
	bool is_running() const noexcept {
		return !!_state;
	}
	bool trigger(const EventVector& events) {
		//
		EventItr itr = events.cbegin();
		EventItr end = events.cend();
		while(_state && itr != events.cend()) {
			process(itr, end);
		}
		return !!_state;
	}
	void set(ReactorPtr&& state) {
		_state = std::move(state);
	}
private:
	void reaction(EventItr& itr, const EventItr end) {
		Reactor* cur = _state.get();
		while(_state.get() == cur && itr != end) {
			_state->react(itr->first, itr->second); // can throw
			++itr;
		}
	}
	void post_factum(const EventItr ini, const EventItr end) {
		for(EventItr itr = ini; itr <= end; ++itr) {
			// TODO
		}
	}
	void process(EventItr& itr, const EventItr end) {
		const EventItr ini = itr;
		try {
			reaction(itr, end); // can throw
		} catch(...) {
			post_factum(ini, itr);
			throw;
		}
	}
	ReactorPtr _state;
	//EventVector _reacting; 
	// TODO: ? serialized reactor
};

#endif