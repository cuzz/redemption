/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2012
   Author(s): Christophe Grosjean

   Unit test to image chunk in WRM files
*/

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestWrmImageChunk
#include <boost/test/auto_unit_test.hpp>

#define LOGNULL

#include "test_orders.hpp"
#include "transport.hpp"
#include "testtransport.hpp"
#include "outfilenametransport.hpp"
#include "FileToGraphic.hpp"
#include "GraphicToFile.hpp"
#include "image_capture.hpp"


BOOST_AUTO_TEST_CASE(TestImageChunk)
{
    const char expected_stripped_wrm[] =
    /* 0000 */ "\xEE\x03\x1C\x00\x00\x00\x01\x00" // 03EE: META 0010: chunk_len=16 0001: 1 order
               "\x03\x00\x14\x00\x0A\x00\x18\x00" // WRM version = 3, width = 20, height=10, bpp=24
               "\x58\x02\x00\x01\x2c\x01\x00\x04\x06\x01\x00\x10"

// Initial black PNG image
/* 0000 */ "\x00\x10\x50\x00\x00\x00\x01\x00"
/* 0000 */ "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52" //.PNG........IHDR
/* 0010 */ "\x00\x00\x00\x14\x00\x00\x00\x0a\x08\x02\x00\x00\x00\x3b\x37\xe9" //.............;7.
/* 0020 */ "\xb1\x00\x00\x00\x0f\x49\x44\x41\x54\x28\x91\x63\x60\x18\x05\xa3" //.....IDAT(.c`...
/* 0030 */ "\x80\x96\x00\x00\x02\x62\x00\x01\xfc\x4c\x5e\xbd\x00\x00\x00\x00" //.....b...L^.....
/* 0040 */ "\x49\x45\x4e\x44\xae\x42\x60\x82"                                 //IEND.B`.

    /* 0000 */ "\xf0\x03\x10\x00\x00\x00\x01\x00" // 03F0: TIMESTAMP 0010: chunk_len=16 0001: 1 order
    /* 0000 */ "\x00\xCA\x9A\x3B\x00\x00\x00\x00" // 0x000000003B9ACA00 = 1000000000
    /* 0000 */ "\x00\x00\x1e\x00\x00\x00\x03\x00" // 0000: ORDERS  001A:chunk_len=26 0002: 2 orders
    /* 0000 */ "\x19\x0a\x1c\x14\x0a\xff"             // RED rect
    /* 0000 */ "\x11\x5f\x05\x05\xF6\xf9\x00\xFF\x11" // BLUE RECT
    /* 0000 */ "\x3f\x05\xfb\xf7\x07\xff\xff"         // WHITE RECT

    /* 0000 */ "\x00\x10\x74\x00\x00\x00\x01\x00" // 0x1000: IMAGE_CHUNK 0048: chunk_len=86 0001: 1 order
        "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a"                                 //.PNG....
        "\x00\x00\x00\x0d\x49\x48\x44\x52"                                 //....IHDR
        "\x00\x00\x00\x14\x00\x00\x00\x0a\x08\x02\x00\x00\x00"             //.............
        "\x3b\x37\xe9\xb1"                                                 //;7..
        "\x00\x00\x00"
/* 0000 */ "\x33\x49\x44\x41\x54\x28\x91\x63\x64\x60\xf8\xcf\x80\x1b\xfc\xff" //3IDAT(.cd`......
/* 0010 */ "\xcf\xc0\xc8\x88\x53\x96\x09\x8f\x4e\x82\x60\x88\x6a\x66\x41\xe3" //....S...N.`.jfA.
/* 0020 */ "\xff\x67\x40\x0b\x9f\xff\xc8\x22\x8c\xa8\xa1\x3b\x70\xce\x66\x1c" //.g@...."...;p.f.
/* 0030 */ "\xb0\x78\x06\x00\x69\xde\x0a\x12\x3d\x77\xd0\x9e\x00\x00\x00\x00" //.x..i...=w......
/* 0040 */ "\x49\x45\x4e\x44\xae\x42\x60\x82"                                 //IEND.B`
    ;

    // Timestamps are applied only when flushing
    struct timeval now;
    now.tv_usec = 0;
    now.tv_sec = 1000;

    try {
        Rect scr(0, 0, 20, 10);
        CheckTransport trans(expected_stripped_wrm, sizeof(expected_stripped_wrm)-1, 511);
        Inifile ini;
        BmpCache bmp_cache(24, 600, 256, 300, 1024, 262, 4096);
        RDPDrawable drawable(scr.cx, scr.cy);
        GraphicToFile consumer(now, &trans, scr.cx, scr.cy, 24, bmp_cache, drawable, ini);
        consumer.draw(RDPOpaqueRect(scr, RED), scr);
        consumer.draw(RDPOpaqueRect(Rect(5, 5, 10, 3), BLUE), scr);
        consumer.draw(RDPOpaqueRect(Rect(10, 0, 1, 10), WHITE), scr);
        consumer.flush();
        consumer.send_image_chunk();
    }
    catch (Error & e){
        BOOST_CHECK(false);
    };
}

