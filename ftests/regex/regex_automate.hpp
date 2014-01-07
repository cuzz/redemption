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

#ifndef REDEMPTION_FTESTS_REGEX_REGEX_AUTOMATE_HPP
#define REDEMPTION_FTESTS_REGEX_REGEX_AUTOMATE_HPP

#include <iostream>
#include <new>
#include <vector>
#include <utility>
#include <algorithm>
#include <iterator>

#include "regex_utils.hpp"

namespace re {
#ifdef DISPLAY_TRACE
    enum { g_trace_active = 1 };
#else
    enum { g_trace_active = 0 };
#endif

#define RE_SHOW !::re::g_trace_active ? void() : void

template<typename InputIt, typename InputIt2, typename OutputIt>
OutputIt unique_merge(InputIt first1, InputIt last1,
                      InputIt2 first2, InputIt2 last2,
                      OutputIt d_first)
{
    if (first1 != last1 && first2 != last2) {
        typedef typename std::iterator_traits<InputIt>::value_type value_type;
        value_type x = (*first1 < *first2) ? *first1++ : *first2++;
        *d_first = x;
        ++d_first;

        while (first1 != last1) {
            if (first2 == last2) {
                while (first1 != last1 && x == *first1) {
                    ++first1;
                }
                d_first = std::unique_copy(first1, last1, d_first);
                break ;
            }
            if (*first2 < *first1) {
                if (x != *first2) {
                    x = *first2;
                    *d_first = x;
                    ++d_first;
                }
                ++first2;
            } else {
                if (x != *first1) {
                    x = *first1;
                    *d_first = x;
                    ++d_first;
                }
                ++first1;
            }
        }

        while (first2 != last2 && x == *first2) {
            ++first2;
        }
    }
    else if (first1 != last1) {
        return std::unique_copy(first1, last1, d_first);
    }

    return std::unique_copy(first2, last2, d_first);
}


class StateMachine2
{
    class StateRange;

    struct NState {
        std::vector<uint> nums;
        StateRange * sr;
        char_int l;
        char_int r;

        bool operator==(NState const other) const
        {
            return other.nums == this->nums
                && other.l == this->l
                && other.r == this->r;
        }

        bool is_range() const
        { return !(l > r); }

        bool is_terminate() const
        { return 0 == r && 1 == l; }

        bool is_beginning() const
        { return 0 == r && 2 == l; }
    };

    struct StateRange
    {
        std::vector<unsigned> stg;
        std::vector<NState> v;
        bool is_finish;
    };

    struct SameRange
    {
        StateRange const & csr;

        bool operator()(StateRange& sr) const
        {
            return sr.is_finish == csr.is_finish && sr.v == csr.v;
        }
    };

    class NStateSearch;

    struct StateRangeSearch
    {
        NStateSearch * v;
        size_t sz;
        bool is_finish;
        unsigned step;
    };

    struct NStateSearch {
        StateRangeSearch * sr;
        char_int l;
        char_int r;

        bool is_range() const
        { return !(l > r); }

        bool is_terminate() const
        { return 0 == r && 1 == l; }

        bool is_beginning() const
        { return 0 == r && 2 == l; }
    };

    bool add_beginning(std::vector<NState> & v, const State * st)
    {
        struct Impl {
            static bool run(std::vector<NState> & v, const State * st, bool is_beg) {
                if (!st || st->is_finish()) {
                    return true;
                }

                if (st->is_split()) {
                    bool ret = run(v, st->out1, is_beg);
                    return run(v, st->out2, is_beg) || ret;
                }

                if (st->type == FIRST) {
                    return run(v, st->out1, true);
                }

                if (!is_beg) {
                    return false;
                }

                else if (st->is_range()) {
                    v.push_back({{st->num}, nullptr, st->l, st->r});
                }
                else if (st->type == LAST) {
                    v.push_back({{st->num}, nullptr, 1, 0});
                }
                else {
                    return run(v, st->out1, true);
                }

                return false;
            }
        };

        return Impl::run(v, st, false);
    }

