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


// TODO rename to deterministic finite automaton ?
class StateMachine2
{
    class StateRange;

    struct NState {
        unsigned id_nums;
        StateRange * sr;
        char_int l;
        char_int r;

        bool operator==(NState const other) const
        {
            return other.id_nums == this->id_nums
                && other.l == this->l
                && other.r == this->r;
        }

        bool operator<(NState const other) const
        {
          if (this->l != other.l) {
            return this->l < other.l;
          }
          if (this->r != other.r) {
            return this->r < other.r;
          }
          return this->id_nums < other.id_nums;
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
        unsigned id_nums;
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

public:
    class NStateSearch;

    struct StateRangeSearch
    {
        NStateSearch const * v;
        size_t sz;
        bool is_finish;
    };

    struct NStateSearch
    {
        StateRangeSearch const * sr;
        char_int l;
        char_int r;

        bool is_range() const
        { return !(l > r); }

        bool is_terminate() const
        { return 0 == r && 1 == l; }

        bool is_beginning() const
        { return 0 == r && 2 == l; }
    };

private:
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
                    v.push_back({st->num, nullptr, st->l, st->r});
                }
                else if (st->type == LAST) {
                    v.push_back({st->num, nullptr, 1, 0});
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
                    v.push_back({st->num, nullptr, st->l, st->r});
                }
                else if (st->type == FIRST) {
                    return false;
                }
                else if (st->type == LAST) {
                    v.push_back({st->num, nullptr, 1, 0});
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
                    v.push_back({st->num, nullptr, st->l, st->r});
                }
                else if (st->type == LAST) {
                    v.push_back({st->num, nullptr, 1, 0});
                }
                else if (st->type == FIRST) {
                    v.push_back({st->num, nullptr, 2, 0});
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

    template<typename Traits, typename C>
    void init_srss(C & cont)
    {
        size_t count_nst = 0;
        typedef typename Traits::iterator_range iterator_range;
        iterator_range first = Traits::state_range_begin(cont);
        iterator_range last = Traits::state_range_end(cont);
        for (; first != last; ++first) {
            count_nst += Traits::count_states(*first);
        }

        const size_t byte_srs = this->nb_states * sizeof(StateRangeSearch);
        const size_t byte_st = count_nst * sizeof(NStateSearch);
        const size_t byte_b = byte_srs + (byte_srs % sizeof(NStateSearch) ?  sizeof(NStateSearch) : 0);
        this->srss = static_cast<StateRangeSearch*>(::operator new(byte_b + byte_st));
        NStateSearch * pstss = reinterpret_cast<NStateSearch*>(reinterpret_cast<char*>(srss) + byte_b);
        StateRangeSearch * csrss = srss;

        for (first = Traits::state_range_begin(cont); first != last; ++first) {
            csrss->v = Traits::count_states(*first) ? pstss : nullptr;
            csrss->is_finish = first->is_finish;
            csrss->sz = Traits::count_states(*first);
            typedef typename Traits::iterator_state iterator_state;
            iterator_state first2 = Traits::nstate_begin(*first);
            iterator_state last2 = Traits::nstate_end(*first);
            for (; first2 != last2; ++first2) {
                pstss->l = first2->l;
                pstss->r = first2->r;
                pstss->sr = this->srss + (first2->sr - Traits::data(cont));
                ++pstss;
            }
            ++csrss;
        }
    }

    StateRangeSearch * srss;
    StateRangeSearch * s_sr_beg;
    StateRangeSearch * s_sr_first;
    size_t nb_states;
    bool has_bol;
    bool has_eol;

public:
    StateMachine2(const state_list_t & sts, const State * root, unsigned /*nb_capture*/,
                bool /*copy_states*/ = false, bool /*minimal_mem*/ = false)
    : srss(0)
    , s_sr_beg(0)
    , s_sr_first(0)
    , nb_states(0)
    , has_bol(false)
    , has_eol(false)
    {
        if (!sts.size()) {
            RE_SHOW(std::cout << "sts is empty\n");
            return ;
        }

        // ligne (max): compound * 1.5 + 1 + simple
        // group (max): compound * 3 + 2 + simple * 2 ?

        std::vector<StateRange> srs;
        StateRange * sr_beg = nullptr;
        StateRange * sr_first = nullptr;

        srs.push_back({-1u, {}, false});
        {
            StateRange & sr = srs.back();
            if (add_beginning(sr.v, root)) {
                this->has_eol = true;
                return ;
            }
            if (!sr.v.empty()) {
                has_bol = true;
                srs.push_back({-2u, {}, false});
                sr_beg = &sr;
            }
        }
        {
            StateRange & sr = srs.back();
            if (add_first(sr.v, root)) {
                this->has_eol = true;
                return ;
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
            if (st->is_range() || st->type == LAST || st->type == FIRST) {
                srs.push_back({st->num, {}, false});
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
                    const unsigned new_id = it->id_nums;
                    const unsigned old_id = csr.id_nums;
                    RE_SHOW(std::cout << "old_id: " << old_id << "\tnew_id: " << new_id << std::endl);
                    for (StateRange & sr: srs) {
                        for (NState & nst: sr.v) {
                            if (nst.id_nums == old_id) {
                                nst.id_nums = new_id;
                            }
                        }
                    }
                    srs.erase(srs.begin()+i);
                }
            }
        }

        std::vector<std::vector<unsigned> > nums;

        //re-index
        {
          std::vector<uint> indexes(srs.size());
          auto first = srs.begin();
          std::generate(indexes.begin(), indexes.end(), [&]{
            uint ret = first->id_nums;
            ++first;
            return ret;
          });

          auto newindex = [indexes](uint old){
            return std::find(indexes.begin(), indexes.end(), old) - indexes.begin();
          };

          for (StateRange & sr: srs) {
            sr.id_nums = newindex(sr.id_nums);
            for (NState & nst: sr.v) {
              nst.id_nums = newindex(nst.id_nums);
            }
          }

          nums.resize(indexes.size());
          for (uint i = 0; i < indexes.size(); ++i) {
            nums[i].push_back(i);
          }
        }

        RE_SHOW(std::cout << "reindex:\n");

        struct Display {
            std::vector<std::vector<unsigned> > const & ref_nums;

            void operator()(std::vector<StateRange> & srs) const {
                if (g_trace_active) {
                    for (StateRange & sr : srs) {
                        std::cout << ("ranges: ");
                        for (unsigned id: ref_nums[sr.id_nums]) {
                            std::cout << id << ", ";
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

                            for (unsigned id: ref_nums[st.id_nums]) {
                                std::cout << id << ", ";
                            }
                            std::cout << "\n";
                        }
                    }
                    std::cout << ("--------\n");
                }
            }
        } display{nums};

        display(srs);

        std::vector<unsigned> num;

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
            auto addnext = [&srs, ir, &nums, &num](size_t i, size_t i2) {
                std::vector<unsigned> & v1 = nums[srs[ir].v[i].id_nums];
                std::vector<unsigned> & v2 = nums[srs[ir].v[i2].id_nums];

                num.resize(v1.size() + v2.size());
                num.erase(unique_merge(v1.begin(), v1.end(), v2.begin(), v2.end(), num.begin()), num.end());

                auto it = std::find(nums.begin(), nums.end(), num);
                if (it == nums.end()) {
                    srs[ir].v[i].id_nums = nums.size();
                    srs.push_back({uint(nums.size()), {}, false});
                    auto & v = srs.back().v;
                    bool & is_finish = srs.back().is_finish;
                    for (unsigned id: num) {
                        StateRange & sr = srs[id];
                        is_finish |= sr.is_finish || sr.v.empty();
                        v.insert(v.end(), sr.v.begin(), sr.v.end());
                    }
                    nums.push_back(std::move(num));
                }
                else {
                  srs[ir].v[i].id_nums = it - nums.begin();
                }
            };
            for (size_t i1 = 0; i1 < srs[ir].v.size(); ++i1) {
                ilast = srs[ir].v.size();
                if (!srs[ir].v[i1].is_range()) {
                    for (size_t i2 = i1+1; i2 < ilast; ++i2) {
                        if (l(i1) == l(i2) && r(i1) == r(i2)) {
                            //std::cout << "=\n";
                            addnext(i1, i2);
                            rmlast(i2);
                            --i2;
                        }
                    }
                    continue ;
                }
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
                            srs[ir].v.push_back({srs[ir].v[i2].id_nums, nullptr, l(i2)+1, r(i2)});
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
                            srs[ir].v.push_back({srs[ir].v[i1].id_nums, nullptr, l(i1)+1, r(i1)});
                            --r(i2);
                            ++l(i1);
                            addnext(i1, i2);
                        }
                    }
                    // [l1 r1]
                    //   [l2 r2]
                    else if (l(i1) < l(i2) && r(i1) < r(i2) && l(i2) > r(i1)) {
                        //std::cout << "[1 [2 1] 2]\n";
                        srs[ir].v.push_back({srs[ir].v[i2].id_nums, nullptr, r(i1)+1, r(i2)});
                        r(i1) = l(i2) - 1;
                        r(i2) = r(i1);
                        addnext(i2, i1);
                    }
                    // [l2 r2]
                    //   [l1 r1]
                    else if (l(i2) < l(i1) && r(i2) < r(i1) && l(i1) > r(i2)) {
                        //std::cout << "[2 [1 2] 1]\n";
                        srs[ir].v.push_back({srs[ir].v[i1].id_nums, nullptr, r(i2)+1, r(i1)});
                        r(i2) = l(i1) - 1;
                        r(i1) = r(i2);
                        addnext(i1, i2);
                    }
                    // [l1   r1]
                    //  [l2 r2]
                    else if (l(i1) < l(i2) && r(i1) > r(i2)) {
                        //std::cout << "[1 [2 2] 1]\n";
                        srs[ir].v.push_back({srs[ir].v[i1].id_nums, nullptr, r(i2)+1, r(i1)});
                        r(i1) = l(i2) - 1;
                        addnext(i2, i1);
                    }
                    // [l2   r2]
                    //  [l1 r1]
                    else if (l(i1) > l(i2) && r(i1) < r(i2)) {
                        //std::cout << "[2 [1 1] 2]\n";
                        srs[ir].v.push_back({srs[ir].v[i2].id_nums, nullptr, r(i1)+1, r(i2)});
                        r(i2) = l(i1) - 1;
                        addnext(i1, i2);
                    }
                }
            }