BOOST_AUTO_TEST_CASE(TestImagePNGMediumChunks)
{
    // Same test as above but forcing use of small png chunks
    // Easier to do than write tests with huge pngs to force PNG chunking.

    const char expected[] =
    /* 0000 */ "\xEE\x03\x1C\x00\x00\x00\x01\x00" // 03EE: META 0010: chunk_len=16 0001: 1 order
               "\x03\x00\x14\x00\x0A\x00\x18\x00" // WRM version 3, width = 20, height=10, bpp=24
               "\x58\x02\x00\x01\x2c\x01\x00\x04\x06\x01\x00\x10"

// Initial black PNG image
/* 0000 */ "\x00\x10\x50\x00\x00\x00\x01\x00"
/* 0000 */ "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52" //.PNG........IHDR
/* 0010 */ "\x00\x00\x00\x14\x00\x00\x00\x0a\x08\x02\x00\x00\x00\x3b\x37\xe9" //.............;7.
/* 0020 */ "\xb1\x00\x00\x00\x0f\x49\x44\x41\x54\x28\x91\x63\x60\x18\x05\xa3" //.....IDAT(.c`...
/* 0030 */ "\x80\x96\x00\x00\x02\x62\x00\x01\xfc\x4c\x5e\xbd\x00\x00\x00\x00" //.....b...L^.....
/* 0040 */ "\x49\x45\x4e\x44\xae\x42\x60\x82"                                 //IEND.B`.

    /* 0000 */ "\xf0\x03\x10\x00\x00\x00\x01\x00" // 03F0: TIMESTAMP 0010: chunk_len=16 0001: 1 order
    /* 0000 */ "\x00\xCA\x9A\x3B\x00\x00\x00\x00" // 0x000000003B9ACA00 = 1000000000
    /* 0000 */ "\x00\x00\x1e\x00\x00\x00\x03\x00" // 0000: ORDERS  001A:chunk_len=26 0002: 2 orders
    /* 0000 */ "\x19\x0a\x1c\x14\x0a\xff"             // RED rect
    /* 0000 */ "\x11\x5f\x05\x05\xF6\xf9\x00\xFF\x11" // BLUE RECT
    /* 0000 */ "\x3f\x05\xfb\xf7\x07\xff\xff"         // WHITE RECT

    /* 0000 */ "\x01\x10\x64\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order

        "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a"                                 //.PNG....
        "\x00\x00\x00\x0d\x49\x48\x44\x52"                                 //....IHDR
        "\x00\x00\x00\x14\x00\x00\x00\x0a"
        "\x08\x02\x00\x00\x00"             //.............
        "\x3b\x37\xe9\xb1"                                                 //;7..
        "\x00\x00\x00\x32\x49\x44\x41\x54"                                 //...2IDAT
        "\x28\x91\x63\xfc\xcf\x80\x17"
        "\xfc\xff\xcf\xc0\xc8\x88\x4b\x92"
        "\x09" //(.c..........K..
        "\xbf\x5e\xfc\x60\x88\x6a\x66\x41\xe3\x33\x32\xa0\x84\xe0\x7f"
        "\x54" //.^.`.jfA.32....T
        "\x91\xff\x0c\x28\x81\x37\x70\xce\x66\x1c\xb0\x78\x06\x00\x69\xdc" //...(.7p.f..x..i.
        "\x0a\x12"                                                         //..
        "\x86"
        "\x00\x10\x17\x00\x00\x00\x01\x00"  // 0x1000: FINAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x4a\x0c\x44"                                                 //.J.D
        "\x00\x00\x00\x00\x49\x45\x4e\x44"                             //....IEND
        "\xae\x42\x60\x82"                                             //.B`.
        ;

    // Timestamps are applied only when flushing
    struct timeval now;
    now.tv_usec = 0;
    now.tv_sec = 1000;

    Rect scr(0, 0, 20, 10);
    CheckTransport trans(expected, sizeof(expected)-1, 511);
    Inifile ini;
    BmpCache bmp_cache(24, 600, 256, 300, 1024, 262, 4096);
    RDPDrawable drawable(scr.cx, scr.cy);
    GraphicToFile consumer(now, &trans, scr.cx, scr.cy, 24, bmp_cache, drawable, ini);
    consumer.draw(RDPOpaqueRect(scr, RED), scr);
    consumer.draw(RDPOpaqueRect(Rect(5, 5, 10, 3), BLUE), scr);
    consumer.draw(RDPOpaqueRect(Rect(10, 0, 1, 10), WHITE), scr);
    consumer.flush();

    OutChunkedBufferingTransport<100> png_trans(&trans);
    try {
        consumer.drawable.dump_png24(&png_trans, true);
    } catch (Error & e) {
        BOOST_CHECK(false);
    };
}

