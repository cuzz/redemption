import std.stdio;
import std.conv;
import std.datetime;

ubyte[127*127*32] a;
ubyte[127*127*32] a2;


mixin template alg(alias op) {
	void f() {
		for (int i = 0; i < a.length; ++i) {
			a[i] = op(a[i], a2[i]);
		}
	}
}

void f(int rop)
{
	switch (rop) {
		case 0x01: // R2_BLACK
			mixin alg!(function(ubyte target, ubyte source){ return cast(ubyte)0x0; });
			f();
			break;
		case 0x02: // R2_NOTMERGEPEN
			mixin alg!(function(ubyte target, ubyte source){ return cast(ubyte)~(target | source); });
			f();
			break;
		case 0x03: // R2_MASKNOTPEN
			mixin alg!(function(ubyte target, ubyte source){ return cast(ubyte)(target & ~source); });
			f();
			break;
		case 0x04: // R2_NOTCOPYPEN
			mixin alg!(function(ubyte target, ubyte source){ return ~source; });
			f();
			break;
		case 0x05: // R2_MASKPENNOT
			mixin alg!(function(ubyte target, ubyte source){ return cast(ubyte)(source & ~target); });
			f();
			break;
		case 0x06:  // R2_NOT
			mixin alg!(function(ubyte target, ubyte source){ return ~target; });
			f();
			break;
		case 0x07:  // R2_XORPEN
			mixin alg!(function(ubyte target, ubyte source){ return cast(ubyte)(target ^ source); });
			f();
			break;
		case 0x08:  // R2_NOTMASKPEN
			mixin alg!(function(ubyte target, ubyte source){ return cast(ubyte)~(target & source); });
			f();
			break;
		case 0x09:  // R2_MASKPEN
			mixin alg!(function(ubyte target, ubyte source){ return cast(ubyte)(target & source); });
			f();
			break;
		case 0x0A:  // R2_NOTXORPEN
			mixin alg!(function(ubyte target, ubyte source){ return cast(ubyte)~(target ^ source); });
			f();
			break;
		case 0x0B:  // R2_NOP
			break;
		case 0x0C:  // R2_MERGENOTPEN
			mixin alg!(function(ubyte target, ubyte source){ return cast(ubyte)(target | ~source); });
			f();
			break;
		case 0x0D:  // R2_COPYPEN
			mixin alg!(function(ubyte target, ubyte source){ return source; });
			f();
			break;
		case 0x0E:  // R2_MERGEPENNOT
			mixin alg!(function(ubyte target, ubyte source){ return cast(ubyte)(source | ~target); });
			f();
			break;
		case 0x0F:  // R2_MERGEPEN
			mixin alg!(function(ubyte target, ubyte source){ return cast(ubyte)(target | source); });
			f();
			break;
		case 0x10: // R2_WHITE
			mixin alg!(function(ubyte target, ubyte source){ return cast(ubyte)0xFF; });
			f();
			break;
		default:
			mixin alg!(function(ubyte target, ubyte source){ return source; });
			f();
			break;
	}
}

void main(char[][] av)
{
	if (av.length < 2) {
		return;
	}

	a[0..a.length] = 0;
	a2[0..a.length] = 2;


	int rop = parse!(int)(av[1]);

	{
		SysTime systime1 = Clock.currTime();
		for (int i = 0; i < 5000u; ++i) {
			f(rop);
		}
		SysTime systime2 = Clock.currTime();
		writeln(systime2-systime1);
	}
	writeln(a[0]);
}
