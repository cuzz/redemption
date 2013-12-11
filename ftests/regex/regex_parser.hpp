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

#ifndef REDEMPTION_FTESTS_REGEX_REGEX_PARSER_HPP
#define REDEMPTION_FTESTS_REGEX_REGEX_PARSER_HPP

#include "regex_utils.hpp"

#include <algorithm>
#include <utility> //std::pair
#include <vector>
#include <new>

namespace re {

    class StateAccu
    {
    public:
        unsigned num_cap;
        state_list_t sts;
        std::vector<unsigned> indexes;
        typedef std::vector<std::pair<char_int, char_int> > c_range_type;
        c_range_type c_ranges;

    private:
#ifdef RE_PARSER_POOL_STATE
        struct memory_list_t
        {
            static const unsigned reserve_state = 31;
            memory_list_t * prev;
            unsigned len;
            State states[1];

            static memory_list_t * allocate(memory_list_t * cur)
            {
                memory_list_t * ret = static_cast<memory_list_t*>(
                    ::operator new(sizeof(memory_list_t) + (reserve_state - 1) * sizeof(State))
                );
                ret->prev = cur;
                ret->len = 0;
                return ret;
            }

            struct Deleter
            {
                void operator()(State & st) const
                {
                    st.~State();
                }
            };

            void clear()
            {
                std::for_each(this->states + 0, this->states + this->len, Deleter());
                this->len = 0;
            }

            static memory_list_t * deallocate(memory_list_t * cur)
            {
                memory_list_t * ret = cur->prev;
                cur->clear();
                ::operator delete(cur);
                return ret;
            }

            bool is_full() const
            {
                return this->len == reserve_state;
            }

            void * next()
            {
                return this->states + this->len++;
            }
        };

        memory_list_t * mem;
#endif

        State * push(unsigned type,
                     char_int range_left = 0, char_int range_right = 0,
                     State * out1 = 0, State * out2 = 0)
        {
#ifdef RE_PARSER_POOL_STATE
            if (this->mem->is_full()) {
                this->mem = memory_list_t::allocate(this->mem);
            }
            State * st = new(this->mem->next()) State(type, range_left, range_right, out1, out2);
#else
            State * st = new State(type, range_left, range_right, out1, out2);
#endif
            sts.push_back(st);
            return st;
        }

    public:
        StateAccu()
        : num_cap(0)
        , sts()
#ifdef RE_PARSER_POOL_STATE
        , mem(memory_list_t::allocate(0))
#endif
        {}

        ~StateAccu()
        {
            this->delete_state();
#ifdef RE_PARSER_POOL_STATE
            memory_list_t::deallocate(this->mem);
#endif
        }

        State * normal(unsigned type, char_int range_left = 0, char_int range_right = 0,
                       State * out1 = 0, State * out2 = 0)
        {
            return this->push(type, range_left, range_right, out1, out2);
        }

        State * character(char_int c, State * out1 = 0) {
            return this->push(RANGE, c, c, out1);
        }

        State * range(char_int left, char_int right, State * out1 = 0) {
            return this->push(RANGE, left, right, out1);
        }

        State * any(State * out1 = 0) {
            return this->push(RANGE, 0, char_int(-1u), out1);
        }

        State * split(State * out1 = 0, State * out2 = 0) {
            return this->push(SPLIT, 0, 0, out1, out2);
        }

        State * cap_open(State * out1 = 0) {
            State * ret = this->push(CAPTURE_OPEN, 0, 0, out1);
            ret->num = this->num_cap++;
            return ret;
        }

        State * cap_close(State * out1 = 0) {
            State * ret = this->push(CAPTURE_CLOSE, 0, 0, out1);
            ret->num = this->num_cap++;
            return ret;
        }

        State * epsilone(State * out1 = 0) {
            return this->push(EPSILONE, 0, 0, out1);
        }

        State * finish(State * out1 = 0) {
            return this->push(FINISH, 0, 0, out1);
        }

        State * begin(State * out1 = 0) {
            return this->push(FIRST, 0, 0, out1);
        }

        State * last() {
            return this->push(LAST);
        }

        void clear()
        {
            this->delete_state();
            this->num_cap = 0;
            this->c_ranges.clear();
            this->indexes.clear();
            this->sts.clear();
        }

