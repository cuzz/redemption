#include <cstring>
#include <stdint.h>
// #include <iostream>
// #include <boost/timer.hpp>
//
// class display_timer
// {
// 	boost::timer timer;
// public:
// 	display_timer()
// 	{}
//
// 	~display_timer()
// 	{
// 		const double elapsed = this->timer.elapsed();
// 		std::ios::fmtflags old_flags = std::cout.setf(std::istream::fixed, std::istream::floatfield);
// 		std::streamsize old_prec = std::cout.precision(2);
// 		std::cout << elapsed << " s" << std::endl;
// 		std::cout.flags(old_flags);
// 		std::cout.precision(old_prec);
// 	}
// };

int main()
{
#ifndef TYPEB
	#define TYPEB uint8_t
#endif
	TYPEB a[127*127*32] = {0};
	const unsigned x = 25;
	const unsigned y = 25;
	const unsigned cx = 100;
	const unsigned cy = 100;
	TYPEB * p = a + y * cx + x;
// 	__builtin_prefetch(a, 1, 0);
// 	display_timer auto_display_timer;
	for (unsigned i = 0; i < 500000u; ++i) {
		for (unsigned iy = 0; iy < cy; ++iy) {
			memset(p + iy * 127, 100, cx);
		}
	}
	return p[100];
}
