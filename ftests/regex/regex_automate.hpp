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

#ifndef REDEMPTION_REGEX_AUTOMATE_HPP
#define REDEMPTION_REGEX_AUTOMATE_HPP

#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <cstring>
#include <cassert>

#include "regex_states_wraper.hpp"

namespace re {

    class StateMachine2
    {
        class RangeList;
        class StateList;

        struct is_begin_state {
            bool operator()(const StateList& stl) const
            {
                return stl.st->type == FIRST;
            }
        };

        StateMachine2(const StateMachine2&) /*= delete*/;

    public:
        explicit StateMachine2(StatesWrapper & state_wrapper)
        : stw(state_wrapper)
        , idx_trace(-1u)
        , idx_trace_free(0)
        , pidx_trace_free(0)
        , captures(0)
        , pcaptures(0)
        , traces(0)
        , l1()
        , l2()
        , st_list(0)
        , st_range_list(0)
        {
            if (this->stw.states.empty()) {
                return ;
            }

            const size_t count_st = this->stw.size();
            l1.set_parray(new StateListByStep::Info[count_st * 2]);
            l2.set_parray(l1.array + count_st);

            {
                const size_t matrix_size = count_st * count_st;
                this->st_list = new StateList[matrix_size];
                std::memset(this->st_list, 0, matrix_size * sizeof * this->st_list);
            }

            {
                this->st_range_list = new RangeList[count_st];
                this->st_range_list_last = this->st_range_list;
                for (unsigned n = 0; n < count_st; ++n) {
                    RangeList& l = *this->st_range_list_last;
                    ++this->st_range_list_last;
                    l.st = 0;
                    l.first = this->st_list + n * count_st;
                    l.last = l.first;
                }
            }

            if (this->stw.nb_capture) {
                const unsigned col = count_st - this->stw.nb_capture;
                const unsigned matrix_size = col * this->stw.nb_capture;

                this->captures = new StateBase const *[this->stw.nb_capture + matrix_size];
                this->traces = reinterpret_cast<const char **>(this->captures + this->stw.nb_capture);

                this->idx_trace_free = new unsigned[col+1];

                this->pcaptures = std::copy(this->stw.states.end() - this->stw.nb_capture,
                                            this->stw.states.end(),
                                            this->captures);
            }

            {
                unsigned step = 1;
                this->init_list(this->st_range_list, this->stw.root, step);
            }

            while (this->st_range_list != this->st_range_list_last && 0 == (this->st_range_list_last-1)->st) {
                --this->st_range_list_last;
            }

            this->st_range_beginning.st = 0;
            this->st_range_beginning.first = std::partition(
                this->st_range_list->first,
                this->st_range_list->last,
                is_begin_state()
            );
            if (st_range_beginning.first != this->st_range_list->first) {
                this->st_range_beginning.last = this->st_range_beginning.first;
                StateList * l = st_range_list->first;
                StateList * rlast = st_range_list->last;
                this->st_range_beginning.st = l->st;
                for (; l != rlast; ++l) {
                    StateList * first = l->next->first;
                    StateList * last = l->next->last;
                    for (; first != last; ++first) {
                        st_range_beginning.last->next = first->next;
                        st_range_beginning.last->st = first->st;
                        ++st_range_beginning.last;
                    }
                }

                this->st_range_list->first += (this->st_range_beginning.last - this->st_range_beginning.first);
            }
            else {
                this->st_range_beginning.st = this->st_range_list->st;
                this->st_range_beginning.first = this->st_range_list->first;
                this->st_range_beginning.last = this->st_range_list->last;
            }
        }

        ~StateMachine2()
        {
            delete [] this->st_list;
            delete [] this->st_range_list;
            delete [] this->captures;
            delete [] this->idx_trace_free;
            delete [] this->l1.array;
        }

    private:
        unsigned pop_idx_trace(unsigned cp_idx)
        {
            --this->pidx_trace_free;
            assert(this->pidx_trace_free >= this->idx_trace_free);
            const unsigned size = this->stw.nb_capture;
            char const ** from = this->traces + cp_idx * size;
            char const ** to = this->traces + *this->pidx_trace_free * size;
            for (char const ** last = to + size; to < last; ++to, ++from) {
                *to = *from;
            }
            return *this->pidx_trace_free;
        }