BOOST_AUTO_TEST_CASE(TestImagePNGSmallChunks)
{
    // Same test as above but forcing use of small png chunks
    // Easier to do than write tests with huge pngs to force PNG chunking.

    const char expected[] =
    /* 0000 */ "\xEE\x03\x1C\x00\x00\x00\x01\x00" // 03EE: META 0010: chunk_len=16 0001: 1 order
               "\x03\x00\x14\x00\x0A\x00\x18\x00" // WRM version = 3, width = 20, height=10, bpp=24
               "\x58\x02\x00\x01\x2c\x01\x00\x04\x06\x01\x00\x10"

// Initial black PNG image
/* 0000 */ "\x00\x10\x50\x00\x00\x00\x01\x00"
/* 0000 */ "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52" //.PNG........IHDR
/* 0010 */ "\x00\x00\x00\x14\x00\x00\x00\x0a\x08\x02\x00\x00\x00\x3b\x37\xe9" //.............;7.
/* 0020 */ "\xb1\x00\x00\x00\x0f\x49\x44\x41\x54\x28\x91\x63\x60\x18\x05\xa3" //.....IDAT(.c`...
/* 0030 */ "\x80\x96\x00\x00\x02\x62\x00\x01\xfc\x4c\x5e\xbd\x00\x00\x00\x00" //.....b...L^.....
/* 0040 */ "\x49\x45\x4e\x44\xae\x42\x60\x82"                                 //IEND.B`.

    /* 0000 */ "\xf0\x03\x10\x00\x00\x00\x01\x00" // 03F0: TIMESTAMP 0010: chunk_len=16 0001: 1 order
    /* 0000 */ "\x00\xCA\x9A\x3B\x00\x00\x00\x00" // 0x000000003B9ACA00 = 1000000000
    /* 0000 */ "\x00\x00\x1e\x00\x00\x00\x03\x00" // 0000: ORDERS  001A:chunk_len=26 0002: 2 orders
    /* 0000 */ "\x19\x0a\x1c\x14\x0a\xff"             // RED rect
    /* 0000 */ "\x11\x5f\x05\x05\xF6\xf9\x00\xFF\x11" // BLUE RECT
    /* 0000 */ "\x3f\x05\xfb\xf7\x07\xff\xff"         // WHITE RECT

    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a"                                 //.PNG....
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x0d\x49\x48\x44\x52"                                 //....IHDR
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x14\x00\x00\x00\x0a"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x08\x02\x00\x00\x00\x3b\x37\xe9"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xb1\x00\x00\x00\x32\x49\x44\x41"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x54\x28\x91\x63\xfc\xcf\x80\x17"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xfc\xff\xcf\xc0\xc8\x88\x4b\x92"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x09\xbf\x5e\xfc\x60\x88\x6a\x66"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x41\xe3\x33\x32\xa0\x84\xe0\x7f"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x54\x91\xff\x0c\x28\x81\x37\x70"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xce\x66\x1c\xb0\x78\x06\x00\x69"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xdc\x0a\x12\x86\x4a\x0c\x44\x00"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x49\x45\x4e\x44\xae"
    /* 0000 */ "\x00\x10\x0b\x00\x00\x00\x01\x00" // 0x1000: FINAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x42\x60\x82"
        ;

    // Timestamps are applied only when flushing
    struct timeval now;
    now.tv_usec = 0;
    now.tv_sec = 1000;

    Rect scr(0, 0, 20, 10);
    CheckTransport trans(expected, sizeof(expected)-1, 511);
    Inifile ini;
    BmpCache bmp_cache(24, 600, 256, 300, 1024, 262, 4096);
    RDPDrawable drawable(scr.cx, scr.cy);
    GraphicToFile consumer(now, &trans, scr.cx, scr.cy, 24, bmp_cache, drawable, ini);
    consumer.draw(RDPOpaqueRect(scr, RED), scr);
    consumer.draw(RDPOpaqueRect(Rect(5, 5, 10, 3), BLUE), scr);
    consumer.draw(RDPOpaqueRect(Rect(10, 0, 1, 10), WHITE), scr);
    consumer.flush();

    OutChunkedBufferingTransport<16> png_trans(&trans);

    consumer.drawable.dump_png24(&png_trans, true);

}

