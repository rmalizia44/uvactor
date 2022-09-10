#ifndef DATABASE_EVENTS_HPP
#define DATABASE_EVENTS_HPP

#include <string>
#include "event.hpp"

class EvtPut: public EventType<0x15040EFE> {
public:
	explicit EvtPut(std::string&& _key, std::string&& _value): key(std::move(_key)), value(std::move(_value)) {}
	virtual void dump(Writer& writer) const override {}
	std::string key;
	std::string value;
};

class EvtGet: public EventType<0x46AFA95A> {
public:
	explicit EvtGet(std::string&& _key): key(std::move(_key)) {}
	virtual void dump(Writer& writer) const override {}
	std::string key;
};

class EvtResult: public EventType<0xA5105B4F> {
public:
	explicit EvtResult(std::string&& _key, std::string&& _value): key(std::move(_key)), value(std::move(_value)) {}
	virtual void dump(Writer& writer) const override {}
	std::string key;
	std::string value;
};

#endif