import std.stdio;
// import std.conv;
// import core.time.StopWatch;

ubyte[127*127*32] a;
ubyte[127*127*32] a2;

struct Op2_0x01 // R2_BLACK 0
{
		ubyte opCall(ubyte /*target*/, ubyte /*source*/)
		{
				return 0x00;
		}
};
struct Op2_0x02 // R2_NOTMERGEPEN DPon
{
		ubyte opCall(ubyte target, ubyte source)
		{
				return cast(ubyte)~(target | source);
		}
};
struct Op2_0x03 // R2_MASKNOTPEN DPna
{
		ubyte opCall(ubyte target, ubyte source)
		{
				return (target & ~source);
		}
};
struct Op2_0x04 // R2_NOTCOPYPEN Pn
{
		ubyte opCall(ubyte /*target*/, ubyte source)
		{
				return ~source;
		}
};
struct Op2_0x05 // R2_MASKPENNOT PDna
{
		ubyte opCall(ubyte target, ubyte source)
		{
				return (source & ~target);
		}
};
struct Op2_0x06 // R2_NOT Dn
{
		ubyte opCall(ubyte target, ubyte /*source*/)
		{
				return ~target;
		}
};
struct Op2_0x07 // R2_XORPEN DPx
{
		ubyte opCall(ubyte target, ubyte source)
		{
				return (target ^ source);
		}
};
struct Op2_0x08 // R2_NOTMASKPEN DPan
{
		ubyte opCall(ubyte target, ubyte source)
		{
				return cast(ubyte)~(target & source);
		}
};
struct Op2_0x09 // R2_MASKPEN DPa
{
		ubyte opCall(ubyte target, ubyte source)
		{
				return (target & source);
		}
};
struct Op2_0x0A // R2_NOTXORPEN DPxn
{
		ubyte opCall(ubyte target, ubyte source)
		{
				return cast(ubyte)~(target ^ source);
		}
};
// struct Op2_0x0B // R2_NOP D
// {
//     ubyte opCall(ubyte target, ubyte source)
//     {
//         return target;
//     }
// };
struct Op2_0x0C // R2_MERGENOTPEN DPno
{
		ubyte opCall(ubyte target, ubyte source)
		{
				return (target | ~source);
		}
};
struct Op2_0x0D // R2_COPYPEN P
{
		ubyte opCall(ubyte /*target*/, ubyte source)
		{
				return source;
		}
};
struct Op2_0x0E // R2_MERGEPENNOT PDno
{
		ubyte opCall(ubyte target, ubyte source)
		{
				return (source | ~target);
		}
};
struct Op2_0x0F // R2_MERGEPEN PDo
{
		ubyte opCall(ubyte target, ubyte source)
		{
				return (target | source);
		}
};
struct Op2_0x10 // R2_WHITE 1
{
		ubyte opCall(ubyte /*target*/, ubyte /*source*/)
		{
				return 0xFF;
		}
};

template Tf(Op) {
	void f() {
	// 		for (ubyte * end = a + sizeof(a)/sizeof(a[0]), * p = a, * p2 = a2; p != end; ++p, ++p2) {
	// 			*p = op(*p, *p2);
	// 		}
		// equivalent to:
	// 		std::transform(a,a+sizeof(a)/sizeof(a[0]),a2,a,op);

		Op op;
		for (int i = 0; i < a.length; ++i) {
			a[i] = op(a[i], a2[i]);
		}
	}
}

// void f(Op2_0x01)
// { std::memset(a, 0, sizeof(a)); }
//
// void f(Op2_0x10)
// { std::memset(a, 0xFF, sizeof(a)); }

void f(int rop)
{
	switch (rop) {
		case 0x01: // R2_BLACK
			Tf!(Op2_0x01).f();
						break;
		case 0x02: // R2_NOTMERGEPEN
			Tf!(Op2_0x02).f();
						break;
		case 0x03: // R2_MASKNOTPEN
			Tf!(Op2_0x03).f();
						break;
		case 0x04: // R2_NOTCOPYPEN
			Tf!(Op2_0x04).f();
						break;
		case 0x05: // R2_MASKPENNOT
			Tf!(Op2_0x05).f();
						break;
		case 0x06:  // R2_NOT
			Tf!(Op2_0x06).f();
						break;
		case 0x07:  // R2_XORPEN
			Tf!(Op2_0x07).f();
						break;
		case 0x08:  // R2_NOTMASKPEN
			Tf!(Op2_0x08).f();
						break;
		case 0x09:  // R2_MASKPEN
			Tf!(Op2_0x09).f();
						break;
		case 0x0A:  // R2_NOTXORPEN
			Tf!(Op2_0x0A).f();
						break;
		case 0x0B:  // R2_NOP
						break;
		case 0x0C:  // R2_MERGENOTPEN
			Tf!(Op2_0x0C).f();
						break;
		case 0x0D:  // R2_COPYPEN
			Tf!(Op2_0x0D).f();
						break;
		case 0x0E:  // R2_MERGEPENNOT
			Tf!(Op2_0x0E).f();
						break;
		case 0x0F:  // R2_MERGEPEN
			Tf!(Op2_0x0F).f();
						break;
		case 0x10: // R2_WHITE
			Tf!(Op2_0x10).f();
						break;
		default:
			Tf!(Op2_0x0D).f();
			break;
	}
}

// #include <iostream>
// #include <boost/timer.hpp>
//
// class display_timer
// {
// boost::timer timer;
// public:
// display_timer()
// {}
//
// ~display_timer()
// {
// 	const double elapsed = timer.elapsed();
// 	std::ios::fmtflags old_flags = std::cout.setf(std::istream::fixed, std::istream::floatfield);
// 	std::streamsize old_prec = std::cout.precision(2);
// 	std::cout << elapsed << " s" << std::endl;
// 	std::cout.flags(old_flags);
// 	std::cout.precision(old_prec);
// }

void main(char[][] av)
{
	if (av.length < 2) {
		return;
	}

	a[0..a.length] = 0;
	a2[0..a.length] = 2;


// 	int rop = parse!(int)(av[1]);
	int rop = av[1][0] - '0';
	if (av[1].length == 2) {
		rop = rop * 10 + av[1][1] - '0';
	}

	{
// 		__builtin_prefetch(a.a, 1, 1);
// 		__builtin_prefetch(a.a2, 1, 0);
// 		display_timer auto_display_timer;
// 		StopWatch sw;
// 		sw.start();
		for (int i = 0; i < 5000u; ++i) {
			f(rop);
		}
// 		sw.stop();
// 		writeln((i + 1) * 1_000_000, " times done, lap time: ",
// 						sw.peek().msecs, "[ms]");
	}
	writefln(a[0]);
}
