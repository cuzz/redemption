/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Product name: redemption, a FLOSS RDP proxy
 *   Copyright (C) Wallix 2010-2013
 *   Author(s): Christophe Grosjean, Raphael Zhou, Jonathan Poelen,
 *              Meng Tan
 */


#if !defined(REDEMPTION_MOD_WIDGET2_RADIO_LIST_HPP)
#define REDEMPTION_MOD_WIDGET2_RADIO_LIST_HPP

#include "widget.hpp"
#include "label.hpp"

class WidgetRadioButton : public Widget2
{
public:
    WidgetLabel label;
    bool selected;

    WidgetRadioButton(DrawApi & drawable, int16_t x, int16_t y, Widget2& parent,
                      NotifyApi* notifier, const char * text, bool auto_resize = true,
                      int group_id = 0, int fgcolor = WHITE, int bgcolor = DARK_BLUE_BIS,
                      int xtext = 0, int ytext = 0)
        : Widget2(drawable, Rect(x,y,1,1), parent, notifier, group_id)
        , label(drawable, 1, 1, *this, 0, text, auto_resize, 0, fgcolor, bgcolor, 4, 2)
        , selected(false)
    {
        this->label.rect.x = this->dx() + this->label.cy() + 2;
        this->rect.cx = this->label.cx() + this->label.cy() + 3;
        this->rect.cy = this->label.cy() + 2;
        this->tab_flag = IGNORE_TAB;
        this->focus_flag = IGNORE_FOCUS;
    }

    virtual ~WidgetRadioButton() {}

    virtual void draw(const Rect& clip)
    {
        this->drawable.draw(RDPOpaqueRect(Rect(this->dx(), this->dy(),
                                               this->label.cy() + 1, this->cy()),
                                          this->label.bg_color),
                            clip);
        if (this->selected) {
            int sqx = this->label.cy() / 3 + 1;
            this->drawable.draw(RDPOpaqueRect(Rect(this->dx() + sqx, this->dy() + sqx,
                                                   sqx , sqx),
                                              this->label.fg_color),
                                clip);
        }
        this->drawable.draw(RDPOpaqueRect(Rect(this->dx(), this->dy(),
                                               this->cx(), 1),
                                          this->label.fg_color),
                            clip);
        this->drawable.draw(RDPOpaqueRect(Rect(this->dx(), this->dy(),
                                               1, this->cy()),
                                          this->label.fg_color),
                            clip);
        this->drawable.draw(RDPOpaqueRect(Rect(this->dx() + this->label.cy() + 1, this->dy(),
                                               1, this->cy()),
                                          this->label.fg_color),
                            clip);
        this->drawable.draw(RDPOpaqueRect(Rect(this->dx() + this->cx() - 1, this->dy(),
                                               1, this->cy()),
                                          this->label.fg_color),
                            clip);
        this->drawable.draw(RDPOpaqueRect(Rect(this->dx(), this->dy() + this->cy() - 1,
                                               this->cx(), 1),
                                          this->label.fg_color),
                            clip);
        this->label.draw(clip);
    }

    void select() {
        this->selected = true;
        this->send_notify(NOTIFY_SUBMIT);
        this->refresh(this->rect);
    }
    void unselect() {
        this->selected = false;
        this->refresh(this->rect);
    }

};

class WidgetRadioList : public Widget2
{
    enum {
        AUTOSIZE = 256
    };

    WidgetRadioButton * child_list[AUTOSIZE];
    size_t size;
    int selected;

public:
    WidgetRadioList(DrawApi & drawable, int x, int y, Widget2 & parent,
                    NotifyApi * notifier, int group_id = 0)
        : Widget2(drawable, Rect(x, y, 1, 1), parent, notifier, group_id)
        , size(0)
        , selected(-1)
    {
        this->focus_flag = IGNORE_FOCUS;
        this->tab_flag = IGNORE_TAB;
    }

    virtual ~WidgetRadioList() {
        for (size_t i = 0; i < this->size; ++i) {
            delete this->child_list[i];
            this->child_list[i] = NULL;
        }
    }

    virtual void add_elem(const char * text) {
        if (this->size == AUTOSIZE)
            return;
        WidgetRadioButton * radio = new WidgetRadioButton(drawable, this->lx() + 10, this->dy(),
                                                             this->parent, this, text);

        this->child_list[this->size] = radio;
        this->size++;
        this->rect.cx += radio->cx() + 10;
        if (this->rect.cy < radio->cy()) {
            this->rect.cy = radio->cy();
        }
    }

    virtual void select(size_t n) {
        if (n < this->size &&
            n >= 0 &&
            static_cast<int>(n) != this->selected) {
            if (this->selected >= 0) {
                this->child_list[this->selected]->unselect();
            }
            this->child_list[n]->select();
            this->selected = n;
        }
    }
    int get_selected() {
        return this->selected;
    }

    virtual void draw(const Rect& clip) {
        if (this->size > 0) {
            this->drawable.draw(RDPOpaqueRect(this->rect,
                                              this->child_list[0]->label.bg_color),
                                clip);
            for (size_t i = 0; i < this->size; i++) {
                this->child_list[i]->draw(clip);
            }
        }
    }

    int find_index(Widget2 * w) {
        int res = -1;
        for (size_t i = 0; i < this->size && res == -1; ++i){
            if (this->child_list[i] == w){
                res = i;
            }
        }
        return res;
    }

    int index_at_pos(int16_t x, int16_t y) {
        WidgetRadioButton * ret = 0;
        int res = -1;
        for (size_t i = 0; i < this->size && ret == 0; ++i){
            if (this->child_list[i]->rect.contains_pt(x, y)){
                ret = this->child_list[i];
                res = i;
            }
        }
        if (ret) {
            if (x > (ret->dx() + ret->cy())) {
                res = -1;
            }
        }
        return res;
    }

    virtual void rdp_input_mouse(int device_flags, int x, int y, Keymap2* keymap)
    {
        int index = this->index_at_pos(x, y);

        if (index >= 0 && index < static_cast<int>(this->size)){
            if (device_flags == (MOUSE_FLAG_BUTTON1|MOUSE_FLAG_DOWN)) {
                this->select(index);
            }
        }
    }

    virtual void notify(Widget2* widget, notify_event_t event)
    {
        int index = this->find_index(widget);
        if (index >= 0 && index < static_cast<int>(this->size)) {
            this->send_notify(NOTIFY_SUBMIT);
        }
    }

};

#endif