    static bool add_first(std::vector<NState> & v, const State * st)
    {
        struct Impl {
            static bool run(std::vector<NState> & v, const State * st) {
                if (!st || st->is_finish()) {
                    return true;
                }

                if (st->is_split()) {
                    bool ret = run(v, st->out1);
                    return run(v, st->out2) || ret;
                }

                if (st->is_range()) {
                    v.push_back({{st->num}, nullptr, st->l, st->r});
                }
                else if (st->type == FIRST) {
                    return false;
                }
                else if (st->type == LAST) {
                    v.push_back({{st->num}, nullptr, 1, 0});
                }
                else {
                    return run(v, st->out1);
                }

                return false;
            }
        };

        return Impl::run(v, st);
    }

    static bool add_next_sts(std::vector<NState> & v, const State * st)
    {
        struct Impl {
            static bool run(std::vector<NState> & v, const State * st) {
                if (!st || st->is_finish()) {
                    return true;
                }

                if (st->is_split()) {
                    bool ret = run(v, st->out1);
                    return run(v, st->out2) || ret;
                }

                if (st->is_range()) {
                    v.push_back({{st->num}, nullptr, st->l, st->r});
                }
                else if (st->type == LAST) {
                    v.push_back({{st->num}, nullptr, 1, 0});
                }
                else if (st->type == FIRST) {
                    v.push_back({{st->num}, nullptr, 2, 0});
                    //return run(v, st->out1);
                }
                else {
                    return run(v, st->out1);
                }

                return false;
            }
        };

        return Impl::run(v, st);
    }

    typedef std::pair<bool,bool> pre_cond_t;

    pre_cond_t pre_condition_search(const char * s) const
    {
        if (0 == this->nb_states) {
            return pre_cond_t(true, !*s);
        }

        if (yes_finish) {
            RE_SHOW(std::cout << "yes_finish\n");
            return pre_cond_t(true, true);
        }

        if (!s_sr_beg) {
            RE_SHOW(std::cout << "!s_sr_beg\n");
            return pre_cond_t(true, yes_beg);
        }

        if (!s_sr_beg->v) {
            RE_SHOW(std::cout << "!s_sr_beg->v\n");
            return pre_cond_t(true, true);
        }

        if (s_sr_beg->sz == 1 && s_sr_beg->v->is_terminate()) {
            if (yes_beg) {
                RE_SHOW(std::cout << "is ^$");
                return pre_cond_t(true, !*s);
            }
            else {
                RE_SHOW(std::cout << "is_terminate\n");
                return pre_cond_t(true, true);
            }
        }

        return pre_cond_t(false, false);
    }

    struct set_auto_pos
    {
        size_t * pos;
        const char * s;
        utf8_consumer & consumer;

        set_auto_pos(size_t * pos, const char * s, utf8_consumer & cons)
        : pos(pos)
        , s(s)
        , consumer(cons)
        {}

        ~set_auto_pos()
        {
            if (this->pos) {
                *this->pos = this->consumer.str() - this->s;
            }
        }
    };

    template<bool> struct ExactMatch { };

    bool exact_search_impl(const char * s, size_t * pos) const
    {
        return exact_search_impl(s, ExactMatch<true>(), pos);
    }

    template<bool exact_match>
    bool exact_search_impl(const char * s, ExactMatch<exact_match>, size_t * pos) const
    {
        //std::cout << "exact_search (exact = " << exact_match << ")\n";
        //std::cout << "sr_beg: " << s_sr_beg << ", sr_first: " << s_sr_first << "\n";

        if (pos) {
            *pos = 0;
        }

        {
            const pre_cond_t p = pre_condition_search(s);
            if (p.first) {
                return p.second;
            }
        }

        const StateRangeSearch * sr = s_sr_beg;
        utf8_consumer consumer(s);

        set_auto_pos auto_pos(pos, s, consumer);

        next_char:
        while (consumer.valid()) {
            if (!sr->v) {
                RE_SHOW(std::cout << "sr->v = nullptr\n");
                return !exact_match;
            }
            const char_int c = consumer.bumpc();
            RE_SHOW(std::cout << "\033[33m" << utf8_char(c) << "\033[0m\n");
            for (size_t i = 0; i < sr->sz; ++i) {
                const NStateSearch & nst = sr->v[i];
                RE_SHOW(std::cout << "\t[" << utf8_char(nst.l) << "-" << utf8_char(nst.l) << "]\n");
                if (nst.l <= c && c <= nst.r) {
                    RE_SHOW(std::cout << "\t\tok\n");
                    sr = nst.sr;
                    goto next_char;
                }
            }
            RE_SHOW(std::cout << "none\n");
            return false;
        }

        if (sr->v && !sr->is_finish) {
            RE_SHOW(std::cout << "! st.v.empty" << std::endl);
            for (size_t i = 0; i < sr->sz; ++i) {
                if (sr->v[i].is_terminate()) {
                    return true;
                }
            }
        }

        return !sr->v || sr->is_finish;
    }