            display(srs);
        }

        // erase unused StateRange
        /*if (minimal_memory)*/ {
            size_t sz = srs.size();
            do {
                sz = srs.size();
                srs.erase(std::remove_if(srs.begin()+1, srs.end(), [srs](const StateRange & sr) -> bool {
                    for (const StateRange & sr2: srs) {
                        for (const NState & nst : sr2.v) {
                            if (sr.id_nums == nst.id_nums) {
                                return false;
                            }
                        }
                    }
                    return true;
                }), srs.end());

                RE_SHOW(std::cout << "##erase\n--------\n");
                display(srs);
            } while (sz != srs.size());
        }

        for (StateRange & sr: srs) {
            for (NState & nst: sr.v) {
                nst.sr = &*std::find_if(srs.begin(), srs.end(), [nst](const StateRange & sr) {
                    return sr.id_nums == nst.id_nums;
                });
            }
        }

        struct traits {
            typedef std::vector<StateRange> container_range_type;
            typedef container_range_type::const_iterator iterator_range;
            typedef std::vector<NState> container_state_type;
            typedef container_state_type::const_iterator iterator_state;

            static iterator_range state_range_begin(const std::vector<StateRange> & srs)
            { return srs.begin(); }

            static iterator_range state_range_end(const std::vector<StateRange> & srs)
            { return srs.end(); }

            static const StateRange * data(const std::vector<StateRange> & srs)
            { return srs.data(); }

            static iterator_state nstate_begin(const StateRange & sr)
            { return sr.v.begin(); }

            static iterator_state nstate_end(const StateRange & sr)
            { return sr.v.end(); }

            static size_t count_states(const StateRange & sr)
            { return sr.v.size(); }
        };
        this->nb_states = srs.size();
        this->init_srss<traits>(srs);

        {
            const StateRangeSearch * sr = this->srss;
            while (sr) {
                const NStateSearch * first = sr->v;
                const NStateSearch * last = first + sr->sz;
                bool bol = false;
                bool eol = false;
                for (; first != last && !(bol && eol); ++first) {
                    if (first->is_beginning()) {
                        bol = true;
                        sr = first->sr;
                        this->has_bol = true;
                    }
                    if (first->is_terminate()) {
                        eol = true;
                        sr = first->sr;
                        this->has_eol = true;
                    }
                }

                if (sr->is_finish || !(bol || eol)) {
                    break;
                }
            }

            if (!sr || !sr->is_finish) {
                this->has_bol = false;
                this->has_eol = false;
            }
        }

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
                "has_bol: " << (has_bol) << "\n"
                "has_eol: " << (has_eol) << "\n"
                "-----------\n"
            ;
        }
    }

    StateMachine2(const StateMachine2 & other)
    : srss(0)
    , s_sr_beg(0)
    , s_sr_first(0)
    , nb_states(other.nb_states)
    , has_bol(other.has_bol)
    , has_eol(other.has_eol)
    {
        struct traits {
            typedef StateRangeSearch const * iterator_range;
            typedef NStateSearch const * iterator_state;

            static iterator_range state_range_begin(const StateMachine2 & sm)
            { return sm.srss; }

            static iterator_range state_range_end(const StateMachine2 & sm)
            { return sm.srss + sm.nb_states; }

            static StateRangeSearch const * data(const StateMachine2 & sm)
            { return sm.srss; }

            static iterator_state nstate_begin(const StateRangeSearch & sr)
            { return sr.v; }

            static iterator_state nstate_end(const StateRangeSearch & sr)
            { return sr.v + sr.sz; }

            static size_t count_states(const StateRangeSearch & sr)
            { return sr.sz; }
        };

        this->init_srss<traits>(other);

        this->s_sr_beg = this->srss + (other.srss - other.s_sr_beg);
        this->s_sr_first = this->srss + (other.srss - other.s_sr_first);
    }