        void push_idx_trace(unsigned n)
        {
            assert(this->pidx_trace_free <= this->idx_trace_free + this->stw.size() - this->stw.nb_capture);
            *this->pidx_trace_free = n;
            ++this->pidx_trace_free;
        }

        void push_state(RangeList* l, const StateBase * st, unsigned step)
        {
            if (st && this->stw.get_num_at(st) != step) {
                this->stw.set_num_at(st, step);
                if (st->is_split()) {
                    this->push_state(l, st->out1, step);
                    this->push_state(l, st->out2, step);
                }
                else {
                    l->last->st = st;
                    ++l->last;
                    if (st->is_cap()) {
                        this->push_state(l, st->out1, step);
                    }
                }
            }
        }

        RangeList* find_range_list(const StateBase * st)
        {
            /**///std::cout << (__FUNCTION__) << std::endl;
            for (RangeList * l = this->st_range_list; l < this->st_range_list_last && l->st; ++l) {
                if (l->st == st) {
                    return l;
                }
            }
            return 0;
        }

        void init_list(RangeList* l, const StateBase * st, unsigned& step)
        {
            /**///std::cout << (__FUNCTION__) << std::endl;
            l->st = st;
            this->push_state(l, st, step);
            /**///std::cout << "-- " << (l) << std::endl;
            for (StateList * first = l->first, * last = l->last; first < last; ++first) {
                /**///std::cout << first->st->num << ("\t") << first->st << ("\t") << first->next << std::endl;
                if (0 == first->st->out1) {
                    continue ;
                }
                if (first->st->out1->is_finish()) {
                    first->next = 0;
                    continue;
                }
                RangeList * luse = this->find_range_list(first->st->out1);
                /**///std::cout << "[" << luse << "]" << std::endl;
                if (luse) {
                    first->next = luse;
                }
                else {
                    RangeList * le = l+1;
                    while (le < this->st_range_list_last && le->st) {
                        ++le;
                    }
                    first->next = le;
                    this->init_list(le, first->st->out1, ++step);
                }
            }
        }

    public:
        typedef std::pair<const char **, const char **> TraceRange;

        TraceRange get_trace() const
        {
            char const ** strace = this->traces + this->idx_trace * this->stw.nb_capture;
            return TraceRange(strace, strace + this->stw.nb_capture);
        }

        typedef std::pair<const char *, const char *> range_t;
        typedef std::vector<range_t> range_matches;

    private:
        struct DefaultMatchTracer
        {
            void start(unsigned /*idx*/) const
            {}

            void new_id(unsigned /*old_id*/, unsigned /*new_id*/) const
            {}

            bool open(unsigned /*idx*/, const char *, unsigned /*num_cap*/) const
            { return true; }

            bool close(unsigned /*idx*/, const char *, unsigned /*num_cap*/) const
            { return true; }

            void fail(unsigned /*idx*/) const
            {}

            void good(unsigned /*idx*/) const
            {}
        };

    public:
        bool exact_search(const char * s, unsigned step_limit, bool check_end = true)
        {
            if (this->stw.states.empty()) {
                return false;
            }
            return Searching(*this).exact_search(s, step_limit, check_end);
        }

        bool exact_search_with_trace(const char * s, unsigned step_limit,
                                     bool check_end = true)
        {
            if (this->stw.nb_capture == 0) {
                return exact_search(s, step_limit, check_end);
            }
            return Matching(*this).match(s, step_limit, DefaultMatchTracer(),
                                         Matching::is_exact());
        }

        template<typename Tracer>
        bool exact_search_with_trace(const char * s, unsigned step_limit,
                                     Tracer tracer, bool check_end = true)
        {
            if (this->stw.nb_capture == 0) {
                return exact_search(s, step_limit, check_end);
            }
            return Matching(*this).match(s, step_limit, tracer, Matching::is_exact());
        }

        bool search(const char * s, unsigned step_limit)
        {
            if (this->stw.states.empty()) {
                return false;
            }
            return Searching(*this).search(s, step_limit);
        }

        bool search_with_trace(const char * s, unsigned step_limit)
        {
            if (this->stw.nb_capture == 0) {
                return search(s, step_limit);
            }
            return Matching(*this).match(s, step_limit, DefaultMatchTracer(),
                                         Matching::is_not_exact());
        }