BOOST_AUTO_TEST_CASE(TestReadPNGFromTransport)
{
    const char source_png[] =
        "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a"                                 //.PNG....
        "\x00\x00\x00\x0d\x49\x48\x44\x52"                                 //....IHDR
        "\x00\x00\x00\x14\x00\x00\x00\x0a\x08\x02\x00\x00\x00"             //.............
        "\x3b\x37\xe9\xb1"                                                 //;7..
        "\x00\x00\x00\x32\x49\x44\x41\x54"                                 //...2IDAT
        "\x28\x91\x63\xfc\xcf\x80\x17\xfc\xff\xcf\xc0\xc8\x88\x4b\x92\x09" //(.c..........K..
        "\xbf\x5e\xfc\x60\x88\x6a\x66\x41\xe3\x33\x32\xa0\x84\xe0\x7f\x54" //.^.`.jfA.32....T
        "\x91\xff\x0c\x28\x81\x37\x70\xce\x66\x1c\xb0\x78\x06\x00\x69\xdc" //...(.7p.f..x..i.
        "\x0a\x12"                                                         //..
        "\x86\x4a\x0c\x44"                                                 //.J.D
        "\x00\x00\x00\x00\x49\x45\x4e\x44"                                 //....IEND
        "\xae\x42\x60\x82"                                                 //.B`.
    ;

    RDPDrawable d(20, 10);
    GeneratorTransport in_png_trans(source_png, sizeof(source_png)-1);
    ::transport_read_png24(&in_png_trans, d.drawable.data,
                 d.drawable.width, d.drawable.height,
                 d.drawable.rowsize
                );
    const int groupid = 0;
    OutFilenameTransport png_trans(SQF_PATH_FILE_PID_COUNT_EXTENSION, "./", "testimg", ".png", groupid);
    d.dump_png24(&png_trans, true);
    sq_outfilename_unlink(&(png_trans.seq), 0);
}


BOOST_AUTO_TEST_CASE(TestReadPNGFromChunkedTransport)
{
    const char source_png[] =
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a"                                 //.PNG....
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x0d\x49\x48\x44\x52"                                 //....IHDR
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x14\x00\x00\x00\x0a"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x08\x02\x00\x00\x00\x3b\x37\xe9"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xb1\x00\x00\x00\x32\x49\x44\x41"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x54\x28\x91\x63\xfc\xcf\x80\x17"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xfc\xff\xcf\xc0\xc8\x88\x4b\x92"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x09\xbf\x5e\xfc\x60\x88\x6a\x66"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x41\xe3\x33\x32\xa0\x84\xe0\x7f"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x54\x91\xff\x0c\x28\x81\x37\x70"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xce\x66\x1c\xb0\x78\x06\x00\x69"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xdc\x0a\x12\x86\x4a\x0c\x44\x00"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x49\x45\x4e\x44\xae"
    /* 0000 */ "\x00\x10\x0b\x00\x00\x00\x01\x00" // 0x1000: FINAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x42\x60\x82"
    ;

    GeneratorTransport in_png_trans(source_png, sizeof(source_png)-1);
    BStream stream(8);
    in_png_trans.recv(&stream.end, 8); // skip first chunk header

//    in_png_trans.recv(&stream.end, 107); // skip first chunk header

    uint16_t chunk_type = stream.in_uint16_le();
    uint32_t chunk_size = stream.in_uint32_le();
    uint16_t chunk_count = stream.in_uint16_le();
    (void)chunk_count;

    InChunkedImageTransport chunk_trans(chunk_type, chunk_size, &in_png_trans);


    RDPDrawable d(20, 10);
    ::transport_read_png24(&chunk_trans, d.drawable.data,
                 d.drawable.width, d.drawable.height,
                 d.drawable.rowsize
                 );
    const int groupid = 0;
    OutFilenameTransport png_trans(SQF_PATH_FILE_PID_COUNT_EXTENSION, "./", "testimg", ".png", groupid);
    d.dump_png24(&png_trans, true);
    sq_outfilename_unlink(&(png_trans.seq), 0);
}