#if __cplusplus >= 201103L && __cplusplus != 1 || defined(__GXX_EXPERIMENTAL_CXX0X__)
    StateMachine2(StateMachine2 && other) noexcept
    : srss(other.srss)
    , s_sr_beg(other.s_sr_beg)
    , s_sr_first(other.s_sr_first)
    , nb_states(other.nb_states)
    , has_bol(other.has_bol)
    , has_eol(other.has_eol)
    {
        other.srss = nullptr;
        other.s_sr_beg = nullptr;
        other.s_sr_first = nullptr;
        other.nb_states = 0;
        other.has_bol = 0;
    }
#endif

    ~StateMachine2()
    {
        ::operator delete(this->srss);
    }

    size_t count_states() const
    { return this->nb_states; }

    const StateRangeSearch * state() const
    { return this->srss; }

    const StateRangeSearch * begin() const
    { return this->srss; }

    const StateRangeSearch * end() const
    { return this->srss + this->nb_states; }

    bool bol() const
    { return this->has_bol; }

    bool eol() const
    { return this->has_eol; }

    const StateRangeSearch * sr_beg() const
    { return this->s_sr_beg; }

    const StateRangeSearch * sr_first() const
    { return this->s_sr_first; }
};

enum result_test_type { test_fail, test_ok, test_run, test_unknow };