        template<typename Tracer>
        bool search_with_trace(const char * s, unsigned step_limit, Tracer tracer)
        {
            if (this->stw.nb_capture == 0) {
                return search(s, step_limit);
            }
            return Matching(*this).match(s, step_limit, tracer, Matching::is_not_exact());
        }

        range_matches match_result()
        {
            range_matches ret;
            this->append_match_result(ret);
            return ret;
        }

        void append_match_result(range_matches& ranges) const
        {
            ranges.reserve(this->stw.nb_capture / 2);
            TraceRange trace = this->get_trace();

            const StateBase ** pst = this->captures;
            while (pst < this->pcaptures) {
                while ((*pst)->is_cap_close()) {
                    if (++pst >= this->pcaptures) {
                        return ;
                    }
                }
                const StateBase ** pbst = pst;
                unsigned n = 1;
                while (++pst < this->pcaptures && ((*pst)->is_cap_open() ? ++n : --n)) {
                }
                if (trace.first[pst - this->captures]) {
                    ranges.push_back(range_t(
                        trace.first[pbst - this->captures],
                        trace.first[pst - this->captures]
                    ));
                }
                pst = ++pbst;
            }
        }

    private:
        struct StateListByStep
        {
            struct Info {
                RangeList * rl;
                unsigned idx;
            };

            StateListByStep()
            : array(0)
            {}

            void push_back(RangeList* val, unsigned idx)
            {
                this->parray->rl = val;
                this->parray->idx = idx;
                ++this->parray;
            }

            void push_back(RangeList* val)
            {
                this->parray->rl = val;
                ++this->parray;
            }

            Info * begin() const
            { return this->array; }

            Info * end() const
            { return this->parray; }

            void set_parray(Info * p)
            {
                this->array = p;
                this->parray = p;
            }

            bool empty() const
            { return this->array == this->parray; }

            void clear()
            { this->parray = this->array; }

            Info * array;
            Info * parray;
        };

        struct Searching
        {
            StateMachine2 &sm;
            unsigned step_id;

            Searching(StateMachine2& sm)
            : sm(sm)
            , step_id(1)
            {}

            typedef StateListByStep::Info Info;

            RangeList * step(char_int c, RangeList * l, unsigned & step_count)
            {
                for (StateList * first = l->first, * last = l->last; first != last; ++first) {
                    ++step_count;
                    if (!first->st->is_cap() && first->st->check(c)) {
#ifdef DISPLAY_TRACE
                        this->sm.display_elem_state_list(*first, 0);
#endif
                        if ( ! first->next && first->st->is_finish()) {
                            continue ;
                        }
                        return first->next;
                    }
                }

                return reinterpret_cast<RangeList*>(1);
            }

            bool exact_search(const char * s, unsigned step_limit, bool check_end = true)
            {
#ifdef DISPLAY_TRACE
                this->sm.display_dfa();
#endif

                this->sm.reset_id();

                RangeList * l = &this->sm.st_range_beginning;

                utf_consumer consumer(s);

                unsigned step_count = 0;

                while (consumer.valid() && l > reinterpret_cast<void*>(1)){
#ifdef DISPLAY_TRACE
                    std::cout << "\033[01;31mc: '" << utf_char(consumer.getc()) << "'\033[0m\n";
#endif
                    l = this->step(consumer.bumpc(), l, step_count);
                    if (step_count >= step_limit) {
                        return false;
                    }
                    ++this->step_id;
                }

                if ((0 == l && (check_end ? !consumer.valid() : 1))) {
                    return true;
                }
                if (reinterpret_cast<void*>(1) == l || consumer.valid()) {
                    return false;
                }

                for (StateList * first = l->first, * last = l->last; first != last; ++first) {
                    if (first->st->is_terminate()) {
                        return true;
                    }
                }

                return false;
            }

            unsigned step(char_int c, StatesWrapper & stw,  StateListByStep * l1, StateListByStep * l2, unsigned & step_count)
            {
                for (Info* ifirst = l1->begin(), * ilast = l1->end(); ifirst != ilast ; ++ifirst) {
                    ++step_count;
                    if (stw.get_num_at(ifirst->rl->st) == this->step_id) {
                        /**///std::cout << "\t\033[35mx " << (ifirst->idx) << "\033[0m\n";
                        continue;
                    }
                    stw.set_num_at(ifirst->rl->st, this->step_id);

                    for (StateList * first = ifirst->rl->first, * last = ifirst->rl->last; first != last; ++first) {
                        ++step_count;
                        if (!first->st->is_cap() && first->st->check(c)) {
#ifdef DISPLAY_TRACE
                            this->sm.display_elem_state_list(*first, 0);
#endif
                            if (0 == first->next) {
                                return 0;
                            }
                            l2->push_back(first->next);
                        }
                    }
                }
                return -1u;
            }