BOOST_AUTO_TEST_CASE(TestExtractPNGImagesFromWRM)
{
   const char source_wrm[] =
    /* 0000 */ "\xEE\x03\x1C\x00\x00\x00\x01\x00" // 03EE: META 0010: chunk_len=16 0001: 1 order
               "\x03\x00\x14\x00\x0A\x00\x18\x00" // WRM version 3, width = 20, height=10, bpp=24 PAD: 2 bytes
               "\x58\x02\x00\x01\x2c\x01\x00\x04\x06\x01\x00\x10"

// Initial black PNG image
/* 0000 */ "\x00\x10\x50\x00\x00\x00\x01\x00"
/* 0000 */ "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52" //.PNG........IHDR
/* 0010 */ "\x00\x00\x00\x14\x00\x00\x00\x0a\x08\x02\x00\x00\x00\x3b\x37\xe9" //.............;7.
/* 0020 */ "\xb1\x00\x00\x00\x0f\x49\x44\x41\x54\x28\x91\x63\x60\x18\x05\xa3" //.....IDAT(.c`...
/* 0030 */ "\x80\x96\x00\x00\x02\x62\x00\x01\xfc\x4c\x5e\xbd\x00\x00\x00\x00" //.....b...L^.....
/* 0040 */ "\x49\x45\x4e\x44\xae\x42\x60\x82"                                 //IEND.B`.

    /* 0000 */ "\xf0\x03\x10\x00\x00\x00\x01\x00" // 03F0: TIMESTAMP 0010: chunk_len=16 0001: 1 order
    /* 0000 */ "\x00\xCA\x9A\x3B\x00\x00\x00\x00" // 0x000000003B9ACA00 = 1000000000
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a"                                 //.PNG....
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x0d\x49\x48\x44\x52"                                 //....IHDR
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x14\x00\x00\x00\x0a"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x08\x02\x00\x00\x00\x3b\x37\xe9"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xb1\x00\x00\x00\x32\x49\x44\x41"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x54\x28\x91\x63\xfc\xcf\x80\x17"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xfc\xff\xcf\xc0\xc8\x88\x4b\x92"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x09\xbf\x5e\xfc\x60\x88\x6a\x66"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x41\xe3\x33\x32\xa0\x84\xe0\x7f"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x54\x91\xff\x0c\x28\x81\x37\x70"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xce\x66\x1c\xb0\x78\x06\x00\x69"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xdc\x0a\x12\x86\x4a\x0c\x44\x00"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x49\x45\x4e\x44\xae"
    /* 0000 */ "\x00\x10\x0b\x00\x00\x00\x01\x00" // 0x1000: FINAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x42\x60\x82"
        ;

    GeneratorTransport in_wrm_trans(source_wrm, sizeof(source_wrm)-1);
    timeval begin_capture;
    begin_capture.tv_sec = 0; begin_capture.tv_usec = 0;
    timeval end_capture;
    end_capture.tv_sec = 0; end_capture.tv_usec = 0;
    FileToGraphic player(&in_wrm_trans, begin_capture, end_capture, false, 0);

    const int groupid = 0;
    OutFilenameTransport out_png_trans(SQF_PATH_FILE_PID_COUNT_EXTENSION, "./", "testimg", ".png", groupid);
    RDPDrawable drawable(player.screen_rect.cx, player.screen_rect.cy);
    ImageCapture png_recorder(out_png_trans, player.screen_rect.cx, player.screen_rect.cy, drawable.drawable);

    player.add_consumer(&drawable);
    BOOST_CHECK_EQUAL(1, player.nbconsumers);
    while (player.next_order()){
        player.interpret_order();
    }
    png_recorder.flush();
    BOOST_CHECK_EQUAL(107, sq_outfilename_filesize(&(out_png_trans.seq), 0));
    sq_outfilename_unlink(&(out_png_trans.seq), 0);
}


