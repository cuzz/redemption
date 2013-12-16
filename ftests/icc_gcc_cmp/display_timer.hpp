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

#ifndef REDEMPTION_FTESTS_INTEL_COMPILER_DISPLAY_TIMER_HPP
#define REDEMPTION_FTESTS_INTEL_COMPILER_DISPLAY_TIMER_HPP

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

#endif