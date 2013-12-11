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
 *   Author(s): Christophe Grosjean, Raphael Zhou, Jonathan Poelen
 */

#ifndef REDEMPTION_REGEX_STATE_HPP
#define REDEMPTION_REGEX_STATE_HPP

#include <ostream>
#include <algorithm>

#include "regex_consumer.hpp"

///TODO regex compiler ("..." -> C++)

namespace re {

    const unsigned RANGE            = 1;
    const unsigned FINISH           = 1 << 8;
    const unsigned SPLIT            = 1 << 9;
    const unsigned CAPTURE_OPEN     = 1 << 10;
    const unsigned CAPTURE_CLOSE    = 1 << 11;
    const unsigned EPSILONE         = 1 << 12;
    const unsigned FIRST            = 1 << 13;
    const unsigned LAST             = 1 << 14;

    struct State
    {
        explicit State(unsigned type, char_int range_left = 0, char_int range_right = 0,
                       State * out1 = 0, State * out2 = 0)
        : type(type)
        , num(0)
        , l(range_left)
        , r(range_right)
        , out1(out1)
        , out2(out2)
        {}

        bool check(char_int c) const {
            return this->l <= c && c <= this->r;
        }

        void display(std::ostream& os) const {
            switch (this->type) {
                case RANGE:
                    if (this->l == 0 && this->r == char_int(-1u)) {
                        os << ".";
                    }
                    else if (this->l == this->r) {
                        os << "[" << this->l << "] '"
                        << utf8_char(this->l) << "'";
                    }
                    else {
                        os << "[" << this->l << "-" << this->r << "] ['"
                        << utf8_char(this->l) << "'-'" << utf8_char(this->r)
                        << "']";
                    }
                    break;
                case CAPTURE_CLOSE: os << ")"; break;
                case CAPTURE_OPEN: os << "("; break;
                case EPSILONE: os << "(epsilone)"; break;
                case FINISH: os << "(finish)"; break;
                case SPLIT: os << "(split)"; break;
                case FIRST: os << "^"; break;
                case LAST: os << "$"; break;
                default: os << "???"; break;
            }
        }

        bool is_border() const
        { return this->type & (FIRST|LAST); }

        bool is_cap() const
        { return this->type & (CAPTURE_OPEN|CAPTURE_CLOSE); }

        bool is_cap_open() const
        { return this->type == CAPTURE_OPEN; }

        bool is_cap_close() const
        { return this->type == CAPTURE_CLOSE; }

        bool is_split() const
        { return this->type == SPLIT; }

        bool is_epsilone() const
        { return this->type == EPSILONE || (this->type == FINISH && this->out1); }

        bool is_finish() const
        { return this->type == FINISH; }

        bool is_terminate() const
        { return this->type & (LAST|FINISH); }

        bool is_range() const
        { return this->type == RANGE; }

        bool is_simple_char() const
        { return this->is_range() && this->l == this->r; }

        bool is_uninitialized() const
        { return this->type == 0; }

        unsigned type;
        unsigned num;

        char_int l;
        char_int r;

        State *out1;
        State *out2;

    private:
        State(const State &);
        State& operator=(const State &);
    };

    inline std::ostream& operator<<(std::ostream& os, const State& st)
    {
        st.display(os);
        return os;
    }

    inline State * new_character(char_int c, State * out1 = 0) {
        return new State(RANGE, c, c, out1);
    }

    inline State * new_range(char_int left, char_int right, State * out1 = 0) {
        return new State(RANGE, left, right, out1);
    }

    inline State * new_any(State * out1 = 0) {
        return new State(RANGE, 0, char_int(-1u), out1);
    }

    inline State * new_split(State * out1 = 0, State * out2 = 0) {
        return new State(SPLIT, 0, 0, out1, out2);
    }

    inline State * new_cap_open(State * out1 = 0) {
        return new State(CAPTURE_OPEN, 0, 0, out1);
    }

    inline State * new_cap_close(State * out1 = 0) {
        return new State(CAPTURE_CLOSE, 0, 0, out1);
    }

    inline State * new_epsilone(State * out1 = 0) {
        return new State(EPSILONE, 0, 0, out1);
    }

    inline State * new_finish(State * out1 = 0) {
        return new State(FINISH, 0, 0, out1);
    }

    inline State * new_begin(State * out1 = 0) {
        return new State(FIRST, 0, 0, out1);
    }

    inline State * new_last() {
        return new State(LAST, 0, 0);
    }

}

#endif