        void clear_and_shrink()
        {
            this->delete_state();
            this->num_cap = 0;
#if __cplusplus >= 201103L
            this->c_ranges.clear();
            this->c_ranges.shrink_to_fit();
            this->indexes.clear();
            this->indexes.shrink_to_fit();
            this->sts.clear();
            this->sts.shrink_to_fit();
#else
            this->c_ranges.~vector();
            new (&this->c_ranges) c_range_type();
            this->indexes.~vector();
            new (&this->indexes) std::vector<unsigned>();
            this->sts.~vector();
            new (&this->sts) state_list_t();
#endif
        }

    private:
        void delete_state()
        {
#ifdef RE_PARSER_POOL_STATE
            while (this->mem->prev) {
                this->mem = memory_list_t::deallocate(this->mem);
            }
            this->mem->clear();
#else
            std::for_each(this->sts.begin(), this->sts.end(), StateDeleter());
#endif
        }
    };

    inline State ** c2range(StateAccu & accu,
                            State ** pst, State * eps,
                            char_int l1, char_int r1,
                            char_int l2, char_int r2)
    {
        *pst = accu.split(accu.range(l1, r1, eps),
                          accu.range(l2, r2, eps)
                         );
        return &eps->out1;
    }

    inline State ** c2range(StateAccu & accu,
                            State ** pst, State * eps,
                            char_int l1, char_int r1,
                            char_int l2, char_int r2,
                            char_int l3, char_int r3)
    {
        *pst = accu.split(accu.range(l1, r1, eps),
                          accu.split(accu.range(l2, r2, eps),
                                     accu.range(l3, r3, eps)
                                    )
                         );
        return &eps->out1;
    }

    inline State ** c2range(StateAccu & accu,
                            State ** pst, State * eps,
                            char_int l1, char_int r1,
                            char_int l2, char_int r2,
                            char_int l3, char_int r3,
                            char_int l4, char_int r4)
    {
        *pst = accu.split(accu.range(l1, r1, eps),
                          accu.split(accu.range(l2, r2, eps),
                                     accu.split(accu.range(l3, r3, eps),
                                                accu.range(l4, r4, eps)
                                     )
                         )
        );
        return &eps->out1;
    }

    inline State ** c2range(StateAccu & accu,
                            State ** pst, State * eps,
                            char_int l1, char_int r1,
                            char_int l2, char_int r2,
                            char_int l3, char_int r3,
                            char_int l4, char_int r4,
                            char_int l5, char_int r5)
    {
        *pst = accu.split(accu.range(l1, r1, eps),
                          accu.split(accu.range(l2, r2, eps),
                                     accu.split(accu.range(l3, r3, eps),
                                                accu.split(accu.range(l4, r4, eps),
                                                           accu.range(l5, r5, eps)
                                               )
                                    )
                         )
        );
        return &eps->out1;
    }

    inline State ** ident_D(StateAccu & accu, State ** pst, State * eps) {
        return c2range(accu, pst, eps, 0,'0'-1, '9'+1,-1u);
    }

    inline State ** ident_w(StateAccu & accu, State ** pst, State * eps) {
        return c2range(accu, pst, eps, 'a','z', 'A','Z', '0','9', '_', '_');
    }

    inline State ** ident_W(StateAccu & accu, State ** pst, State * eps) {
        return c2range(accu, pst, eps, 0,'0'-1, '9'+1,'A'-1, 'Z'+1,'_'-1, '_'+1,'a'-1, 'z'+1,-1u);
    }

    inline State ** ident_s(StateAccu & accu, State ** pst, State * eps) {
        return c2range(accu, pst, eps, ' ',' ', '\t','\v');
    }

    inline State ** ident_S(StateAccu & accu, State ** pst, State * eps) {
        return c2range(accu, pst, eps, 0,'\t'-1, '\v'+1,' '-1, ' '+1,-1u);
    }

    inline const char * check_interval(char_int a, char_int b)
    {
        bool valid = ('0' <= a && a <= '9' && '0' <= b && b <= '9')
                  || ('a' <= a && a <= 'z' && 'a' <= b && b <= 'z')
                  || ('A' <= a && a <= 'Z' && 'A' <= b && b <= 'Z');
        return (valid && a <= b) ? 0 : "range out of order in character class";
    }