    bool search_impl(const char * s, size_t * pos) const
    {
        //std::cout << "search\n";
        if (!s_sr_first) {
            RE_SHOW(std::cout << "call exact_search\n");
            return exact_search_impl(s, ExactMatch<false>(), pos);
        }

        if (pos) {
            *pos = 0;
        }

        {
            const pre_cond_t p = pre_condition_search(s);
            if (p.first) {
                return p.second;
            }
        }

        if (!nb_states || !srss->v) {
            return true;
        }

        for (size_t i = 0; i < nb_states; ++i) {
            srss[i].step = 0;
        }

        unsigned step = 0;
        std::vector<StateRangeSearch*> srs1(1, s_sr_beg) /*({s_sr_beg})*/;
        std::vector<StateRangeSearch*> srs2;
        srs1.reserve(nb_states);
        srs2.reserve(nb_states);

        if (!s_sr_beg->v || s_sr_beg->is_finish) {
            RE_SHOW(std::cout << ("first is finish") << std::endl);
            return true;
        }

        utf8_consumer consumer(s);

        set_auto_pos auto_pos(pos, s, consumer);

        while (consumer.valid()) {
            ++step;
            const char_int c = consumer.bumpc();
            RE_SHOW(std::cout << "\033[33m" << utf8_char(c) << "\033[0m\n");
            for (StateRangeSearch * sr: srs1) {
                if (sr->step == step) {
                    continue ;
                }
                sr->step = step;
                for (size_t i = 0; i < sr->sz; ++i) {
                    const NStateSearch & nst = sr->v[i];
                    RE_SHOW(std::cout << "\t[" << utf8_char(nst.l) << "-" << utf8_char(nst.l) << "]\n");
                    if (nst.l <= c && c <= nst.r) {
                        RE_SHOW(std::cout << "\t\tok\n");
                        srs2.push_back(nst.sr);
                        if (!nst.sr->v || nst.sr->is_finish) {
                            RE_SHOW(std::cout << ("finish") << std::endl);
                            return true;
                        }
                        break;
                    }
                }
            }
            swap(srs1, srs2);
            srs1.push_back(s_sr_first);
            srs2.clear();
        }

        RE_SHOW(std::cout << "consumer.empty\n");

        for (StateRangeSearch * sr: srs1) {
            if (sr->v && !sr->is_finish) {
                RE_SHOW(std::cout << "! st.v.empty" << std::endl);
                for (size_t i = 0; i < sr->sz; ++i) {
                    if (sr->v[i].is_terminate()) {
                        RE_SHOW(std::cout << "is_terminate" << std::endl);
                        return true;
                    }
                }
            }
            else {
                RE_SHOW(std::cout << "st.v.empty" << std::endl);
                return true;
            }
        }

        return false;
    }