            bool search(const char * s, unsigned step_limit)
            {
#ifdef DISPLAY_TRACE
                this->sm.display_dfa();
#endif

                this->sm.l1.clear();
                this->sm.l2.clear();
                this->sm.reset_id();

                StateListByStep * pal1 = &this->sm.l1;
                StateListByStep * pal2 = &this->sm.l2;
                this->sm.l1.push_back(&this->sm.st_range_beginning);

                utf_consumer consumer(s);

                unsigned step_count = 0;

                while (consumer.valid()) {
#ifdef DISPLAY_TRACE
                    std::cout << "\033[01;31mc: '" << utf_char(consumer.getc()) << "'\033[0m\n";
#endif
                    if (0 == this->step(consumer.bumpc(), this->sm.stw, pal1, pal2, step_count)) {
                        return true;
                    }
                    if (step_count >= step_limit) {
                        return false;
                    }
                    ++this->step_id;
                    std::swap(pal1, pal2);
                    pal2->clear();
                    pal1->push_back(this->sm.st_range_list);
                }

                for (Info* ifirst = pal1->begin(), * ilast = pal1->end(); ifirst != ilast ; ++ifirst) {
                    for (StateList * first = ifirst->rl->first, * last = ifirst->rl->last; first != last; ++first) {
                        if (first->st->is_terminate()) {
                            return true;
                        }
                    }
                }

                return false;
            }
        };

    private:
        void reset_id()
        {
            for (RangeList * l = this->st_range_list; l < this->st_range_list_last; ++l) {
                this->stw.set_num_at(l->st, 0);
            }
        }

        void reset_trace()
        {
            this->pidx_trace_free = this->idx_trace_free;
            const unsigned size = this->stw.size() - this->stw.nb_capture;
            //pidx_trace_free ∈ [0, size+1]
            for (unsigned i = 0; i != size; ++i, ++this->pidx_trace_free) {
                *this->pidx_trace_free = i;
            }
            std::memset(this->traces, 0,
                        size * this->stw.nb_capture * sizeof this->traces[0]);
        }

    public:
        void display_elem_state_list(const StateList& e, unsigned idx) const
        {
            std::cout << "\t\033[33m" << idx << "\t" << e.st->num << "\t"
            << *e.st << "\t" << (e.next) << "\033[0m" << std::endl;
        }

        void display_dfa(const RangeList * l, const RangeList * last) const
        {
            for (; l < last && l->first != l->last; ++l) {
                std::cout << l << "  st: " << l->st->num << (l->st->is_cap() ? " (cap)\n" : "\n");
                for (StateList * first = l->first, * last = l->last; first != last; ++first) {
                    std::cout << "\t" << first->st->num << "\t"
                    << *first->st << "\t" << first->next << ("\n");
                }
            }
        }

        void display_dfa() const
        {
            if (this->st_range_beginning.first != this->st_range_beginning.last) {
                std::cout << ("beginning") << std::endl;
                this->display_dfa(&this->st_range_beginning, &this->st_range_beginning+1);
                std::cout << ("\nrange") << std::endl;
            }

            this->display_dfa(this->st_range_list, this->st_range_list_last);
            std::cout << std::endl;
        }

    private:
        struct Matching
        {
            StateMachine2 &sm;
            unsigned step_id;

            Matching(StateMachine2& sm)
            : sm(sm)
            , step_id(1)
            {}

            typedef StateListByStep::Info Info;