    struct VectorRange
    {
        typedef std::pair<char_int, char_int> range_t;
        typedef std::vector<range_t> container_type;
        typedef container_type::iterator iterator;

        container_type & ranges;

        explicit VectorRange(container_type & c_ranges)
        : ranges(c_ranges)
        {
            this->ranges.clear();
        }

        void push(char_int left, char_int right) {
            ranges.push_back(range_t(left, right));
        }

        void push(char_int c) {
            this->push(c,c);
        }
    };

    inline char_int get_c(utf8_consumer & consumer, char_int c)
    {
        if (c != '[' && c != '.') {
            if (c == '\\') {
                char_int c2 = consumer.bumpc();
                switch (c2) {
                    case 0:
                        return '\\';
                    case 'd':
                    case 'D':
                    case 'w':
                    case 'W':
                    case 's':
                    case 'S':
                        return 0;
                    case 'n': return '\n';
                    case 't': return '\t';
                    case 'r': return '\r';
                    //case 'v': return '\v';
                    default : return c2;
                }
            }
            return c;
        }
        return 0;
    }

    inline bool is_range_repetition(const char * s)
    {
        const char * begin = s;
        while (*s && '0' <= *s && *s <= '9') {
            ++s;
        }
        if (begin == s || !*s || (*s != ',' && *s != '}')) {
            return false;
        }
        if (*s == '}') {
            return true;
        }
        begin = ++s;
        while (*s && '0' <= *s && *s <= '9') {
            ++s;
        }
        return *s && *s == '}';
    }

    inline bool is_meta_char(utf8_consumer & consumer, char_int c)
    {
        return c == '*' || c == '+' || c == '?' || c == '|' || c == '(' || c == ')' || c == '^' || c == '$' || (c == '{' && is_range_repetition(consumer.str()));
    }

    inline State ** st_compile_range(StateAccu & accu, State ** pst,
                                     utf8_consumer & consumer, char_int c, const char * & msg_err)
    {
        bool reverse_result = false;
        VectorRange ranges(accu.c_ranges);
        if (consumer.valid() && (c = consumer.bumpc()) != ']') {
            if (c == '^') {
                reverse_result = true;
                c = consumer.bumpc();
            }
            if (c == '-') {
                ranges.push('-');
                c = consumer.bumpc();
            }
            const unsigned char * cs = consumer.s;
            while (consumer.valid() && c != ']') {
                const unsigned char * p = consumer.s;
                char_int prev_c = c;
                while (c != ']' && c != '-') {
                    if (c == '\\') {
                        char_int cc = consumer.bumpc();
                        switch (cc) {
                            case 'd':
                                ranges.push('0',    '9');
                                break;
                            case 'D':
                                ranges.push(0,      '0'-1);
                                ranges.push('9'+1,  -1u);
                                break;
                            case 'w':
                                ranges.push('a',    'z');
                                ranges.push('A',    'Z');
                                ranges.push('0',    '9');
                                ranges.push('_');
                                break;
                            case 'W':
                                ranges.push(0,      '0'-1);
                                ranges.push('9'+1,  'A'-1);
                                ranges.push('Z'+1,  '_'-1);
                                ranges.push('_'+1,  'a'-1);
                                ranges.push('z'+1,  -1u);
                                break;
                            case 's':
                                ranges.push(' ');
                                ranges.push('\t',   '\v');
                                break;
                            case 'S':
                                ranges.push(0,      '\t'-1);
                                ranges.push('\v'+1, ' '-1);
                                ranges.push(' '+1,  -1u);
                                break;
                            case 'n': ranges.push('\n'); break;
                            case 't': ranges.push('\t'); break;
                            case 'r': ranges.push('\r'); break;
                            case 'v': ranges.push('\v'); break;
                            default : ranges.push(cc); break;
                        }
                    }
                    else {
                        ranges.push(c);
                    }

                    if ( ! consumer.valid()) {
                        break;
                    }

                    prev_c = c;
                    c = consumer.bumpc();
                }

                if (c == '-') {
                    if (cs == consumer.s) {
                        ranges.push('-');
                    }
                    else if (!consumer.valid()) {
                        msg_err = "missing terminating ]";
                        return 0;
                    }
                    else if (consumer.getc() == ']') {
                        ranges.push('-');
                        consumer.bumpc();
                    }
                    else if (consumer.s == p) {
                        ranges.push('-');
                    }
                    else {
                        c = consumer.bumpc();
                        if ((msg_err = check_interval(prev_c, c))) {
                            return 0;
                        }
                        if (ranges.ranges.size()) {
                            ranges.ranges.pop_back();
                        }
                        ranges.push(prev_c, c);
                        cs = consumer.s;
                        if (consumer.valid()) {
                            c = consumer.bumpc();
                        }
                    }
                }
            }
        }

        if (ranges.ranges.empty() || c != ']') {
            msg_err = "missing terminating ]";
            return 0;
        }

        if (ranges.ranges.size() == 1) {
            if (reverse_result) {
                State * eps = accu.epsilone();
                *pst = accu.split(accu.range(0, ranges.ranges[0].first-1, eps),
                                    accu.range(ranges.ranges[0].second+1, -1u, eps));
                return &eps->out1;
            }
            else {
                *pst = accu.range(ranges.ranges[0].first, ranges.ranges[0].second);
                return &(*pst)->out1;
            }
        }

        std::sort(ranges.ranges.begin(), ranges.ranges.end());

        VectorRange::iterator first = ranges.ranges.begin();
        VectorRange::iterator last = ranges.ranges.end();
        VectorRange::iterator result = first;

        if (first != last) {
            while (++first != last && !(result->second + 1 >= first->first)) {
                ++result;
            }
            for (; first != last; ++first) {
                if (result->second + 1 >= first->first) {
                    if (result->second < first->second) {
                        result->second = first->second;
                    }
                }
                else {
                    ++result;
                    *result = *first;
                }
            }
            ++result;
            ranges.ranges.resize(ranges.ranges.size() - (last - result));
        }

        State * eps = accu.epsilone();
        first = ranges.ranges.begin();
        if (reverse_result) {
            State * st = accu.range(0, first->first-1, eps);
            char_int cr = first->second;
            while (++first != result) {
                st = accu.split(st, accu.range(cr+1, first->first-1, eps));
                cr = first->second;
            }
            st = accu.split(st, accu.range(cr+1, -1u, eps));
            *pst = st;
        }
        else {
            State * st = accu.range(first->first, first->second, eps);
            while (++first != result) {
                st = accu.split(st, accu.range(first->first, first->second, eps));
            }
            *pst = st;
        }
        return &eps->out1;
    }


