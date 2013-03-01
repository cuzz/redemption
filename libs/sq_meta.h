/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2013
   Author(s): Christophe Grosjean

   Template for new SQ_Intracker sequence class

*/

#ifndef _REDEMPTION_LIBS_SQ_META_H_
#define _REDEMPTION_LIBS_SQ_META_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "rio_constants.h"
#include "sq_intracker.h"

extern "C" {
    struct SQMeta {
        RT * tracker;
        struct SQIntracker impl;
    };


    RT_ERROR sq_m_SQMeta_constructor(SQMeta * self, const char * prefix, const char * extension)
    {
        TODO("Manage all actual open error with more details")
        char tmpname[1024];
        if ((size_t)snprintf(tmpname, sizeof(tmpname), "%s.%s", prefix, extension) >= sizeof(tmpname)){
            return RT_ERROR_FILENAME_TOO_LONG;
        }
        int fd = ::open(tmpname, O_RDONLY);
        if (fd < 0){
            return RT_ERROR_OPEN;
        }
        RT_ERROR status = RT_ERROR_OK;
        RT * rt = rt_new_infile(&status, fd);
        if (status != RT_ERROR_OK){
            close(fd);
            return status;
        }
        self->tracker = rt;
        status = sq_m_SQIntracker_constructor(&self->impl, rt);
        if (status != RT_ERROR_OK){
            rt_delete(rt);
        }
        return status;
    }

    RT_ERROR sq_m_SQMeta_destructor(SQMeta * self)
    {
        sq_m_SQIntracker_destructor(&self->impl);
        rt_delete(self->tracker);
        return RT_ERROR_OK;
    }

    RT * sq_m_SQMeta_get_trans(SQMeta * self, RT_ERROR * status)
    {
        return sq_m_SQIntracker_get_trans(&self->impl, status);
    }

    RT_ERROR sq_m_SQMeta_next(SQMeta * self)
    {
        return sq_m_SQIntracker_next(&self->impl);
    }
};

#endif
