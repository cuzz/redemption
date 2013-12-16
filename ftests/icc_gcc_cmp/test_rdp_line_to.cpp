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
    drawable.draw(RDPLineTo(1, 0, 10, 1000, 60, 0x102030, 0xFF, RDPPen(0, 5, 0x112233)), screen);

    display_timer timer;
    for (uint i = 0; i < 2000000u; ++i) {
        drawable.draw(RDPLineTo(1, 0, 10, 1000, 60, 0x102030, 0xFF, RDPPen(0, 5, 0x112233)), screen);
    }
}
