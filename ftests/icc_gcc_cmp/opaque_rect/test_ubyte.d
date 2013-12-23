void main(char[][] av)
{
	ubyte[127*127*32] a;
	a[0..a.length] = 0;

	const uint x = 25;
	const uint y = 25;
	const uint cx = 100;
	const uint cy = 100;
	const uint posbeg = y * cx + x;
	for (uint i = 0; i < 500000u; ++i) {
		for (uint iy = 0; iy < cy; ++iy) {
			uint pos = posbeg + iy * 127;
			a[pos..pos+cy] = 100;
		}
	}
}