    class ContextClone
    {
        size_t pos;
        size_t poslast;
    public:
        state_list_t sts2;
    private:
        StateAccu & accu;
        unsigned nb_clone;

    public:
        ContextClone(StateAccu & accu, State * st_base, unsigned nb_clone)
        : pos(std::find(accu.sts.rbegin(), accu.sts.rend(), st_base).base() - accu.sts.begin() - 1)
        , poslast(accu.sts.size())
        , sts2()
        , accu(accu)
        , nb_clone(nb_clone)
        {
            const size_t size = this->poslast - this->pos;
            this->sts2.resize(size);
            this->accu.indexes.resize(size*2);
            this->accu.sts.reserve(this->accu.sts.size() + size*nb_clone);
            state_list_t::iterator first = accu.sts.begin() + this->pos;
            state_list_t::iterator last = accu.sts.begin() + this->poslast;
            std::vector<unsigned>::iterator idxit = this->accu.indexes.begin();
            for (; first != last; ++first) {
                if ((*first)->out1) {
                    *idxit = this->get_idx((*first)->out1);
                    ++idxit;
                }
                if ((*first)->out2) {
                    *idxit = this->get_idx((*first)->out2);
                    ++idxit;
                }
            }
        }

        State * clone()
        {
            --this->nb_clone;
            state_list_t::iterator last = this->accu.sts.begin() + this->poslast;
            state_list_t::iterator first = this->accu.sts.begin() + this->pos;
            state_list_t::iterator first2 = this->sts2.begin();
            for (; first != last; ++first, ++first2) {
                *first2 = this->copy(*first);
            }
            std::vector<unsigned>::iterator idxit = this->accu.indexes.begin();
            first = this->accu.sts.begin() + this->pos;
            first2 = this->sts2.begin();
            for (; first != last; ++first, ++first2) {
                if ((*first)->out1) {
                    (*first2)->out1 = this->sts2[*idxit];
                    ++idxit;
                }
                if ((*first)->out2) {
                    (*first2)->out2 = this->sts2[*idxit];
                    ++idxit;
                }
            }
            return this->sts2.front();
        }

