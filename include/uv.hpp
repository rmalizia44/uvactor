#ifndef UV_HPP
#define UV_HPP

#include <uv.h>
#include <exception>

class ExceptionUV: public std::exception {
public:
	explicit ExceptionUV(int ec) noexcept: _ec(ec) {}
	const char* what() const noexcept override {
		return uv_strerror(_ec);
	}
private:
	const int _ec;
};

#define UV_INVOKE(_expr) {\
	int _ec = (_expr);\
	if(_ec < 0) {\
		throw ExceptionUV(_ec);\
	}\
}

#endif