    StateRangeSearch * srss;
    StateRangeSearch * s_sr_beg;
    StateRangeSearch * s_sr_first;
    size_t nb_states;
    bool yes_beg;
    bool yes_finish;

public:
    StateMachine2(const state_list_t & sts, const State * root, unsigned /*nb_capture*/,
                bool /*copy_states*/ = false, bool /*minimal_mem*/ = false)
    : srss(0)
    , s_sr_beg(0)
    , s_sr_first(0)
    , nb_states(sts.size())
    , yes_beg(false)
    , yes_finish(false)
    {
        if (!sts.size()) {
            RE_SHOW(std::cout << "sts is empty\n");
            return ;
        }

        std::vector<StateRange> srs;
        StateRange * sr_beg = nullptr;
        StateRange * sr_first = nullptr;

        srs.push_back({{-1u}, {}, false});
        {
            StateRange & sr = srs.back();
            if (add_beginning(sr.v, root)) {
                yes_finish = true;
                sr.is_finish = true;
            }
            if (!sr.v.empty()) {
                yes_beg = true;
                srs.push_back({{}, {}, false});
                sr_beg = &sr;
            }
        }
        {
            StateRange & sr = srs.back();
            if (add_first(sr.v, root)) {
                yes_finish = true;
                sr.is_finish = true;
            }
            if (!sr.v.empty()) {
                sr_first = &sr;
            }
            else {
                srs.pop_back();
            }
        }

        if (srs.empty()) {
            return ;
        }

        //insert
        for (State * st: sts) {
            if (st->is_range() || st->type == LAST/* || st->type == FIRST*/) {
                srs.push_back({{st->num}, {}, false});
                StateRange & sr = srs.back();
                if (add_next_sts(sr.v, st->out1)) {
                    sr.is_finish = true;
                }
            }
        }

        if (!srs.empty()) {
            for (std::size_t i = srs.size(); i-- > 1;) {
                StateRange const & csr = srs[i];
                auto it = std::find_if(srs.begin(), srs.begin()+i, SameRange{csr});
                if (it != srs.begin()+i) {
                    const unsigned new_id = it->stg[0];
                    const unsigned old_id = csr.stg[0];
                    RE_SHOW(std::cout << "old_id: " << old_id << "\tnew_id: " << new_id << std::endl);
                    for (StateRange & sr: srs) {
                        for (NState & nst: sr.v) {
                            if (nst.nums[0] == old_id) {
                                nst.nums[0] = new_id;
                            }
                        }
                    }
                    srs.erase(srs.begin()+i);
                }
            }
        }

        struct Display {
            void operator()(std::vector<StateRange> & srs) const {
                if (g_trace_active) {
                    for (StateRange & sr : srs) {
                        std::cout << ("ranges: ");
                        for (unsigned num: sr.stg) {
                            std::cout << num << ", ";
                        }

                        if (sr.v.empty()) {
                            std::cout << "\t(empty)";
                        }
                        if (sr.is_finish) {
                            std::cout << "\t(finish)";
                        }
                        std::cout << "\n";
                        for (NState & st : sr.v) {
                            std::cout << "\t\033[33m";
                            if (st.is_range()) {
                                std::cout << utf8_char(st.l) << '-' << utf8_char(st.r) << "\033[0m\t";
                            }
                            else if (st.is_terminate()) {
                                std::cout << "(terminate)\033[0m\t";
                            }
                            else if (st.is_beginning()) {
                                std::cout << "(beginning)\033[0m\t";
                            }
                            else {
                                std::cout << "(finish)\033[0m\t";
                            }

                            for (uint id: st.nums) {
                                std::cout << id << ", ";
                            }
                            std::cout << "\n";
                        }
                    }
                    std::cout << ("--------\n");
                }
            }
        } display;

        display(srs);

        const size_t simple_srs_limit = srs.size();
        for (size_t ir = 0; ir != srs.size(); ++ir) {
            if (srs[ir].v.size() < 2) {
                continue;
            }
            size_t ilast;
            auto r = [&srs, ir](size_t i) -> char_int & { return srs[ir].v[i].r; };
            auto l = [&srs, ir](size_t i) -> char_int & { return srs[ir].v[i].l; };
            auto rmlast = [&srs, ir, &ilast](size_t i) {
                const size_t vli = srs[ir].v.size() - 1;
                if (i < vli) {
                    srs[ir].v[i] = std::move(srs[ir].v.back());
                }
                if (ilast > vli) {
                    --ilast;
                }
                srs[ir].v.pop_back();
            };
            auto addnext = [&srs, ir](size_t i, size_t i2) {
                std::vector<unsigned> & v1 = srs[ir].v[i].nums;
                std::vector<unsigned> & v2 = srs[ir].v[i2].nums;

                std::vector<unsigned> nums(v1.size() + v2.size());
                nums.erase(unique_merge(v1.begin(), v1.end(), v2.begin(), v2.end(), nums.begin()),
                           nums.end());
                std::swap(nums, v1);
            };
            for (size_t i1 = 0; i1 < srs[ir].v.size(); ++i1) {
                if (!srs[ir].v[i1].is_range()) {
                    continue ;
                }
                ilast = srs[ir].v.size();
                for (size_t i2 = i1+1; i2 < ilast; ++i2) {
                    if (!srs[ir].v[i2].is_range()
                    || (r(i1) < l(i2) && l(i1) < l(i2))
                    || (r(i2) < l(i1) && l(i2) < l(i1))) {
                        continue;
                    }
                    // [l1 r1]
                    // [l2   r2]
                    else if (l(i1) == l(i2) && r(i1) < r(i2)) {
                        //std::cout << "= 1] 2]\n";
                        l(i2) = r(i1) + 1;
                        addnext(i1, i2);
                    }
                    // [l2 r2]
                    // [l1   r1]
                    else if (l(i2) == l(i1) && r(i2) < r(i1)) {
                        //std::cout << "= 2] 1]\n";
                        l(i1) = r(i2) + 1;
                        addnext(i2, i1);
                    }
                    //   [l1 r1]
                    // [l2   r2]
                    else if (r(i1) == r(i2) && l(i2) < l(i1)) {
                        //std::cout << "[2 [1 =\n";
                        r(i2) = l(i1) - 1;
                        addnext(i1, i2);
                    }
                    //   [l2 r2]
                    // [l1   r1]
                    else if (r(i2) == r(i1) && l(i1) < l(i2)) {
                        //std::cout << "[1 [2 =\n";
                        r(i1) = l(i2) - 1;
                        addnext(i2, i1);
                    }
                    // [l1 r1]
                    // [l2 r2]
                    else if (l(i1) == l(i2) && r(i1) == r(i2)) {
                        //std::cout << "=\n";
                        addnext(i1, i2);
                        rmlast(i2);
                        --i2;
                    }
                    // [l1 r1][l2 r2]
                    else if (r(i2) == l(i1)){
                        //std::cout << "[1 = 2]\n";
                        // a [a-c]
                        if (l(i1) == l(i2)) {
                            ++l(i2);
                            addnext(i1, i2);
                        }
                        // [a-c] c
                        else if (r(i1) == r(i2)) {
                            --r(i1);
                            addnext(i2, i1);
                        }
                        // [a-b][b-a]
                        else {
                            srs[ir].v.push_back({srs[ir].v[i2].nums, nullptr, l(i2)+1, r(i2)});
                            --r(i1);
                            ++l(i2);
                            addnext(i2, i1);
                        }
                    }
                    // [l2 r2][l1 r1]
                    else if (r(i1) == l(i2)){
                        //std::cout << "[2 = 1]\n";
                        // a [a-c]
                        if (l(i2) == l(i1)) {
                            ++l(i1);
                            addnext(i2, i1);
                        }
                        // [a-c] c
                        else if (r(i2) == r(i1)) {
                            --r(i2);
                            addnext(i1, i2);
                        }
                        // [a-b][b-a]
                        else {
                            srs[ir].v.push_back({srs[ir].v[i1].nums, nullptr, l(i1)+1, r(i1)});
                            --r(i2);
                            ++l(i1);
                            addnext(i1, i2);
                        }
                    }
                    // [l1 r1]
                    //   [l2 r2]
                    else if (l(i1) < l(i2) && r(i1) < r(i2) && l(i2) > r(i1)) {
                        //std::cout << "[1 [2 1] 2]\n";
                        srs[ir].v.push_back({srs[ir].v[i2].nums, nullptr, r(i1)+1, r(i2)});
                        r(i1) = l(i2) - 1;
                        r(i2) = r(i1);
                        addnext(i2, i1);
                    }
                    // [l2 r2]
                    //   [l1 r1]
                    else if (l(i2) < l(i1) && r(i2) < r(i1) && l(i1) > r(i2)) {
                        //std::cout << "[2 [1 2] 1]\n";
                        srs[ir].v.push_back({srs[ir].v[i1].nums, nullptr, r(i2)+1, r(i1)});
                        r(i2) = l(i1) - 1;
                        r(i1) = r(i2);
                        addnext(i1, i2);
                    }
                    // [l1   r1]
                    //  [l2 r2]
                    else if (l(i1) < l(i2) && r(i1) > r(i2)) {
                        //std::cout << "[1 [2 2] 1]\n";
                        srs[ir].v.push_back({srs[ir].v[i1].nums, nullptr, r(i2)+1, r(i1)});
                        r(i1) = l(i2) - 1;
                        addnext(i2, i1);
                    }
                    // [l2   r2]
                    //  [l1 r1]
                    else if (l(i1) > l(i2) && r(i1) < r(i2)) {
                        //std::cout << "[2 [1 1] 2]\n";
                        srs[ir].v.push_back({srs[ir].v[i2].nums, nullptr, r(i1)+1, r(i2)});
                        r(i2) = l(i1) - 1;
                        addnext(i1, i2);
                    }
                }
            }

            for (size_t i = 0; i < srs[ir].v.size(); ++i) {
                NState & nsts = srs[ir].v[i];
                if (std::none_of(srs.begin(), srs.end(), [nsts](const StateRange & sr) {
                    return sr.stg == nsts.nums;
                })) {
                    srs.push_back({nsts.nums, {}, false});
                    auto & v = srs.back().v;
                    bool & is_finish = srs.back().is_finish;
                    for (unsigned id: nsts.nums) {
                        auto it = std::find_if(srs.begin(), srs.begin() + simple_srs_limit
                        , [id](const StateRange & sr) {
                            return sr.stg[0] == id;
                        });
                        is_finish |= it->is_finish || it->v.empty();
                        v.insert(v.end(), it->v.begin(), it->v.end());
                    }
                }
            }

            display(srs);
        }

        //TODO erase unused StateRange
        {
            srs.erase(std::remove_if(srs.begin()+1, srs.end(), [srs](const StateRange & sr) -> bool {
                for (const StateRange & sr2: srs) {
                    for (const NState & nst : sr2.v) {
                        if (sr.stg == nst.nums) {
                            return false;
                        }
                    }
                }
                return true;
            }), srs.end());

            RE_SHOW(std::cout << "##erase\n--------\n");
            display(srs);
        }

        for (StateRange & sr: srs) {
            for (NState & nst: sr.v) {
                nst.sr = &*std::find_if(srs.begin(), srs.end(), [nst](const StateRange & sr) {
                    return sr.stg == nst.nums;
                });
            }
        }

        for (StateRange & sr: srs) {
            for (NState & nst: sr.v) {
                if (nst.is_terminate() && nullptr != nst.sr && 0 != nst.sr->v.size()) {
                    nst.r = ~char_int(0) - 1;
                    nst.l = ~char_int(0);
                    nst.sr = &*srs.end();
                }
            }
        }

        {
            size_t count_nst = 0;
            for (StateRange& sr: srs) {
                count_nst += sr.v.size();
            }
            const size_t byte_srs = srs.size() * sizeof(StateRangeSearch);
            const size_t byte_st = count_nst * sizeof(NStateSearch);
            const size_t byte_b = byte_srs + (byte_srs % sizeof(NStateSearch) ?  sizeof(NStateSearch) : 0);
            srss = static_cast<StateRangeSearch*>(::operator new(byte_b + byte_st));
            NStateSearch * pstss = reinterpret_cast<NStateSearch*>(reinterpret_cast<char*>(srss) + byte_b);
            StateRangeSearch * csrss = srss;
            for (StateRange& sr: srs) {
                csrss->v = sr.v.empty() ? nullptr : pstss;
                csrss->is_finish = sr.is_finish;
                csrss->sz = sr.v.size();
                for (NState& st: sr.v) {
                    pstss->l = st.l;
                    pstss->r = st.r;
                    pstss->sr = srss + (st.sr - srs.data());
                    ++pstss;
                }
                ++csrss;
            }
        }
        nb_states = srs.size();

        if (sr_beg && sr_first) {
            s_sr_beg = srss + 0;
            s_sr_first = srss + 1;
        }
        else if (sr_beg) {
            s_sr_beg = srss + 0;
        }
        else if (sr_first) {
            s_sr_first = srss + 0;
            s_sr_beg = srss + 0;
        }

        if (g_trace_active) {
            std::cout <<
                "yes_beg: " << (yes_beg) << "\n"
                "yes_finish: " << (yes_finish) << "\n"
                "-----------\n"
            ;
        }
    }

