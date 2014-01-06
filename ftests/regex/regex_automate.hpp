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
#include <iomanip>
#include <cstring> //memset
#include <cassert>
#include <stdint.h>

#include "regex_utils.hpp"

namespace re {
#ifdef DISPLAY_TRACE
    enum { g_trace_active = 1 };
#else
    enum { g_trace_active = 0 };
#endif

#define RE_SHOW !::re::g_trace_active ? void() : void

class StateMachine2
{
    StateMachine2(const StateMachine2&) /*= delete*/;

    class StateRange;

    struct NState {
        std::vector<uint> nums;
        StateRange * sr;
        char_int l;
        char_int r;

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
        //std::vector<unsigned> caps;
        bool is_finish;
        unsigned step;
        unsigned num;
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

                if (st->type == FIRST) {
                    return false;
                }
                else if (st->is_range()) {
                    v.push_back({{st->num}, nullptr, st->l, st->r});
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

                if (st->is_range()) {
                    v.push_back({{st->num}, nullptr, st->l, st->r});
                }
                else if (st->type == LAST) {
                    v.push_back({{st->num}, nullptr, 1, 0});
                }
                else if (st->type == FIRST) {
                    v.push_back({{st->num}, nullptr, 2, 0});
                    return run(v, st->out1);
                }
                else if (st->is_split()) {
                    bool ret = run(v, st->out1);
                    return run(v, st->out2) || ret;
                }
                else {
                    return run(v, st->out1);
                }

                return false;
            }
        };

        return Impl::run(v, st);
    }

    const State * root;
    unsigned nb_states;
    mutable std::vector<StateRange> srs;
    StateRange * sr_beg;
    StateRange * sr_first;
    StateRangeSearch * s_sr_beg;
    StateRangeSearch * s_sr_first;
    bool yes_beg;
    bool yes_finish;

    StateRangeSearch * srss;
    size_t srs_size;

    template<bool> struct ExactMatch { };

public:
    bool exact_search_impl(const char * s) const
    {
        return exact_search_impl(s, ExactMatch<true>());
    }