        std::size_t get_idx(State * st) const {
            return std::find(accu.sts.begin() + this->pos,
                             accu.sts.begin() + this->poslast, st)
            - (accu.sts.begin() + this->pos);
        }

    private:
        State * copy(State * st) {
            State * ret = this->accu.normal(st->type, st->l, st->r);
            if (st->is_cap()) {
                if (!this->nb_clone) {
                    ret->num = st->num;
                    st->num = 0;
                    st->type = EPSILONE;
                }
                else {
                    ret->type = EPSILONE;
                }
            }
            return ret;
        }
    };

    typedef std::pair<State*, State**> IntermendaryState;

    inline IntermendaryState intermendary_st_compile(StateAccu & accu,
                                                     utf8_consumer & consumer,
                                                     const char * & msg_err,
                                                     int recusive = 0)
    {
        State st(EPSILONE);
        State ** pst = &st.out1;
        State ** spst = pst;
        State * bst = &st;
        State * eps = 0;
        bool special = true;

        char_int c = consumer.bumpc();

        while (c) {
            /**///std::cout << "c: " << (c) << std::endl;
            if (c == '^' || c == '$') {
                *pst = c == '^' ? accu.begin() : accu.last();
                c = consumer.bumpc();
                pst = &(*pst)->out1;
                special = true;
                continue;
            }

            if (!is_meta_char(consumer, c)) {
                do {
                    spst = pst;
                    if (c == '.') {
                        pst = &(*pst = accu.any())->out1;
                    }
                    else if (c == '[') {
                        if (!(pst = st_compile_range(accu, pst, consumer, c, msg_err))) {
                            return IntermendaryState(0,0);
                        }
                    }
                    else if (c == '\\') {
                        char_int c2 = consumer.bumpc();
                        switch (c2) {
                            case 0: pst = &(*pst = accu.character(c))->out1; break;
                            case 'd': pst = &(*pst = accu.range('0','9'))->out1; break;
                            case 'D': pst = ident_D(accu, pst, accu.epsilone()); break;
                            case 'w': pst = ident_w(accu, pst, accu.epsilone()); break;
                            case 'W': pst = ident_W(accu, pst, accu.epsilone()); break;
                            case 's': pst = ident_s(accu, pst, accu.epsilone()); break;
                            case 'S': pst = ident_S(accu, pst, accu.epsilone()); break;
                            case 'n': pst = &(*pst = accu.character('\n'))->out1; break;
                            case 't': pst = &(*pst = accu.character('\t'))->out1; break;
                            case 'r': pst = &(*pst = accu.character('\r'))->out1; break;
                            case 'v': pst = &(*pst = accu.character('\v'))->out1; break;
                            default : pst = &(*pst = accu.character(c2))->out1; break;
                        }
                        c = c2;
                    }
                    else {
                        pst = &(*pst = accu.character(c))->out1;
                    }
                    if (is_meta_char(consumer, c = consumer.bumpc())) {
                        break;
                    }
                } while (c);
                special = false;
            }
            else {
                if (special && c != '(' && c != ')' && c != '|') {
                    msg_err = "nothing to repeat";
                    return IntermendaryState(0,0);
                }
                switch (c) {
                    case '?': {
                        *pst = accu.finish();
                        *spst = accu.split(*pst, *spst);
                        pst = &(*pst)->out1;
                        spst = pst;
                        special = true;
                        break;
                    }
                    case '*':
                        *spst = accu.split(accu.finish(), *spst);
                        *pst = *spst;
                        pst = &(*spst)->out1->out1;
                        spst = pst;
                        special = true;
                        break;
                    case '+':
                        *pst = accu.split(accu.finish(), *spst);
                        spst = pst;
                        pst = &(*pst)->out1->out1;
                        special = true;
                        break;
                    case '|':
                        if (!eps) {
                            eps = accu.epsilone();
                        }
                        *pst = eps;
                        bst = accu.split(bst == &st ? st.out1 : bst);
                        pst = &bst->out2;
                        spst = pst;
                        special = true;
                        break;
                    case '{': {
                        special = true;
                        /**///std::cout << ("{") << std::endl;
                        char * end = 0;
                        unsigned m = strtoul(consumer.str(), &end, 10);
                        /**///std::cout << ("end ") << *end << std::endl;
                        /**///std::cout << "m: " << (m) << std::endl;
                        if (*end != '}') {
                            /**///std::cout << ("reste") << std::endl;
                            //{m,}
                            if (*(end+1) == '}') {
                                /**///std::cout << ("infini") << std::endl;
                                if (m == 1) {
                                    *pst = accu.split(accu.finish(), *spst);
                                    spst = pst;
                                    pst = &(*pst)->out1->out1;
                                }
                                else if (m) {
                                    State * e = accu.finish();
                                    *pst = e;
                                    ContextClone cloner(accu, *spst, m-1);
                                    std::size_t idx = cloner.get_idx(e);
                                    State ** lst = &e->out1;
                                    while (--m) {
                                        *pst = cloner.clone();
                                        lst = pst;
                                        pst = &cloner.sts2[idx]->out1;
                                    }
                                    *pst = accu.split(e, *lst);
                                    pst = &e->out1;
                                }
                                else {
                                    *spst = accu.split(accu.finish(), *spst);
                                    *pst = *spst;
                                    pst = &(*spst)->out1->out1;
                                    spst = pst;
                                }
                            }
                            //{m,n}
                            else {
                                /**///std::cout << ("range") << std::endl;
                                unsigned n = strtoul(end+1, &end, 10);
                                if (m > n || (0 == m && 0 == n)) {
                                    msg_err = "numbers out of order in {} quantifier";
                                    return IntermendaryState(0,0);
                                }
                                /**///std::cout << "n: " << (n) << std::endl;
                                n -= m;
                                if (n > 50) {
                                    msg_err = "numbers too large in {} quantifier";
                                    return IntermendaryState(0,0);
                                }
                                if (0 == m) {
                                    --end;
                                    /**///std::cout << ("m = 0") << std::endl;
                                    if (n != 1) {
                                        /**///std::cout << ("n != 1") << std::endl;
                                        State * e = accu.finish();
                                        State * split = accu.split();
                                        *pst = split;
                                        ContextClone cloner(accu, *spst, n-1);
                                        std::size_t idx = cloner.get_idx(split);
                                        split->out1 = e;
                                        State * cst = split;

                                        while (--n) {
                                            cst->out2 = cloner.clone();
                                            cst = cloner.sts2[idx];
                                            cst->out1 = e;
                                        }
                                        cst->type = EPSILONE;
                                        pst = &e->out1;
                                        *spst = accu.split(e, *spst);
                                    }
                                    else {
                                        *pst = accu.finish();
                                        *spst = accu.split(*pst, *spst);
                                        pst = &(*pst)->out1;
                                    }
                                }
                                else {
                                    --end;
                                    State * finish = accu.finish();
                                    State * e = accu.epsilone();
                                    *pst = e;
                                    ContextClone cloner(accu, *spst, m-1+n);
                                    std::size_t idx = cloner.get_idx(e);
                                    pst = &e->out1;
                                    State * lst = e;
                                    while (--m) {
                                        *pst = cloner.clone();
                                        lst = cloner.sts2[idx];
                                        pst = &lst->out1;
                                    }

                                    while (n--) {
                                        lst->type = SPLIT;
                                        lst->out1 = finish;
                                        lst->out2 = cloner.clone();
                                        lst = cloner.sts2[idx];
                                    }
                                    lst->out1 = finish;
                                    lst->type = EPSILONE;
                                    pst = &finish->out1;
                                }
                            }
                        }
                        //{0}
                        else if (0 == m) {
                            msg_err = "numbers is 0 in {}";
                            return IntermendaryState(0,0);
                        }
                        //{m}
                        else {
                            if (1 != m) {
                                /**///std::cout << ("fixe ") << m << std::endl;
                                State * e = accu.epsilone();
                                *pst = e;
                                ContextClone cloner(accu, *spst, m-1);
                                std::size_t idx = cloner.get_idx(e);
                                while (--m) {
                                    /**///std::cout << ("clone") << std::endl;
                                    *pst = cloner.clone();
                                    pst = &cloner.sts2[idx]->out1;
                                }
                            }
                            end -= 1;
                        }
                        consumer.str(end + 1 + 1);
                        /**///std::cout << "'" << (*consumer.s) << "'" << std::endl;
                        break;
                    }
                    case ')':
                        if (0 == recusive) {
                            msg_err = "unmatched parentheses";
                            return IntermendaryState(0,0);
                        }

                        if (!eps) {
                            eps = accu.epsilone();
                        }
                        *pst = eps;
                        pst = &eps->out1;
                        return IntermendaryState(bst == &st ? st.out1 : bst, pst);
                        break;
                    default:
                        return IntermendaryState(0,0);
                    case '(':
                        special = false;
                        if (*consumer.s == '?' && *(consumer.s+1) == ':') {
                            if (!*consumer.s || !(*consumer.s+1) || !(*consumer.s+2)) {
                                msg_err = "unmatched parentheses";
                                return IntermendaryState(0,0);
                            }
                            consumer.s += 2;
                            State * epsgroup = accu.epsilone();
                            IntermendaryState intermendary = intermendary_st_compile(accu, consumer, msg_err, recusive+1);
                            if (intermendary.first) {
                                epsgroup->out1 = intermendary.first;
                                *pst = epsgroup;
                                spst = pst;
                                pst = intermendary.second;
                            }
                            else if (0 == intermendary.second) {
                                return IntermendaryState(0,0);
                            }
                            break;
                        }
                        State * stopen = accu.cap_open();
                        IntermendaryState intermendary = intermendary_st_compile(accu, consumer, msg_err, recusive+1);
                        if (intermendary.first) {
                            stopen->out1 = intermendary.first;
                            *pst = stopen;
                            spst = pst;
                            pst = intermendary.second;
                            *pst = accu.cap_close();
                            pst = &(*pst)->out1;
                        }
                        else if (0 == intermendary.second) {
                            return IntermendaryState(0,0);
                        }
                        break;
                }
                c = consumer.bumpc();
            }
        }

        if (0 != recusive) {
            msg_err = "unmatched parentheses";
            return IntermendaryState(0,0);
        }
        return IntermendaryState(bst == &st ? st.out1 : bst, pst);
    }

