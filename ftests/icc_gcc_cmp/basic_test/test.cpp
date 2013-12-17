#include <algorithm>
#include <cstddef>
#include <cstring>
#include <stdint.h>

struct A {
	uint8_t a[127*127*32];
	uint8_t a2[127*127*32];

	A()
	{
		std::fill(a2+0, a2 + sizeof(a2)/sizeof(a2[0]), 2);
		std::fill(a+0, a + sizeof(a)/sizeof(a[0]), 0);
	}

	struct Op2_0x01 // R2_BLACK 0
	{
			uint8_t operator()(uint8_t /*target*/, uint8_t /*source*/) const
			{
					return 0x00;
			}
	};
	struct Op2_0x02 // R2_NOTMERGEPEN DPon
	{
			uint8_t operator()(uint8_t target, uint8_t source) const
			{
					return ~(target | source);
			}
	};
	struct Op2_0x03 // R2_MASKNOTPEN DPna
	{
			uint8_t operator()(uint8_t target, uint8_t source) const
			{
					return (target & ~source);
			}
	};
	struct Op2_0x04 // R2_NOTCOPYPEN Pn
	{
			uint8_t operator()(uint8_t /*target*/, uint8_t source) const
			{
					return ~source;
			}
	};
	struct Op2_0x05 // R2_MASKPENNOT PDna
	{
			uint8_t operator()(uint8_t target, uint8_t source) const
			{
					return (source & ~target);
			}
	};
	struct Op2_0x06 // R2_NOT Dn
	{
			uint8_t operator()(uint8_t target, uint8_t /*source*/) const
			{
					return ~target;
			}
	};
	struct Op2_0x07 // R2_XORPEN DPx
	{
			uint8_t operator()(uint8_t target, uint8_t source) const
			{
					return (target ^ source);
			}
	};
	struct Op2_0x08 // R2_NOTMASKPEN DPan
	{
			uint8_t operator()(uint8_t target, uint8_t source) const
			{
					return ~(target & source);
			}
	};
	struct Op2_0x09 // R2_MASKPEN DPa
	{
			uint8_t operator()(uint8_t target, uint8_t source) const
			{
					return (target & source);
			}
	};
	struct Op2_0x0A // R2_NOTXORPEN DPxn
	{
			uint8_t operator()(uint8_t target, uint8_t source) const
			{
					return ~(target ^ source);
			}
	};
	// struct Op2_0x0B // R2_NOP D
	// {
	//     uint8_t operator()(uint8_t target, uint8_t source) const
	//     {
	//         return target;
	//     }
	// };
	struct Op2_0x0C // R2_MERGENOTPEN DPno
	{
			uint8_t operator()(uint8_t target, uint8_t source) const
			{
					return (target | ~source);
			}
	};
	struct Op2_0x0D // R2_COPYPEN P
	{
			uint8_t operator()(uint8_t /*target*/, uint8_t source) const
			{
					return source;
			}
	};
	struct Op2_0x0E // R2_MERGEPENNOT PDno
	{
			uint8_t operator()(uint8_t target, uint8_t source) const
			{
					return (source | ~target);
			}
	};
	struct Op2_0x0F // R2_MERGEPEN PDo
	{
			uint8_t operator()(uint8_t target, uint8_t source) const
			{
					return (target | source);
			}
	};
	struct Op2_0x10 // R2_WHITE 1
	{
			uint8_t operator()(uint8_t /*target*/, uint8_t /*source*/) const
			{
					return 0xFF;
			}
	};

	template<typename Op>
	void f(Op op)
	{
// 		for (uint8_t * end = a + sizeof(a)/sizeof(a[0]), * p = a, * p2 = a2; p != end; ++p, ++p2) {
// 			*p = op(*p, *p2);
// 		}
		// equivalent to:
// 		std::transform(a,a+sizeof(a)/sizeof(a[0]),a2,a,op);

		for (size_t i = 0; i != sizeof(a)/sizeof(a[0]); ++i) {
			a[i] = op(a[i], a2[i]);
		}
	}

	void f(Op2_0x01)
	{ std::memset(a, 0, sizeof(a)); }

	void f(Op2_0x10)
	{ std::memset(a, 0xFF, sizeof(a)); }

	inline void f(int rop)
	{
		switch (rop) {
			case 0x01: // R2_BLACK
							f(Op2_0x01());
							break;
			case 0x02: // R2_NOTMERGEPEN
							f(Op2_0x02());
							break;
			case 0x03: // R2_MASKNOTPEN
							f(Op2_0x03());
							break;
			case 0x04: // R2_NOTCOPYPEN
							f(Op2_0x04());
							break;
			case 0x05: // R2_MASKPENNOT
							f(Op2_0x05());
							break;
			case 0x06:  // R2_NOT
							f(Op2_0x06());
							break;
			case 0x07:  // R2_XORPEN
							f(Op2_0x07());
							break;
			case 0x08:  // R2_NOTMASKPEN
							f(Op2_0x08());
							break;
			case 0x09:  // R2_MASKPEN
							f(Op2_0x09());
							break;
			case 0x0A:  // R2_NOTXORPEN
							f(Op2_0x0A());
							break;
			case 0x0B:  // R2_NOP
							break;
			case 0x0C:  // R2_MERGENOTPEN
							f(Op2_0x0C());
							break;
			case 0x0D:  // R2_COPYPEN
							f(Op2_0x0D());
							break;
			case 0x0E:  // R2_MERGEPENNOT
							f(Op2_0x0E());
							break;
			case 0x0F:  // R2_MERGEPEN
							f(Op2_0x0F());
							break;
			case 0x10: // R2_WHITE
							f(Op2_0x10());
							break;
			default:
				f(Op2_0x0D());
				break;
		}
	}
};

#include <iostream>
#include <boost/timer.hpp>

class display_timer
{
	boost::timer timer;
public:
	display_timer()
	{}

	~display_timer()
	{
		const double elapsed = this->timer.elapsed();
		std::ios::fmtflags old_flags = std::cout.setf(std::istream::fixed, std::istream::floatfield);
		std::streamsize old_prec = std::cout.precision(2);
		std::cout << elapsed << " s" << std::endl;
		std::cout.flags(old_flags);
		std::cout.precision(old_prec);
	}
};

int main(int ac, char ** av)
{
	if (ac < 2) {
		return 1;
	}
	int rop = std::atoi(av[1]);
	A a;
	{
// 		__builtin_prefetch(a.a, 1, 1);
// 		__builtin_prefetch(a.a2, 1, 0);
		display_timer auto_display_timer;
		for (unsigned i = 0; i < 5000u; ++i) {
			a.f(rop);
		}
	}
	std::cout << int(a.a[0]) << "\n";
}