    StateMachine2(const StateMachine2 & other)
    : srss(0)
    , s_sr_beg(0)
    , s_sr_first(0)
    , nb_states(other.nb_states)
    , yes_beg(other.yes_beg)
    , yes_finish(other.yes_finish)
    {
        size_t count_nst = 0;
        StateRangeSearch * first = other.srss;
        StateRangeSearch * last = first + this->nb_states;
        for (; first != last; ++first) {
            count_nst += first->sz;
        }

        const size_t byte_srs = this->nb_states * sizeof(StateRangeSearch);
        const size_t byte_st = count_nst * sizeof(NStateSearch);
        const size_t byte_b = byte_srs + (byte_srs % sizeof(NStateSearch) ?  sizeof(NStateSearch) : 0);
        this->srss = static_cast<StateRangeSearch*>(::operator new(byte_b + byte_st));
        NStateSearch * pstss = reinterpret_cast<NStateSearch*>(reinterpret_cast<char*>(srss) + byte_b);
        StateRangeSearch * csrss = srss;

        first = other.srss;
        last = first + this->nb_states;
        for (; first != last; ++first) {
            StateRangeSearch & sr = *first;
            csrss->v = sr.sz ? pstss : nullptr;
            csrss->is_finish = sr.is_finish;
            csrss->sz = sr.sz;
            NStateSearch * first2 = sr.v;
            NStateSearch * last2 = sr.v + sr.sz;
            for (; first2 != last2; ++first2) {
                pstss->l = first2->l;
                pstss->r = first2->r;
                pstss->sr = srss + (first2->sr - other.srss);
                ++pstss;
            }
            ++csrss;
        }
    }

#if __cplusplus >= 201103L && __cplusplus != 1 || defined(__GXX_EXPERIMENTAL_CXX0X__)
    StateMachine2(StateMachine2 && other) noexcept
    {
        this->srss = other.srss;
        this->s_sr_beg = other.s_sr_beg;
        this->s_sr_first = other.s_sr_first;
        this->nb_states = other.nb_states;
        this->yes_beg = other.yes_beg;
        this->yes_finish = other.yes_finish;
        other.srss = nullptr;
        other.s_sr_beg = nullptr;
        other.s_sr_first = nullptr;
        other.nb_states = 0;
        other.yes_beg = 0;
        other.yes_finish = 0;
    }
#endif