            template<typename Tracer>
            unsigned step(const char * s, char_int c,
                          StateListByStep * l1, StateListByStep * l2,
                          Tracer& tracer, unsigned & step_count)
            {
                for (Info* ifirst = l1->begin(), * ilast = l1->end(); ifirst != ilast ; ++ifirst) {
                    if (this->sm.stw.get_num_at(ifirst->rl->st) == this->step_id) {
#ifdef DISPLAY_TRACE
                        std::cout << "\t\033[35mdup " << (ifirst->idx) << "\033[0m\n";
#endif
                        tracer.fail(ifirst->idx);
                        this->sm.push_idx_trace(ifirst->idx);
                        continue;
                    }

                    unsigned new_trace = 0;
                    this->sm.stw.set_num_at(ifirst->rl->st, this->step_id);
                    StateList * first = ifirst->rl->first;
                    StateList * last = ifirst->rl->last;

                    for (; first != last; ++first) {
                        ++step_count;
                        unsigned num = first->st->num - (this->sm.stw.size() - this->sm.stw.nb_capture);
                        if (first->st->is_cap_open()) {
                            if (!this->sm.traces[ifirst->idx * this->sm.stw.nb_capture + num] && tracer.open(ifirst->idx, s, num)) {
#ifdef DISPLAY_TRACE
                                std::cout << ifirst->idx << "  " << *first->st << "  " << num << std::endl;
#endif
                                ++this->sm.traces[ifirst->idx * this->sm.stw.nb_capture + num] = s;
                            }
                            continue ;
                        }

                        if (first->st->is_cap_close()) {
                            if (tracer.close(ifirst->idx, s, num)) {
#ifdef DISPLAY_TRACE
                                std::cout << ifirst->idx << "  " << *first->st << "  " << num << std::endl;
#endif
                                ++this->sm.traces[ifirst->idx * this->sm.stw.nb_capture + num] = s;
                            }
                            continue ;
                        }

                        if (first->st->check(c)) {
#ifdef DISPLAY_TRACE
                            this->sm.display_elem_state_list(*first, ifirst->idx);
#endif

                            if (0 == first->next) {
                                /**///std::cout << "idx: " << (ifirst->idx) << std::endl;
                                return ifirst->idx;
                            }

                            unsigned idx = ifirst->idx;
                            if (new_trace) {
                                idx = this->sm.pop_idx_trace(ifirst->idx);
                                tracer.new_id(ifirst->idx, idx);
                            }
#ifdef DISPLAY_TRACE
                            std::cout << "\t\033[32m" << ifirst->idx << " -> " << idx << "\033[0m" << std::endl;
#endif
                            l2->push_back(first->next, idx);
                            ++new_trace;
                        }
                    }
                    if (0 == new_trace) {
#ifdef DISPLAY_TRACE
                        std::cout << "\t\033[35mx " << ifirst->idx << "\033[0m" << std::endl;
#endif
                        tracer.fail(ifirst->idx);
                        this->sm.push_idx_trace(ifirst->idx);
                    }
                }

                return -1u;
            }

            struct is_exact { static const bool value = true; };
            struct is_not_exact { static const bool value = false; };