namespace detail {
    typedef StateMachine2::StateRangeSearch StateRange;

    inline result_test_type test_start(const StateMachine2 & sm, const char * s)
    {
        if (sm.eol() && !sm.bol()) {
            RE_SHOW(std::cout << "has_eol && !has_bol\n");
            return test_ok;
        }

        if (0 == sm.count_states()) {
            return *s ? test_fail : test_ok;
        }

        if (!sm.sr_beg()) {
            RE_SHOW(std::cout << "!s_sr_beg\n");
            return sm.bol() ? test_ok : test_fail;
        }

        if (!sm.sr_beg()->v) {
            RE_SHOW(std::cout << "!s_sr_beg->v\n");
            return test_ok;
        }

        if (sm.sr_beg()->sz > 1 && sm.bol() && sm.eol() && !*s) {
            RE_SHOW(std::cout << "is ^$\n");
            return test_ok;
        }

        if (sm.sr_beg()->sz == 1 && sm.sr_beg()->v->is_terminate()) {
            if (sm.bol() && sm.eol()) {
                RE_SHOW(std::cout << "is ^$\n");
                return *s ? test_fail : test_ok;
            }
            else {
                RE_SHOW(std::cout << "is_terminate\n");
                return test_ok;
            }
        }

        return test_run;
    }