    ~StateMachine2()
    {
        ::operator delete(srss);
    }

    unsigned mark_count() const
    {
        return 0;
    }

public:
    typedef std::pair<const char *, const char *> range_t;
    typedef std::vector<range_t> range_matches;

    struct DefaultMatchTracer
    {
        bool operator()(unsigned /*idx*/, const char *) const
        { return true; }
    };

public:
    bool exact_search(const char * s, unsigned /*step_limit*/, size_t * pos = 0)
    {
        return this->exact_search_impl(s, pos);
    }

    bool exact_search_with_trace(const char * s, unsigned /*step_limit*/, size_t * pos = 0)
    {
        return exact_search_impl(s, pos);
    }

    template<typename Tracer>
    bool exact_search_with_trace(const char * s, unsigned /*step_limit*/, Tracer /*tracer*/, size_t * pos = 0)
    {
        return exact_search_impl(s, pos);
    }

    bool search(const char * s, unsigned /*step_limit*/, size_t * pos = 0)
    {
        return this->search_impl(s, pos);
    }

    bool search_with_trace(const char * s, unsigned /*step_limit*/, size_t * pos = 0)
    {
        return search_impl(s, pos);
    }

    template<typename Tracer>
    bool search_with_trace(const char * s, unsigned /*step_limit*/, Tracer /*tracer*/, size_t * pos = 0)
    {
        return search_impl(s, pos);
    }

    range_matches match_result(bool /*all*/ = true) const
    {
        return range_matches();
    }

    void append_match_result(range_matches& /*ranges*/, bool /*all*/ = true) const
    {
    }
};
}

#endif
