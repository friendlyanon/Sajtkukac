/* TERMS OF USE - EASING EQUATIONS
 *
 * Open source under the BSD License.
 *
 * Copyright © 2001 Robert Penner
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 *
 * Neither the name of the author nor the names of contributors may be used to endorse
 * or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "stdafx.h"
#include "Easing.h"

 // t - currentTime: timestamp at the time of invocation, same unit as duration
 // b - startValue: the beginning value
 // c - difference: targetValue - startValue
 // d - duration: the length of the animation

namespace {

const auto easeInQuad = []
(double t, double b, double c, double d) -> double
{
	return c * (t /= d) * t + b;
};

const auto easeOutQuad = []
(double t, double b, double c, double d) -> double
{
	return -c * (t /= d) * (t - 2) + b;
};

const auto easeInOutQuad = []
(double t, double b, double c, double d) -> double
{
	if ((t /= d / 2) < 1) return c / 2 * t * t + b;
	return -c / 2 * (--t * (t - 2) - 1) + b;
};

const auto easeInCubic = []
(double t, double b, double c, double d) -> double
{
	return c * (t /= d) * t * t + b;
};

const auto easeOutCubic = []
(double t, double b, double c, double d) -> double
{
	return c * ((t = t / d - 1) * t * t + 1) + b;
};

const auto easeInOutCubic = []
(double t, double b, double c, double d) -> double
{
	if ((t /= d / 2) < 1) return c / 2 * t * t * t + b;
	return c / 2 * ((t -= 2) * t * t + 2) + b;
};

const auto easeInQuart = []
(double t, double b, double c, double d) -> double
{
	return c * (t /= d) * t * t * t + b;
};

const auto easeOutQuart = []
(double t, double b, double c, double d) -> double
{
	return -c * ((t = t / d - 1) * t * t * t - 1) + b;
};

const auto easeInOutQuart = []
(double t, double b, double c, double d) -> double
{
	if ((t /= d / 2) < 1) return c / 2 * t * t * t * t + b;
	return -c / 2 * ((t -= 2) * t * t * t - 2) + b;
};

const auto easeInQuint = []
(double t, double b, double c, double d) -> double
{
	return c * (t /= d) * t * t * t * t + b;
};

const auto easeOutQuint = []
(double t, double b, double c, double d) -> double
{
	return c * ((t = t / d - 1) * t * t * t * t + 1) + b;
};

const auto easeInOutQuint = []
(double t, double b, double c, double d) -> double
{
	if ((t /= d / 2) < 1) return c / 2 * t * t * t * t * t + b;
	return c / 2 * ((t -= 2) * t * t * t * t + 2) + b;
};

const auto easeInSine = []
(double t, double b, double c, double d) -> double
{
	return -c * cos(t / d * (M_PI / 2)) + c + b;
};

const auto easeOutSine = []
(double t, double b, double c, double d) -> double
{
	return c * sin(t / d * (M_PI / 2)) + b;
};

const auto easeInOutSine = []
(double t, double b, double c, double d) -> double
{
	return -c / 2 * (cos(M_PI * t / d) - 1) + b;
};

const auto easeInExpo = []
(double t, double b, double c, double d) -> double
{
	return t == 0 ? b : c * pow(2, 10 * (t / d - 1)) + b;
};

const auto easeOutExpo = []
(double t, double b, double c, double d) -> double
{
	return t == d ? b + c : c * (-pow(2, -10 * t / d) + 1) + b;
};

const auto easeInOutExpo = []
(double t, double b, double c, double d) -> double
{
	if (t == 0) return b;
	if (t == d) return b + c;
	if ((t /= d / 2) < 1) return c / 2 * pow(2, 10 * (t - 1)) + b;
	return c / 2 * (-pow(2, -10 * --t) + 2) + b;
};

const auto easeInCirc = []
(double t, double b, double c, double d) -> double
{
	return -c * (sqrt(1 - (t /= d) * t) - 1) + b;
};

const auto easeOutCirc = []
(double t, double b, double c, double d) -> double
{
	return c * sqrt(1 - (t = t / d - 1) * t) + b;
};

const auto easeInOutCirc = []
(double t, double b, double c, double d) -> double
{
	if ((t /= d / 2) < 1) return -c / 2 * (sqrt(1 - t * t) - 1) + b;
	return c / 2 * (sqrt(1 - (t -= 2) * t) + 1) + b;
};

const auto easeInElastic = []
(double t, double b, double c, double d) -> double
{
	if (t == 0) return b;
	if ((t /= d) == 1) return b + c;
	double s = 1.70158, p = d * .3, a = c;
	if (a < abs(c)) a = c, s = p / 4;
	else s = p / (2 * M_PI) * asin(c / a);
	return -(a * pow(2, 10 * (t -= 1)) * sin((t * d - s) * (2 * M_PI) / p)) + b;
};

const auto easeOutElastic = []
(double t, double b, double c, double d) -> double
{
	if (t == 0) return b;
	if ((t /= d) == 1) return b + c;
	double s = 1.70158, p = d * .3, a = c;
	if (a < abs(c)) a = c, s = p / 4;
	else s = p / (2 * M_PI) * asin(c / a);
	return a * pow(2, -10 * t) * sin((t*d - s)*(2 * M_PI) / p) + c + b;
};

const auto easeInOutElastic = []
(double t, double b, double c, double d) -> double
{
	if (t == 0) return b;
	if ((t /= d / 2) == 2) return b + c;
	double s = 1.70158, p = d * .3 * 1.5, a = c;
	if (a < abs(c)) a = c, s = p / 4;
	else s = p / (2 * M_PI) * asin(c / a);
	return t < 1
		? -.5 * (a * pow(2, 10 * (t -= 1)) * sin((t * d - s) * (2 * M_PI) / p)) + b
		: a * pow(2, -10 * (t -= 1)) * sin((t * d - s) * (2 * M_PI) / p) * .5 + c + b;
};

const auto easeInBack = []
(double t, double b, double c, double d) -> double
{
	const double s = 1.70158;
	return c * (t /= d) * t * ((s + 1) * t - s) + b;
};

const auto easeOutBack = []
(double t, double b, double c, double d) -> double
{
	const double s = 1.70158;
	return c * ((t = t / d - 1) * t * ((s + 1) * t + s) + 1) + b;
};

const auto easeInOutBack = []
(double t, double b, double c, double d) -> double
{
	double s = 1.70158;
	return (t /= d / 2) < 1
		? c / 2 * (t * t * (((s *= 1.525) + 1) * t - s)) + b
		: c / 2 * ((t -= 2) * t * (((s *= 1.525) + 1) * t + s) + 2) + b;
};

const auto easeOutBounce = []
(double t, double b, double c, double d) -> double
{
	if ((t /= d) < (1 / 2.75)) return c * (7.5625 * t * t) + b;
	if (t < (2 / 2.75)) return c * (7.5625 * (t -= 1.5 / 2.75) * t + .75) + b;
	if (t < (2.5 / 2.75)) return c * (7.5625 * (t -= 2.25 / 2.75) * t + .9375) + b;
	return c * (7.5625 * (t -= 2.625 / 2.75) * t + .984375) + b;
};

const auto easeInBounce = []
(double t, double b, double c, double d) -> double
{
	return c - easeOutBounce(d - t, 0, c, d) + b;
};

const auto easeInOutBounce = []
(double t, double b, double c, double d) -> double
{
	return t < d / 2
		? easeInBounce(t * 2, 0, c, d) * .5 + b
		: easeOutBounce(t * 2 - d, 0, c, d) * .5 + c * .5 + b;
};

} // namespace

#define TO_PAIR(fn) { L#fn, fn }

const std::vector<std::pair<
	LPCWSTR,
	double (*)(double, double, double, double)
	>> easingFunctions
{
	TO_PAIR(easeInQuad),
	TO_PAIR(easeOutQuad),
	TO_PAIR(easeInOutQuad),
	TO_PAIR(easeInCubic),
	TO_PAIR(easeOutCubic),
	TO_PAIR(easeInOutCubic),
	TO_PAIR(easeInQuart),
	TO_PAIR(easeOutQuart),
	TO_PAIR(easeInOutQuart),
	TO_PAIR(easeInQuint),
	TO_PAIR(easeOutQuint),
	TO_PAIR(easeInOutQuint),
	TO_PAIR(easeInSine),
	TO_PAIR(easeOutSine),
	TO_PAIR(easeInOutSine),
	TO_PAIR(easeInExpo),
	TO_PAIR(easeOutExpo),
	TO_PAIR(easeInOutExpo),
	TO_PAIR(easeInCirc),
	TO_PAIR(easeOutCirc),
	TO_PAIR(easeInOutCirc),
	TO_PAIR(easeInElastic),
	TO_PAIR(easeOutElastic),
	TO_PAIR(easeInOutElastic),
	TO_PAIR(easeInBack),
	TO_PAIR(easeOutBack),
	TO_PAIR(easeInOutBack),
	TO_PAIR(easeInBounce),
	TO_PAIR(easeOutBounce),
	TO_PAIR(easeInOutBounce)
};