    inline bool is_valid_state_range(const StateRange * sr)
    {
        RE_SHOW(std::cout << "! st.v.empty" << std::endl);
        next_search:
        while (sr) {
            for (size_t i = 0; i < sr->sz; ++i) {
                if (sr->v[i].is_terminate()) {
                    RE_SHOW(std::cout << "is_terminate" << std::endl);
                    sr = sr->v[i].sr;
                    if (sr->is_finish) {
                        return true;
                    }
                    goto next_search;
                }
            }
            break ;
        }
        return false;
    }

    template<bool exact_match>
    inline result_test_type exact_test_impl(const StateRange * & sr, utf8_consumer & consumer)
    {
        next_char:
        while (consumer.valid()) {
            if (!sr->v) {
                RE_SHOW(std::cout << "sr->v = nullptr\n");
                return !exact_match ? test_ok : test_fail;
            }
            const char_int c = consumer.bumpc();
            RE_SHOW(std::cout << "\033[33m" << utf8_char(c) << "\033[0m\n");
            for (size_t i = 0; i < sr->sz; ++i) {
                const StateMachine2::NStateSearch & nst = sr->v[i];
                RE_SHOW(std::cout << "\t[" << utf8_char(nst.l) << "-" << utf8_char(nst.l) << "]\n");
                if (nst.l <= c && c <= nst.r) {
                    RE_SHOW(std::cout << "\t\tok\n");
                    sr = nst.sr;
                    goto next_char;
                }
            }
            RE_SHOW(std::cout << "none\n");
            return test_fail;
        }

        return test_run;
    }

    inline result_test_type search_impl(const StateRange * data,
                                 const StateRange * sr_first,
                                 std::vector<StateRange const *> & srs1,
                                 std::vector<StateRange const *> & srs2,
                                 std::vector<unsigned> & steps,
                                 unsigned & step,
                                 utf8_consumer & consumer)
    {
        while (consumer.valid()) {
            ++step;
            const char_int c = consumer.bumpc();
            RE_SHOW(std::cout << "\033[33m" << utf8_char(c) << "\033[0m\n");
            for (StateRange const * sr: srs1) {
                if (steps[sr - data] == step) {
                    continue ;
                }
                steps[sr - data] = step;
                for (size_t i = 0; i < sr->sz; ++i) {
                    const StateMachine2::NStateSearch & nst = sr->v[i];
                    RE_SHOW(std::cout << "\t[" << utf8_char(nst.l) << "-" << utf8_char(nst.l) << "]\n");
                    if (nst.l <= c && c <= nst.r) {
                        RE_SHOW(std::cout << "\t\tok\n");
                        srs2.push_back(nst.sr);
                        if (!nst.sr->v || nst.sr->is_finish) {
                            RE_SHOW(std::cout << ("finish") << std::endl);
                            return test_ok;
                        }
                        break;
                    }
                }
            }
            swap(srs1, srs2);
            srs1.push_back(sr_first);
            srs2.clear();
        }

        return test_run;
    }
}