BOOST_AUTO_TEST_CASE(TestExtractPNGImagesFromWRMTwoConsumers)
{
   const char source_wrm[] =
    /* 0000 */ "\xEE\x03\x1C\x00\x00\x00\x01\x00" // 03EE: META 0010: chunk_len=16 0001: 1 order
               "\x03\x00\x14\x00\x0A\x00\x18\x00" // WRM version 3, width = 20, height=10, bpp=24
               "\x58\x02\x00\x01\x2c\x01\x00\x04\x06\x01\x00\x10"

// Initial black PNG image
/* 0000 */ "\x00\x10\x50\x00\x00\x00\x01\x00"
/* 0000 */ "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52" //.PNG........IHDR
/* 0010 */ "\x00\x00\x00\x14\x00\x00\x00\x0a\x08\x02\x00\x00\x00\x3b\x37\xe9" //.............;7.
/* 0020 */ "\xb1\x00\x00\x00\x0f\x49\x44\x41\x54\x28\x91\x63\x60\x18\x05\xa3" //.....IDAT(.c`...
/* 0030 */ "\x80\x96\x00\x00\x02\x62\x00\x01\xfc\x4c\x5e\xbd\x00\x00\x00\x00" //.....b...L^.....
/* 0040 */ "\x49\x45\x4e\x44\xae\x42\x60\x82"                                 //IEND.B`.


    /* 0000 */ "\xf0\x03\x10\x00\x00\x00\x01\x00" // 03F0: TIMESTAMP 0010: chunk_len=16 0001: 1 order
    /* 0000 */ "\x00\xCA\x9A\x3B\x00\x00\x00\x00" // 0x000000003B9ACA00 = 1000000000
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a"                                 //.PNG....
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x0d\x49\x48\x44\x52"                                 //....IHDR
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x14\x00\x00\x00\x0a"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x08\x02\x00\x00\x00\x3b\x37\xe9"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xb1\x00\x00\x00\x32\x49\x44\x41"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x54\x28\x91\x63\xfc\xcf\x80\x17"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xfc\xff\xcf\xc0\xc8\x88\x4b\x92"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x09\xbf\x5e\xfc\x60\x88\x6a\x66"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x41\xe3\x33\x32\xa0\x84\xe0\x7f"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x54\x91\xff\x0c\x28\x81\x37\x70"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xce\x66\x1c\xb0\x78\x06\x00\x69"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xdc\x0a\x12\x86\x4a\x0c\x44\x00"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x49\x45\x4e\x44\xae"
    /* 0000 */ "\x00\x10\x0b\x00\x00\x00\x01\x00" // 0x1000: FINAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x42\x60\x82"
        ;

    GeneratorTransport in_wrm_trans(source_wrm, sizeof(source_wrm)-1);
    timeval begin_capture;
    begin_capture.tv_sec = 0; begin_capture.tv_usec = 0;
    timeval end_capture;
    end_capture.tv_sec = 0; end_capture.tv_usec = 0;
    FileToGraphic player(&in_wrm_trans, begin_capture, end_capture, false, 0);
    const int groupid = 0;
    OutFilenameTransport out_png_trans(SQF_PATH_FILE_PID_COUNT_EXTENSION, "./", "testimg", ".png", groupid);
    RDPDrawable drawable1(player.screen_rect.cx, player.screen_rect.cy);
    ImageCapture png_recorder(out_png_trans, player.screen_rect.cx, player.screen_rect.cy, drawable1.drawable);

    OutFilenameTransport second_out_png_trans(SQF_PATH_FILE_PID_COUNT_EXTENSION, "./", "second_testimg", ".png", groupid);
    ImageCapture second_png_recorder(second_out_png_trans, player.screen_rect.cx, player.screen_rect.cy, drawable1.drawable);

    player.add_consumer(&drawable1);
    BOOST_CHECK_EQUAL(1, player.nbconsumers);
    while (player.next_order()){
        player.interpret_order();
    }
    png_recorder.flush();
    BOOST_CHECK_EQUAL(107, sq_outfilename_filesize(&(out_png_trans.seq), 0));
    sq_outfilename_unlink(&(out_png_trans.seq), 0);

    second_png_recorder.flush();
    BOOST_CHECK_EQUAL(107, sq_outfilename_filesize(&(second_out_png_trans.seq), 0));
    sq_outfilename_unlink(&(second_out_png_trans.seq), 0);
}


