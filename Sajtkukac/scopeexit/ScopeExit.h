// This code is in the public domain.
// See the LICENSE file for details.
//
// Simple implementation of Andrei Alexandrescu's scope guard,
// from one of his C++ talks.

#pragma once

#define CONCAT_IMPL(x, y) x ## y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

#ifdef __COUNTER__
#define ANONYMOUS_VAR(id) \
	CONCAT(id, __COUNTER__)
#else
#define ANONYMOUS_VAR(id) \
	CONCAT(id, __LINE__)
#endif

#define SCOPE_EXIT \
	auto ANONYMOUS_VAR(scope_exit_) \
	= ::scope_exit::ScopeGuardOnExit() + [&]() noexcept

namespace scope_exit {
	enum class ScopeGuardOnExit {};

	template<typename Fn>
	class ScopeGuard
	{
		Fn func_;
	public:
		explicit ScopeGuard(const Fn& fn)
			: func_(fn) {}
		explicit ScopeGuard(Fn&& fn)
			: func_(std::move(fn)) {}
		~ScopeGuard() noexcept
		{
			func_();
		}
	};

	template<typename Fn>
	ScopeGuard<Fn>
	operator+(ScopeGuardOnExit, Fn&& fn) {
		return ScopeGuard<Fn>(std::forward<Fn>(fn));
	}
}
