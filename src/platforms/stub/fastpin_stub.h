/// @file platforms/stub/fastpin_stub.h
/// Stub pin implementation for testing and WebAssembly targets
///
/// Provides no-op implementations of Pin and FastPin for targets that don't
/// have hardware access (testing, WebAssembly/browser, simulation).

#pragma once

#include "fl/unused.h"
#include "fl/register.h"

// Include base class definitions (includes Selectable)
#include "fl/fastpin_base.h"

namespace fl {

/// Stub Pin class for no-op pin access
class Pin : public Selectable {

	void _init() {
	}

public:
	Pin(int pin) { FL_UNUSED(pin); }

	void setPin(int pin) { FL_UNUSED(pin); }

	typedef volatile RwReg * port_ptr_t;
	typedef RwReg port_t;

	inline void setOutput() { /* NOOP */ }
	inline void setInput() { /* NOOP */ }
	inline void setInputPullup() { /* NOOP */ }

	inline void hi() __attribute__ ((always_inline)) {}
	/// Set the pin state to `LOW`
	inline void lo() __attribute__ ((always_inline)) {}

	inline void strobe() __attribute__ ((always_inline)) {  }
	inline void toggle() __attribute__ ((always_inline)) { }

	inline void hi(FASTLED_REGISTER port_ptr_t port) __attribute__ ((always_inline)) { FL_UNUSED(port); }
	inline void lo(FASTLED_REGISTER port_ptr_t port) __attribute__ ((always_inline)) { FL_UNUSED(port); }
	inline void set(FASTLED_REGISTER port_t val) __attribute__ ((always_inline)) { FL_UNUSED(val); }

	inline void fastset(FASTLED_REGISTER port_ptr_t port, FASTLED_REGISTER port_t val) __attribute__ ((always_inline)) { FL_UNUSED(port); FL_UNUSED(val); }

	port_t hival() __attribute__ ((always_inline)) { return 0; }
	port_t loval() __attribute__ ((always_inline)) { return 0; }
	port_ptr_t  port() __attribute__ ((always_inline)) {
		static volatile RwReg port = 0;
		return &port;
	}
	port_t mask() __attribute__ ((always_inline)) { return 0xff; }

	virtual void select() override { hi(); }
	virtual void release() override { lo(); }
	virtual bool isSelected() override { return true; }
};

class OutputPin : public Pin {
public:
	OutputPin(int pin) : Pin(pin) { setOutput(); }
};

class InputPin : public Pin {
public:
	InputPin(int pin) : Pin(pin) { setInput(); }
};

/// FastPin template specialization for stub platform
/// Every pin is valid on the stub platform
template<fl::u8 PIN> class FastPin {
	constexpr static bool validpin() { return true; }

	static void _init() { }

public:
	typedef volatile RwReg * port_ptr_t;
	typedef RwReg port_t;

	/// Set pin to output mode
	inline static void setOutput() { }
	/// Set pin to input mode
	inline static void setInput() { }

	/// Set pin high
	inline static void hi() __attribute__ ((always_inline)) { }
	/// Set pin low
	inline static void lo() __attribute__ ((always_inline)) { }

	/// Strobe pin (high then low)
	inline static void strobe() __attribute__ ((always_inline)) { }

	/// Toggle pin state
	inline static void toggle() __attribute__ ((always_inline)) { }

	/// Set pin high with provided port register
	inline static void hi(FASTLED_REGISTER port_ptr_t port) __attribute__ ((always_inline)) { FL_UNUSED(port); }
	/// Set pin low with provided port register
	inline static void lo(FASTLED_REGISTER port_ptr_t port) __attribute__ ((always_inline)) { FL_UNUSED(port); }
	/// Set port to specified value
	inline static void set(FASTLED_REGISTER port_t val) __attribute__ ((always_inline)) { FL_UNUSED(val); }

	/// Fast set operation
	inline static void fastset(FASTLED_REGISTER port_ptr_t port, FASTLED_REGISTER port_t val) __attribute__ ((always_inline)) { FL_UNUSED(port); FL_UNUSED(val); }

	/// Get high value for pin
	static port_t hival() __attribute__ ((always_inline)) { return 0; }
	/// Get low value for pin
	static port_t loval() __attribute__ ((always_inline)) { return 0; }
	/// Get port register pointer
	static port_ptr_t  port() __attribute__ ((always_inline)) {
		static volatile RwReg port = 0;
		return &port;
	}
	/// Get pin mask
	static port_t mask() __attribute__ ((always_inline)) { return 0xff; }
};

} // namespace fl