BOOST_AUTO_TEST_CASE(TestExtractPNGImagesThenSomeOtherChunk)
{
   const char source_wrm[] =
    /* 0000 */ "\xEE\x03\x1C\x00\x00\x00\x01\x00" // 03EE: META 0010: chunk_len=16 0001: 1 order
               "\x03\x00\x14\x00\x0A\x00\x18\x00" // WRM version 3, width = 20, height=10, bpp=24
               "\x58\x02\x00\x01\x2c\x01\x00\x04\x06\x01\x00\x10"

// Initial black PNG image
/* 0000 */ "\x00\x10\x50\x00\x00\x00\x01\x00"
/* 0000 */ "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52" //.PNG........IHDR
/* 0010 */ "\x00\x00\x00\x14\x00\x00\x00\x0a\x08\x02\x00\x00\x00\x3b\x37\xe9" //.............;7.
/* 0020 */ "\xb1\x00\x00\x00\x0f\x49\x44\x41\x54\x28\x91\x63\x60\x18\x05\xa3" //.....IDAT(.c`...
/* 0030 */ "\x80\x96\x00\x00\x02\x62\x00\x01\xfc\x4c\x5e\xbd\x00\x00\x00\x00" //.....b...L^.....
/* 0040 */ "\x49\x45\x4e\x44\xae\x42\x60\x82"                                 //IEND.B`.

    /* 0000 */ "\xf0\x03\x10\x00\x00\x00\x01\x00" // 03F0: TIMESTAMP 0010: chunk_len=16 0001: 1 order
    /* 0000 */ "\x00\xCA\x9A\x3B\x00\x00\x00\x00" // 0x000000003B9ACA00 = 1000000000
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a"                                 //.PNG....
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x0d\x49\x48\x44\x52"                                 //....IHDR
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x14\x00\x00\x00\x0a"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x08\x02\x00\x00\x00\x3b\x37\xe9"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xb1\x00\x00\x00\x32\x49\x44\x41"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x54\x28\x91\x63\xfc\xcf\x80\x17"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xfc\xff\xcf\xc0\xc8\x88\x4b\x92"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x09\xbf\x5e\xfc\x60\x88\x6a\x66"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x41\xe3\x33\x32\xa0\x84\xe0\x7f"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x54\x91\xff\x0c\x28\x81\x37\x70"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xce\x66\x1c\xb0\x78\x06\x00\x69"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\xdc\x0a\x12\x86\x4a\x0c\x44\x00"
    /* 0000 */ "\x01\x10\x10\x00\x00\x00\x01\x00" // 0x1000: PARTIAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x00\x00\x00\x49\x45\x4e\x44\xae"
    /* 0000 */ "\x00\x10\x0b\x00\x00\x00\x01\x00" // 0x1000: FINAL_IMAGE_CHUNK 0048: chunk_len=100 0001: 1 order
        "\x42\x60\x82"
    /* 0000 */ "\xf0\x03\x10\x00\x00\x00\x01\x00" // 03F0: TIMESTAMP 0010: chunk_len=16 0001: 1 order
    /* 0000 */ "\x00\xD3\xD7\x3B\x00\x00\x00\x00" // 0x000000003bd7d300 = 1004000000
       ;

    GeneratorTransport in_wrm_trans(source_wrm, sizeof(source_wrm)-1);
    timeval begin_capture;
    begin_capture.tv_sec = 0; begin_capture.tv_usec = 0;
    timeval end_capture;
    end_capture.tv_sec = 0; end_capture.tv_usec = 0;
    FileToGraphic player(&in_wrm_trans, begin_capture, end_capture, false, 0);
    const int groupid = 0;
    OutFilenameTransport out_png_trans(SQF_PATH_FILE_PID_COUNT_EXTENSION, "./", "testimg", ".png", groupid);
    RDPDrawable drawable(player.screen_rect.cx, player.screen_rect.cy);
    ImageCapture png_recorder(out_png_trans, player.screen_rect.cx, player.screen_rect.cy, drawable.drawable);

    player.add_consumer(&drawable);
    while (player.next_order()){
        player.interpret_order();
    }
    png_recorder.flush();
    BOOST_CHECK_EQUAL((unsigned)1004, (unsigned)player.synctime_now.tv_sec);

    BOOST_CHECK_EQUAL((unsigned)107, sq_outfilename_filesize(&(out_png_trans.seq), 0));
    sq_outfilename_unlink(&(out_png_trans.seq), 0);
}
