#ifndef EVENT_HPP
#define EVENT_HPP

#include <cstdint>
#include <memory>
#include "writer.hpp"

class Event {
public:
	using SharedPtr = std::shared_ptr<const Event>;
	
	explicit Event(uint32_t _type) noexcept: type(_type) {}
	virtual ~Event() = default;
	
	virtual void dump(Writer& writer) const = 0;
	
	template<typename T>
	const T& as() const noexcept {
		return *static_cast<const T*>(this);
	}
	
	const uint32_t type;
};

template<uint32_t T>
class EventType: public Event {
public:
	enum : uint32_t { TYPE = T };
	EventType() noexcept: Event(TYPE) {}
};

#endif