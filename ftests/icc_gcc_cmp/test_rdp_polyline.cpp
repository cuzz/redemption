/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Product name: redemption, a FLOSS RDP proxy
 *   Copyright (C) Wallix 2010-2013
 *   Author(s): Christophe Grosjean, Raphael Zhou, Jonathan Poelen, Meng Tan
 */

#include "display_timer.hpp"
#include "RDP/RDPDrawable.hpp"

int main()
{
    RDPDrawable drawable(1000, 1000);
    const Rect screen(0, 0, 1000, 1000);

    {
        BStream deltaPoints(1024);

        deltaPoints.out_sint16_le(0);
        deltaPoints.out_sint16_le(20);

        deltaPoints.out_sint16_le(160);
        deltaPoints.out_sint16_le(0);

        deltaPoints.out_sint16_le(0);
        deltaPoints.out_sint16_le(-30);

        deltaPoints.out_sint16_le(50);
        deltaPoints.out_sint16_le(50);

        deltaPoints.out_sint16_le(-50);
        deltaPoints.out_sint16_le(50);

        deltaPoints.out_sint16_le(0);
        deltaPoints.out_sint16_le(-30);

        deltaPoints.out_sint16_le(-160);
        deltaPoints.out_sint16_le(0);

        deltaPoints.mark_end();
        deltaPoints.rewind();

        drawable.draw(RDPPolyline(0, 0, 0x0d, 0, 0x00, 7, deltaPoints), screen);
    }

    display_timer timer;
    for (uint i = 0; i < 4000000u; ++i) {
        BStream deltaPoints(1024);

        deltaPoints.out_sint16_le(0);
        deltaPoints.out_sint16_le(20);

        deltaPoints.out_sint16_le(160);
        deltaPoints.out_sint16_le(0);

        deltaPoints.out_sint16_le(0);
        deltaPoints.out_sint16_le(-30);

        deltaPoints.out_sint16_le(50);
        deltaPoints.out_sint16_le(50);

        deltaPoints.out_sint16_le(-50);
        deltaPoints.out_sint16_le(50);

        deltaPoints.out_sint16_le(0);
        deltaPoints.out_sint16_le(-30);

        deltaPoints.out_sint16_le(-160);
        deltaPoints.out_sint16_le(0);

        deltaPoints.mark_end();
        deltaPoints.rewind();

        drawable.draw(RDPPolyline(0, 0, 0x0d, 0, 0x00, 7, deltaPoints), screen);
    }
}