            template<typename Tracer, typename ExactMatch>
            bool match(const char * s, unsigned step_limit, Tracer tracer, ExactMatch)
            {
#ifdef DISPLAY_TRACE
                this->sm.display_dfa();
#endif

                this->sm.l1.clear();
                this->sm.l2.clear();
                this->sm.reset_id();
                this->sm.reset_trace();

                StateListByStep * pal1 = &this->sm.l1;
                StateListByStep * pal2 = &this->sm.l2;
                this->sm.l1.push_back(&this->sm.st_range_beginning, *--this->sm.pidx_trace_free);
                tracer.start(*this->sm.pidx_trace_free);

                utf_consumer consumer(s);

                unsigned step_count = 0;

                while (consumer.valid()) {
#ifdef DISPLAY_TRACE
                    std::cout << "\033[01;31mc: '" << utf_char(consumer.getc()) << "'\033[0m" << std::endl;
#endif
                    if (-1u != (this->sm.idx_trace = this->step(s, consumer.bumpc(), pal1, pal2, tracer, step_count))) {
                        tracer.good(this->sm.idx_trace);
                        return false == ExactMatch::value || !consumer.getc();
                    }
                    if (pal2->empty()) {
                        break;
                    }
                    if (step_count >= step_limit) {
                        return false;
                    }
                    ++this->step_id;
                    std::swap(pal1, pal2);
                    pal2->clear();
                    if (false == ExactMatch::value) {
                        --this->sm.pidx_trace_free;
                        assert(this->sm.pidx_trace_free >= this->sm.idx_trace_free);
#ifdef DISPLAY_TRACE
                        std::cout << "\t\033[32m-> " << *this->sm.pidx_trace_free << "\033[0m" << std::endl;
#endif
                        pal1->push_back(this->sm.st_range_list, *this->sm.pidx_trace_free);
                    }
                    s = consumer.str();
                }

                if (!consumer.valid()) {
                    for (Info* ifirst = pal1->begin(), * ilast = pal1->end(); ifirst != ilast ; ++ifirst) {
                        if (this->sm.stw.get_num_at(ifirst->rl->st) == this->step_id) {
                            /**///std::cout << "\t\033[35mx " << (ifirst->idx) << "\033[0m\n";
                            tracer.fail(ifirst->idx);
                            this->sm.push_idx_trace(ifirst->idx);
                            continue;
                        }

                        this->sm.stw.set_num_at(ifirst->rl->st, this->step_id);
                        StateList * first = ifirst->rl->first;
                        StateList * last = ifirst->rl->last;

                        for (; first != last; ++first) {
                            if (first->st->is_cap_close()) {
                                unsigned num = first->st->num - (this->sm.stw.size() - this->sm.stw.nb_capture);
                                if (tracer.close(ifirst->idx, s, num)) {
#ifdef DISPLAY_TRACE
                                    std::cout << ifirst->idx << "  " << *first->st << "  " << num << std::endl;
#endif
                                    if (0 == first->st->out1) {
                                        this->sm.idx_trace = ifirst->idx;
                                        ++this->sm.traces[ifirst->idx * this->sm.stw.nb_capture + num] = s;
                                        return true;
                                    }
                                }
                                continue ;
                            }

                            if (first->st->type == LAST) {
                                this->sm.idx_trace = ifirst->idx;
                                return true;
                            }
                        }
                    }
                }

                return false;
            }
        };

        friend class Matching;
        friend class Searching;

        typedef std::vector<StateBase*> state_list;
        typedef state_list::iterator state_iterator;

        StatesWrapper & stw;

        unsigned idx_trace;
        unsigned * idx_trace_free;
        unsigned * pidx_trace_free;
        const StateBase ** captures;
        const StateBase ** pcaptures;
        const char ** traces;
        StateListByStep l1;
        StateListByStep l2;

        struct StateList
        {
            const StateBase * st;
            RangeList * next;
        };

        StateList * st_list;

        struct RangeList
        {
            const StateBase * st;
            StateList * first;
            StateList * last;
        };

        RangeList * st_range_list;
        RangeList * st_range_list_last;
        RangeList st_range_beginning;
    };

    inline void display_state(StatesWrapper & stw, StateBase * st, unsigned depth = 0)
    {
        if (st && stw.get_num_at(st) != -2u) {
            std::string s(depth, '\t');
            std::cout
            << s << "\033[33m" << st << "\t" << st->num << "\t" << *st
            << "\033[0m\n\t" << s << st->out1 << "\n\t" << s << st->out2 << "\n";
            stw.set_num_at(st, -2u);
            display_state(stw, st->out1, depth+1);
            display_state(stw, st->out2, depth+1);
        }
    }

    inline void display_state(StatesWrapper & stw)
    {
        stw.reset_nums();
        display_state(stw, stw.root);
    }


    inline bool st_exact_step(const_state_list_t & l1, const_state_list_t & l2,
                              StatesWrapper & stw, size_t c, utf_consumer & consumer, unsigned count)
    {
        struct add {
            static bool impl(const_state_list_t & l, const StateBase * st,
                             StatesWrapper & stw, bool is_end, unsigned count)
            {
                if (stw.get_num_at(st) == count) {
                    return false;
                }
                stw.set_num_at(st, count);
                if (st->is_split()) {
                    if (st->out1 && impl(l, st->out1, stw, is_end, count)) {
                        return true;
                    }
                    if (st->out2 && impl(l, st->out2, stw, is_end, count)) {
                        return true;
                    }
                }
                else if (st->is_cap()) {
                    if (st->out1) {
                        return impl(l, st->out1, stw, is_end, count);
                    }
                }
                else if (st) {
                    if (st->is_terminate()) {
                        if (is_end) {
                            return true;
                        }
                    }
                    else {
                        l.push_back(st);
                    }
                }
                return false;
            }
        };

        const bool is_end = ! consumer.valid();
        for (const_state_list_t::iterator first = l1.begin(), last = l1.end(); first != last; ++first) {
            if ( ! (*first)->is_cap() && (*first)->check(c)) {
                if ( ! (*first)->out1 && ! (*first)->out2 && is_end) {
                    return true;
                }
                if ((*first)->out1 && add::impl(l2, (*first)->out1, stw, is_end, count)) {
                    return true;
                }
                if ((*first)->out2 && add::impl(l2, (*first)->out2, stw, is_end, count)) {
                    return true;
                }
            }
        }
        return false;
    }