    class StateParser
    {
#ifdef RE_PARSER_POOL_STATE
        struct Deleter {
            void operator()(State *) const
            {}
        };
#else
        typedef StateDeleter Deleter;
#endif
    public:
        StateParser()
        : m_root(0)
        , m_accu()
        {
            this->m_accu.sts.reserve(32);
        }

        void compile(const char * s, const char * * msg_err = 0, size_t * pos_err = 0)
        {
            this->m_accu.clear();

            const char * err = 0;
            utf8_consumer consumer(s);
            this->m_root = intermendary_st_compile(this->m_accu, consumer, err).first;
            if (msg_err) {
                *msg_err = err;
            }
            if (pos_err) {
                *pos_err = err ? consumer.str() - s : 0;
            }

            while (this->m_root && this->m_root->is_epsilone()) {
                this->m_root = this->m_root->out1;
            }

            if (err || ! this->m_root) {
                this->m_accu.clear();
            }
            else if (this->m_root) {
                remove_epsilone(this->m_accu.sts, Deleter());

                state_list_t::iterator first = this->m_accu.sts.begin();
                state_list_t::iterator last = this->m_accu.sts.end();
                state_list_t::iterator first_cap = std::stable_partition(first, last, IsNotCapture());

                for (unsigned n = this->nb_capture(); first != first_cap; ++first, ++n) {
                    (*first)->num = n;
                }
            }
        }

        ~StateParser()
        {
            this->m_accu.clear();
        }

        const State * root() const
        {
            return this->m_root;
        }

        const state_list_t & states() const
        {
            return this->m_accu.sts;
        }

        unsigned nb_capture() const
        {
            return this->m_accu.num_cap;
        }

        bool empty() const
        {
            return this->m_accu.sts.empty();
        }

        void clear()
        {
            this->m_accu.clear();
            this->m_root = 0;
        }

        void clear_and_shrink()
        {
            this->m_accu.clear_and_shrink();
            this->m_root = 0;
        }

    private:
        StateParser(const StateParser &);
        StateParser& operator=(const StateParser &);

        State * m_root;
        StateAccu m_accu;
    };
}

#endif