template<bool exact_match>
class BasicExactTest
{
    detail::StateRange const * sr;
    utf8_consumer consumer;

public:
    BasicExactTest()
    : sr(0)
    , consumer(nullptr)
    {}

    result_test_type start(const StateMachine2 & sm, const char * s, size_t * pos = 0)
    {
        if (pos) {
            *pos = 0;
        }
        this->sr = sm.sr_beg();
        result_test_type ret = detail::test_start(sm, s);
        if (ret == test_run) {
            return this->next(s, pos);
        }
        return ret;
    }

    result_test_type next(const char * s, size_t * pos = 0)
    {
        this->consumer.str(s);
        result_test_type ret = detail::exact_test_impl<exact_match>(this->sr, this->consumer);
        if (pos) {
            *pos = this->consumer.str() - s;
        }
        return ret;
    }

    bool stop()
    {
        if (sr->v && !sr->is_finish) {
            if (detail::is_valid_state_range(sr)) {
                return true;
            }
        }
        return !sr->v || sr->is_finish;
    }
};

typedef BasicExactTest<true> ExactTest;


class SearchTest
{
    detail::StateRange const * data;
    detail::StateRange const * sr_first;
    utf8_consumer consumer;
    unsigned step;
    std::vector<unsigned> steps;
    std::vector<detail::StateRange const *> srs1;
    std::vector<detail::StateRange const *> srs2;

public:
    SearchTest()
    : consumer(nullptr)
    , step(0)
    {}

    result_test_type start(const StateMachine2 & sm, const char * s, size_t * pos = 0)
    {
        if (pos) {
            *pos = 0;
        }

        this->srs1.clear();
        this->srs1.reserve(sm.count_states());
        this->srs1.push_back(sm.sr_beg());
        result_test_type ret = detail::test_start(sm, s);

        if (ret == test_run)
        {
            if (!sm.count_states() || !sm.state()->v) {
                return test_ok;
            }

            if (!sm.sr_beg()->v || sm.sr_beg()->is_finish) {
                RE_SHOW(std::cout << ("first is finish") << std::endl);
                return test_ok;
            }

            this->data = sm.state();
            this->sr_first = sm.sr_first();
            this->step = 0;
            this->srs2.clear();
            this->srs2.reserve(sm.count_states());
            this->steps.resize(sm.count_states(), 0);
            return this->next(s, pos);
        }
        return ret;
    }

    result_test_type next(const char * s, size_t * pos = 0)
    {
        this->consumer.str(s);
        result_test_type ret = detail::search_impl(this->data,
                                                   this->sr_first,
                                                   this->srs1,
                                                   this->srs2,
                                                   this->steps,
                                                   this->step,
                                                   this->consumer);
        if (pos) {
            *pos = this->consumer.str() - s;
        }
        return ret;
    }

    bool stop()
    {
        for (detail::StateRange const * sr: this->srs1) {
            if (sr->v && !sr->is_finish) {
                if (detail::is_valid_state_range(sr)) {
                    return true;
                }
            }
            else {
                RE_SHOW(std::cout << "st.v.empty" << std::endl);
                return true;
            }
        }

        return false;
    }
};

inline bool exact_test(StateMachine2 const & sm, const char * s, size_t * pos = 0)
{
    ExactTest test;
    result_test_type ret = test.start(sm, s, pos);
    if (ret == test_run) {
        return test.stop();
    }
    return test_ok == ret;
}

inline bool search_test(StateMachine2 const & sm, const char * s, size_t * pos = 0)
{
    if (!sm.sr_first()) {
        BasicExactTest<false> test;
        result_test_type ret = test.start(sm, s, pos);
        if (ret == test_run) {
            return test.stop();
        }
        return test_ok == ret;
    }
    SearchTest test;
    result_test_type ret = test.start(sm, s, pos);
    if (ret == test_run) {
        return test.stop();
    }
    return test_ok == ret;
}


}

#endif
