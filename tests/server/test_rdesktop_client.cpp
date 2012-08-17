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
   Copyright (C) Wallix 2010
   Author(s): Christophe Grosjean, Javier Caverni
   Based on xrdp Copyright (C) Jay Sorg 2004-2010

   Unit test to check front-end behavior stays identical 
   when connecting from rdesktop (mocked up)

*/


#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestRdpClientW2000
#include <boost/test/auto_unit_test.hpp>
#include <errno.h>
#include <algorithm>
#include <sys/un.h>

#define LOGPRINT
#include "log.hpp"
#include "constants.hpp"
#include "listen.hpp"

BOOST_AUTO_TEST_CASE(TestIncomingConnection)
{
    // This server only support one incoming connection before closing listener
    class ServerOnce : public Server
    {
        public:
        int sck;
        char ip_source[256];

        ServerOnce() : sck(0)
        {
            ip_source[0] = 0;
        }

        virtual Server_status start(int incoming_sck)
        {
            struct sockaddr_in sin;
            unsigned int sin_size = sizeof(struct sockaddr_in);
            memset(&sin, 0, sin_size);
            this->sck = accept(incoming_sck, (struct sockaddr*)&sin, &sin_size);
            strcpy(ip_source, inet_ntoa(sin.sin_addr));
            LOG(LOG_INFO, "Incoming socket to %d (ip=%s)\n", sck, ip_source);
            return START_WANT_STOP;
        }
    } one_shot_server;
    Listen listener(one_shot_server, 3389, true, 5); // 5 seconds to connect, or timeout


    LOG(LOG_INFO, "Listener closed\n");
    LOG(LOG_INFO, "Incoming socket %d (ip=%s)\n", one_shot_server.sck, one_shot_server.ip_source);
}