    inline void add_first(const_state_list_t & l, const_state_list_t & lfirst, const StateBase * st)
    {
        if (st->is_split()) {
            if (st->out1) {
                add_first(l, lfirst, st->out1);
            }
            if (st->out2) {
                add_first(l, lfirst, st->out2);
            }
        }
        else if (st->is_cap()) {
            if (st->out1) {
                add_first(l, lfirst, st->out1);
            }
        }
        else if (st->type == FIRST) {
            if (st->out1) {
                add_first(lfirst, lfirst, st->out1);
            }
        }
        else {
            l.push_back(st);
        }
    }

    inline bool st_exact_search(StatesWrapper & stw, const char * s)
    {
        if ( ! stw.root) {
            return false;
        }
        const_state_list_t l1;
        add_first(l1, l1, stw.root);
        if (l1.empty()) {
            return false;
        }
        utf_consumer consumer(s);
        const_state_list_t l2;
        bool res = false;
        unsigned count = 1;
        while (consumer.valid() && !(res = st_exact_step(l1, l2, stw, consumer.bumpc(), consumer, ++count))) {
            if (l2.empty()) {
                return false;
            }
            l1.swap(l2);
            l2.clear();
        }

        if (consumer.valid()) {
            return false;
        }
        if (res) {
            return true;
        }
        return false;
    }

    inline bool st_step(const_state_list_t & l1, const_state_list_t & l2,
                        StatesWrapper & stw, size_t c, unsigned count)
    {
        struct add {
            static bool impl(const_state_list_t& l, const StateBase * st, StatesWrapper & stw, unsigned count) {
                if (st->is_finish()) {
                    return true;
                }
                if (stw.get_num_at(st) == count) {
                    return false;
                }
                stw.set_num_at(st, count);
                if (st->is_split()) {
                    if (st->out1 && impl(l, st->out1, stw, count)) {
                        return true;
                    }
                    if (st->out2 && impl(l, st->out2, stw, count)) {
                        return true;
                    }
                }
                else if (st->is_cap()) {
                    if (st->out1) {
                        return impl(l, st->out1, stw, count);
                    }
                }
                else if (st) {
                    l.push_back(st);
                }
                return false;
            }
        };

        for (const_state_list_t::iterator first = l1.begin(), last = l1.end(); first != last; ++first) {
            if ( ! (*first)->is_cap() && (*first)->check(c)) {
                if ( ! (*first)->out1 && ! (*first)->out2) {
                    return true;
                }
                if ((*first)->out1) {
                    if (add::impl(l2, (*first)->out1, stw, count)) {
                        return true;
                    }
                }
                if ((*first)->out2) {
                    if (add::impl(l2, (*first)->out2, stw, count)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    inline bool st_search(StatesWrapper & stw, const char * s)
    {
        if ( ! stw.root) {
            return false;
        }

        const_state_list_t lst;
        const_state_list_t l1;
        add_first(lst, l1, stw.root);
        if (l1.empty() && lst.empty()) {
            return false;
        }
        utf_consumer consumer(s);
        const_state_list_t l2;
        bool res = false;
        unsigned count = 1;
        while (consumer.valid()) {
            for (const_state_list_t::iterator first = lst.begin(), last = lst.end(); first != last; ++first) {
                if (stw.get_num_at(*first) != count) {
                    l1.push_back(*first);
                }
            }
            if (l1.empty()) {
                return false;
            }
            l2.clear();
            if ((res = st_step(l1, l2, stw, consumer.bumpc(), ++count))) {
                break ;
            }
            l1.swap(l2);
        }
        if (res) {
            return true;
        }
        if (consumer.valid()) {
            return false;
        }
        for (const_state_list_t::iterator first = l1.begin(), last = l1.end(); first != last; ++first) {
            if ((*first)->type == LAST) {
                return true;
            }
        }
        return false;
    }
}

#endif