    template<bool exact_match>
    bool exact_search_impl(const char * s, ExactMatch<exact_match>) const
    {
        //std::cout << "exact_search (exact = " << exact_match << ")\n";
        //std::cout << "sr_beg: " << s_sr_beg << ", sr_first: " << s_sr_first << "\n";

        if (yes_finish) {
            RE_SHOW(std::cout << "yes_finish\n");
            return true;
        }

        if (!s_sr_beg) {
            RE_SHOW(std::cout << "!s_sr_beg\n");
            return yes_beg;
        }

        if (!s_sr_beg->v) {
            RE_SHOW(std::cout << "!s_sr_beg->v\n");
            return true;
        }

        if (s_sr_beg->sz == 1 && s_sr_beg->v->is_terminate()) {
            if (yes_beg) {
                RE_SHOW(std::cout << "is ^$");
                return !*s;
            }
            else {
                RE_SHOW(std::cout << "is_terminate\n");
                return true;
            }
        }

        const StateRangeSearch * sr = s_sr_beg;
        utf8_consumer consumer(s);

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

    bool search_impl(const char * s) const
    {
        //std::cout << "search\n";
        if (!s_sr_first) {
            RE_SHOW(std::cout << "call exact_search\n");
            return exact_search_impl(s, ExactMatch<false>());
        }

        if (yes_finish) {
            RE_SHOW(std::cout << "yes_finish\n");
            return true;
        }

        if (!s_sr_beg) {
            RE_SHOW(std::cout << "!s_sr_beg\n");
            return yes_beg;
        }

        if (!s_sr_beg->v) {
            RE_SHOW(std::cout << "!s_sr_beg->v\n");
            return true;
        }

        if (s_sr_beg->sz == 1 && s_sr_beg->v->is_terminate()) {
            if (yes_beg) {
                RE_SHOW(std::cout << "is ^$");
                return !*s;
            }
            else {
                RE_SHOW(std::cout << "is_terminate\n");
                return true;
            }
        }

        if (!srs_size || !srss->v) {
            return true;
        }

        for (size_t i = 0; i < srs_size; ++i) {
            srss[i].step = 0;
        }

        unsigned step = 0;
        std::vector<StateRangeSearch*> srs1(1, s_sr_beg) /*({s_sr_beg})*/;
        std::vector<StateRangeSearch*> srs2;
        srs1.reserve(srs_size);
        srs2.reserve(srs_size);

        if (!s_sr_beg->v || s_sr_beg->is_finish) {
            RE_SHOW(std::cout << ("first is finish") << std::endl);
            return true;
        }

        utf8_consumer consumer(s);
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

private:
    void new_init(const state_list_t & sts)
    {
        srs.push_back({{-1u}, {}, false});
        {
            StateRange & sr = srs.back();
            if (add_beginning(sr.v, this->root)) {
                yes_finish = true;
                sr.is_finish = true;
            }
            if (!sr.v.empty()) {
                yes_beg = true;
                srs.push_back({{}, {}, false});
                this->sr_beg = &sr;
            }
        }
        {
            StateRange & sr = srs.back();
            if (add_first(sr.v, this->root)) {
                yes_finish = true;
                sr.is_finish = true;
            }
            if (!sr.v.empty()) {
                this->sr_first = &sr;
            }
            else {
                srs.pop_back();
            }
        }

        if (srs.empty()) {
            return ;
        }

        const size_t first_range_pos = sr_first && sr_beg ? 2 : 1;

        //insert
        for (State * st: sts) {
            if (st->is_range() || st->type == LAST) {
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
                auto it = std::find_if(srs.begin(), srs.begin()+i, [csr](StateRange& sr){
                    if (sr.is_finish == csr.is_finish && sr.v.size() == csr.v.size()) {
                        return std::equal(sr.v.begin(), sr.v.end(), csr.v.begin(),
                                          [](const NState & a, const NState & b) {
                                            return a.l == b.l && a.r == b.r && a.nums == b.nums;
                                          });
                    }
                    return false;
                });
                if (it != srs.begin()+i) {
                    const unsigned new_id = it->stg[0];
                    const unsigned old_id = csr.stg[0];
                    RE_SHOW(std::cout << "old_id: " << old_id << "\nnew_id: " << new_id << std::endl);
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
                        if (sr.stg.empty()) { //only first
                            std::cout << ("range: nil");
                        }
                        else {
                            std::cout << ("range: ");
                            for (unsigned num: sr.stg) {
                                std::cout << num << ", ";
                            }
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
            auto r = [this, ir](size_t i) -> char_int & { return srs[ir].v[i].r; };
            auto l = [this, ir](size_t i) -> char_int & { return srs[ir].v[i].l; };
            auto rmlast = [this, ir, &ilast](size_t i) {
                const size_t vli = srs[ir].v.size() - 1;
                if (i < vli) {
                    srs[ir].v[i] = std::move(srs[ir].v.back());
                }
                if (ilast > vli) {
                    --ilast;
                }
                srs[ir].v.pop_back();
            };
            auto addnext = [this, ir](size_t i, size_t i2) {
                auto & v = srs[ir].v[i].nums;
                auto & v2 = srs[ir].v[i2].nums;

                std::vector<unsigned> nums(v.size() + v2.size());

                auto first1 = v.begin();
                auto last1 = v.end();
                auto first2 = v2.begin();
                auto last2 = v2.end();
                auto d_first = nums.begin();
                unsigned n = -2u;

                while (first1 != last1) {
                    if (first2 == last2) {
                        while (first1 != last1 && n == *first1) {
                            ++first1;
                        }
                        d_first = std::unique_copy(first1, last1, d_first);
                        break ;
                    }
                    if (*first2 < *first1) {
                        if (n != *first2) {
                            n = *d_first = *first2;
                            ++d_first;
                        }
                        ++first2;
                    } else {
                        if (n != *first1) {
                            n = *d_first = *first1;
                            ++d_first;
                        }
                        ++first1;
                    }
                }
                while (first2 != last2 && n == *first2) {
                    ++first2;
                }
                d_first = std::unique_copy(first2, last2, d_first);
                nums.erase(d_first, nums.end());

                std::swap(nums, v);
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
                        auto it = std::find_if(srs.begin()+first_range_pos, srs.begin() + simple_srs_limit
                        , [id](const StateRange & sr) {
                            return sr.stg[0] == id;
                        });
                        v.insert(v.end(), it->v.begin(), it->v.end());
                        is_finish |= it->is_finish || it->v.empty();
                    }
                }
            }

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

        //TODO erase unused StateRange

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
        srs_size = srs.size();

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
    }

public:
    StateMachine2(const state_list_t & sts, const State * root, unsigned /*nb_capture*/,
                bool /*copy_states*/ = false, bool /*minimal_mem*/ = false)
    : root(root)
    , nb_states(sts.size())
    , sr_beg(0)
    , sr_first(0)
    , s_sr_beg(0)
    , s_sr_first(0)
    , yes_beg(false)
    , yes_finish(false)
    , srss(0)
    , srs_size(0)
    {
        if (sts.size()) {
            new_init(sts);
        }
        if (g_trace_active) {
            std::cout <<
                "yes_beg: " << (yes_beg) << "\n"
                "yes_finish: " << (yes_finish) << "\n"
                "-----------\n"
            ;
        }
    }

#if __cplusplus >= 201103L && __cplusplus != 1 || defined(__GXX_EXPERIMENTAL_CXX0X__)
    StateMachine2(StateMachine2 && /*other*/) noexcept
    {
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
    bool exact_search(const char * s, unsigned /*step_limit*/, size_t * /*pos*/ = 0)
    {
        if (0 == this->nb_states) {
            return !*s;
        }
        return this->exact_search_impl(s);
    }

    bool exact_search_with_trace(const char * s, unsigned /*step_limit*/, size_t * /*pos*/ = 0)
    {
        return exact_search_impl(s);
    }

    template<typename Tracer>
    bool exact_search_with_trace(const char * s, unsigned /*step_limit*/, Tracer /*tracer*/, size_t * /*pos*/ = 0)
    {
        return exact_search_impl(s);
    }

    bool search(const char * s, unsigned /*step_limit*/, size_t * /*pos*/ = 0)
    {
        if (0 == this->nb_states) {
            return !*s;
        }
        return this->search_impl(s);
    }

    bool search_with_trace(const char * s, unsigned /*step_limit*/, size_t * /*pos*/ = 0)
    {
        return search_impl(s);
    }

    template<typename Tracer>
    bool search_with_trace(const char * s, unsigned /*step_limit*/, Tracer /*tracer*/, size_t * /*pos*/ = 0)
    {
        return search_impl(s);
    }

    range_matches match_result(bool /*all*/ = true) const
    {
        range_matches ret;
        return ret;
    }

    void append_match_result(range_matches& /*ranges*/, bool /*all*/ = true) const
    {
    }
};
}

#endif
