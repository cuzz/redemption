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

   MCS Channel Management, complies with T.125

*/

#if !defined(__CORE_RDP_MCS_HPP__)
#define __CORE_RDP_MCS_HPP__

#include "RDP/x224.hpp"
#include "channel_list.hpp"


class McsOut
{
    Stream & stream;
    uint8_t offlen;
    public:
    McsOut(Stream & stream, uint8_t command, uint8_t user_id, uint16_t chan_id)
        : stream(stream), offlen(stream.p - stream.data + 6)
    {
        stream.out_uint8(command << 2);
        stream.out_uint16_be(user_id);
        stream.out_uint16_be(chan_id);
        stream.out_uint8(0x70);
        stream.skip_uint8(2); //len
    }

    void end(){
        int len = stream.p - stream.data - offlen - 2;
        stream.set_out_uint16_be(0x8000|len, this->offlen);
    }
};


class McsIn
{
    public:
    uint8_t opcode;
    uint16_t user_id;
    uint16_t chan_id;
    uint8_t magic_0x70; // some ber header ?
    uint16_t len;

    McsIn(Stream & stream)
    {
        this->opcode = stream.in_uint8();
        this->user_id = stream.in_uint16_be();
        this->chan_id = stream.in_uint16_be();
        this->magic_0x70 = stream.in_uint8();
        this->len = stream.in_uint8();
        if (this->len & 0x80){
            this->len = ((this->len & 0x7F) << 8) + stream.in_uint8();
        }
    }

    void end(){
        #warning put some assertion here to ensure all data has been consumed
    }

};

// 2.2.1.3 Client MCS Connect Initial PDU with GCC Conference Create Request
// =========================================================================

// The MCS Connect Initial PDU is an RDP Connection Sequence PDU sent from
// client to server during the Basic Settings Exchange phase (see section
// 1.3.1.1). It is sent after receiving the X.224 Connection Confirm PDU
// (section 2.2.1.2). The MCS Connect Initial PDU encapsulates a GCC Conference
// Create Request, which encapsulates concatenated blocks of settings data. A
// basic high-level overview of the nested structure for the Client MCS Connect
// Initial PDU is illustrated in section 1.3.1.1, in the figure specifying MCS
// Connect Initial PDU. Note that the order of the settings data blocks is
// allowed to vary from that shown in the previously mentioned figure and the
// message syntax layout that follows. This is possible because each data block
// is identified by a User Data Header structure (section 2.2.1.3.1).

// tpktHeader (4 bytes): A TPKT Header, as specified in [T123] section 8.

// x224Data (3 bytes): An X.224 Class 0 Data TPDU, as specified in [X224]
//   section 13.7.

// mcsCi (variable): Variable-length BER-encoded MCS Connect Initial structure
//   (using definite-length encoding) as described in [T125] (the ASN.1
//   structure definition is detailed in [T125] section 7, part 2). The userData
//   field of the MCS Connect Initial encapsulates the GCC Conference Create
//   Request data (contained in the gccCCrq and subsequent fields). The maximum
//   allowed size of this user data is 1024 bytes, which implies that the
//   combined size of the gccCCrq and subsequent fields MUST be less than 1024
//   bytes.

// gccCCrq (variable): Variable-length Packed Encoding Rule encoded
//   (PER-encoded) GCC Connect Data structure, which encapsulates a Connect GCC
//   PDU that contains a GCC Conference Create Request structure as described in
//   [T124] (the ASN.1 structure definitions are detailed in [T124] section 8.7)
//   appended as user data to the MCS Connect Initial (using the format
//   described in [T124] sections 9.5 and 9.6). The userData field of the GCC
//   Conference Create Request contains one user data set consisting of
//   concatenated client data blocks.

// clientCoreData (216 bytes): Client Core Data structure (section 2.2.1.3.2).

// clientSecurityData (12 bytes): Client Security Data structure (section
//   2.2.1.3.3).

// clientNetworkData (variable): Optional and variable-length Client Network
//   Data structure (section 2.2.1.3.4).

// clientClusterData (12 bytes): Optional Client Cluster Data structure (section
//   2.2.1.3.5).

// clientMonitorData (variable): Optional Client Monitor Data structure (section
//   2.2.1.3.6). This field MUST NOT be included if the server does not
//   advertise support for extended client data blocks by using the
//   EXTENDED_CLIENT_DATA_SUPPORTED flag (0x00000001) as described in section
//   2.2.1.2.1.

#warning create McsConnectionInitialIn and McsConnectionInitialOut classes instead of recv_ and send_ functions below (it would also much simplify length management) and allow inlining data part instead of preparint it in a separate buffer.


inline static void send_connection_initial(Transport * trans, Stream & data)
{
    int data_len = data.end - data.data;
    int len = 7 + 3 * 34 + 4 + data_len;

    Stream ci_stream(8192);
    X224Out ci_tpdu(X224Packet::DT_TPDU, ci_stream);

    ci_stream.out_uint16_be(BER_TAG_MCS_CONNECT_INITIAL);
    ci_stream.out_ber_len(len);
    ci_stream.out_uint8(BER_TAG_OCTET_STRING);
    ci_stream.out_ber_len(0); /* calling domain */
    ci_stream.out_uint8(BER_TAG_OCTET_STRING);
    ci_stream.out_ber_len(0); /* called domain */
    ci_stream.out_uint8(BER_TAG_BOOLEAN);
    ci_stream.out_ber_len(1);
    ci_stream.out_uint8(0xff); /* upward flag */

    // target params
    ci_stream.out_uint8(BER_TAG_MCS_DOMAIN_PARAMS);
    ci_stream.out_ber_len(32);
    ci_stream.out_ber_int16(34);     // max_channels
    ci_stream.out_ber_int16(2);      // max_users
    ci_stream.out_ber_int16(0);      // max_tokens
    ci_stream.out_ber_int16(1);
    ci_stream.out_ber_int16(0);
    ci_stream.out_ber_int16(1);
    ci_stream.out_ber_int16(0xffff); // max_pdu_size
    ci_stream.out_ber_int16(2);

    // min params
    ci_stream.out_uint8(BER_TAG_MCS_DOMAIN_PARAMS);
    ci_stream.out_ber_len(32);
    ci_stream.out_ber_int16(1);     // max_channels
    ci_stream.out_ber_int16(1);     // max_users
    ci_stream.out_ber_int16(1);     // max_tokens
    ci_stream.out_ber_int16(1);
    ci_stream.out_ber_int16(0);
    ci_stream.out_ber_int16(1);
    ci_stream.out_ber_int16(0x420); // max_pdu_size
    ci_stream.out_ber_int16(2);

    // max params
    ci_stream.out_uint8(BER_TAG_MCS_DOMAIN_PARAMS);
    ci_stream.out_ber_len(32);
    ci_stream.out_ber_int16(0xffff); // max_channels
    ci_stream.out_ber_int16(0xfc17); // max_users
    ci_stream.out_ber_int16(0xffff); // max_tokens
    ci_stream.out_ber_int16(1);
    ci_stream.out_ber_int16(0);
    ci_stream.out_ber_int16(1);
    ci_stream.out_ber_int16(0xffff); // max_pdu_size
    ci_stream.out_ber_int16(2);

    ci_stream.out_uint8(BER_TAG_OCTET_STRING);
    ci_stream.out_ber_len(data_len);
    ci_stream.out_copy_bytes(data.data, data_len);

    ci_tpdu.end();
    ci_tpdu.send(trans);
}



// 2.2.1.3.2 Client Core Data (TS_UD_CS_CORE)
// -------------------------------------
// Below relevant quotes from MS-RDPBCGR v20100601 (2.2.1.3.2)

// header (4 bytes): GCC user data block header, as specified in section
//                   2.2.1.3.1. The User Data Header type field MUST be
//                   set to CS_CORE (0xC001).

// version (4 bytes): A 32-bit, unsigned integer. Client version number
//                    for the RDP. The major version number is stored in
//                    the high 2 bytes, while the minor version number
//                    is stored in the low 2 bytes.
//
//         Value Meaning
//         0x00080001 RDP 4.0 clients
//         0x00080004 RDP 5.0, 5.1, 5.2, 6.0, 6.1, and 7.0 clients

// desktopWidth (2 bytes): A 16-bit, unsigned integer. The requested
//                         desktop width in pixels (up to a maximum
//                         value of 4096 pixels).

// desktopHeight (2 bytes): A 16-bit, unsigned integer. The requested
//                         desktop height in pixels (up to a maximum
//                         value of 2048 pixels).

// colorDepth (2 bytes): A 16-bit, unsigned integer. The requested color
//                       depth. Values in this field MUST be ignored if
//                       the postBeta2ColorDepth field is present.
//          Value Meaning
//          RNS_UD_COLOR_4BPP 0xCA00 4 bits-per-pixel (bpp)
//          RNS_UD_COLOR_8BPP 0xCA01 8 bpp

// SASSequence (2 bytes): A 16-bit, unsigned integer. Secure access
//                        sequence. This field SHOULD be set to
//                        RNS_UD_SAS_DEL (0xAA03).

// keyboardLayout (4 bytes): A 32-bit, unsigned integer. Keyboard layout
//                           (active input locale identifier). For a
//                           list of possible input locales, see
//                           [MSDN-MUI].

// clientBuild (4 bytes): A 32-bit, unsigned integer. The build number
// of the client.

// clientName (32 bytes): Name of the client computer. This field
//                        contains up to 15 Unicode characters plus a
//                        null terminator.

// keyboardType (4 bytes): A 32-bit, unsigned integer. The keyboard type.
//              Value Meaning
//              0x00000001 IBM PC/XT or compatible (83-key) keyboard
//              0x00000002 Olivetti "ICO" (102-key) keyboard
//              0x00000003 IBM PC/AT (84-key) and similar keyboards
//              0x00000004 IBM enhanced (101-key or 102-key) keyboard
//              0x00000005 Nokia 1050 and similar keyboards
//              0x00000006 Nokia 9140 and similar keyboards
//              0x00000007 Japanese keyboard

// keyboardSubType (4 bytes): A 32-bit, unsigned integer. The keyboard
//                        subtype (an original equipment manufacturer-
//                        -dependent value).

// keyboardFunctionKey (4 bytes): A 32-bit, unsigned integer. The number
//                        of function keys on the keyboard.

// imeFileName (64 bytes): A 64-byte field. The Input Method Editor
//                        (IME) file name associated with the input
//                        locale. This field contains up to 31 Unicode
//                        characters plus a null terminator.

// --> Note By CGR How do we know that the following fields are
//     present of Not ? The only rational method I see is to look
//     at the length field in the preceding User Data Header
//     120 bytes without optional data
//     216 bytes with optional data present

// postBeta2ColorDepth (2 bytes): A 16-bit, unsigned integer. The
//                        requested color depth. Values in this field
//                        MUST be ignored if the highColorDepth field
//                        is present.
//       Value Meaning
//       RNS_UD_COLOR_4BPP 0xCA00        : 4 bits-per-pixel (bpp)
//       RNS_UD_COLOR_8BPP 0xCA01        : 8 bpp
//       RNS_UD_COLOR_16BPP_555 0xCA02   : 15-bit 555 RGB mask
//                                         (5 bits for red, 5 bits for
//                                         green, and 5 bits for blue)
//       RNS_UD_COLOR_16BPP_565 0xCA03   : 16-bit 565 RGB mask
//                                         (5 bits for red, 6 bits for
//                                         green, and 5 bits for blue)
//       RNS_UD_COLOR_24BPP 0xCA04       : 24-bit RGB mask
//                                         (8 bits for red, 8 bits for
//                                         green, and 8 bits for blue)
// If this field is present, all of the preceding fields MUST also be
// present. If this field is not present, all of the subsequent fields
// MUST NOT be present.

// clientProductId (2 bytes): A 16-bit, unsigned integer. The client
//                          product ID. This field SHOULD be initialized
//                          to 1. If this field is present, all of the
//                          preceding fields MUST also be present. If
//                          this field is not present, all of the
//                          subsequent fields MUST NOT be present.

// serialNumber (4 bytes): A 32-bit, unsigned integer. Serial number.
//                         This field SHOULD be initialized to 0. If
//                         this field is present, all of the preceding
//                         fields MUST also be present. If this field
//                         is not present, all of the subsequent fields
//                         MUST NOT be present.

// highColorDepth (2 bytes): A 16-bit, unsigned integer. The requested
//                         color depth.
//          Value Meaning
// HIGH_COLOR_4BPP  0x0004             : 4 bpp
// HIGH_COLOR_8BPP  0x0008             : 8 bpp
// HIGH_COLOR_15BPP 0x000F             : 15-bit 555 RGB mask
//                                       (5 bits for red, 5 bits for
//                                       green, and 5 bits for blue)
// HIGH_COLOR_16BPP 0x0010             : 16-bit 565 RGB mask
//                                       (5 bits for red, 6 bits for
//                                       green, and 5 bits for blue)
// HIGH_COLOR_24BPP 0x0018             : 24-bit RGB mask
//                                       (8 bits for red, 8 bits for
//                                       green, and 8 bits for blue)
//
// If this field is present, all of the preceding fields MUST also be
// present. If this field is not present, all of the subsequent fields
// MUST NOT be present.

// supportedColorDepths (2 bytes): A 16-bit, unsigned integer. Specifies
//                                 the high color depths that the client
//                                 is capable of supporting.
//
//         Flag Meaning
//   RNS_UD_24BPP_SUPPORT 0x0001       : 24-bit RGB mask
//                                       (8 bits for red, 8 bits for
//                                       green, and 8 bits for blue)
//   RNS_UD_16BPP_SUPPORT 0x0002       : 16-bit 565 RGB mask
//                                       (5 bits for red, 6 bits for
//                                       green, and 5 bits for blue)
//   RNS_UD_15BPP_SUPPORT 0x0004       : 15-bit 555 RGB mask
//                                       (5 bits for red, 5 bits for
//                                       green, and 5 bits for blue)
//   RNS_UD_32BPP_SUPPORT 0x0008       : 32-bit RGB mask
//                                       (8 bits for the alpha channel,
//                                       8 bits for red, 8 bits for
//                                       green, and 8 bits for blue)
// If this field is present, all of the preceding fields MUST also be
// present. If this field is not present, all of the subsequent fields
// MUST NOT be present.

// earlyCapabilityFlags (2 bytes)      : A 16-bit, unsigned integer. It
//                                       specifies capabilities early in
//                                       the connection sequence.
//        Flag                        Meaning
//  RNS_UD_CS_SUPPORT_ERRINFO_PDU Indicates that the client supports
//    0x0001                        the Set Error Info PDU
//                                 (section 2.2.5.1).
//
//  RNS_UD_CS_WANT_32BPP_SESSION Indicates that the client is requesting
//    0x0002                     a session color depth of 32 bpp. This
//                               flag is necessary because the
//                               highColorDepth field does not support a
//                               value of 32. If this flag is set, the
//                               highColorDepth field SHOULD be set to
//                               24 to provide an acceptable fallback
//                               for the scenario where the server does
//                               not support 32 bpp color.
//
//  RNS_UD_CS_SUPPORT_STATUSINFO_PDU  Indicates that the client supports
//    0x0004                          the Server Status Info PDU
//                                    (section 2.2.5.2).
//
//  RNS_UD_CS_STRONG_ASYMMETRIC_KEYS  Indicates that the client supports
//    0x0008                          asymmetric keys larger than
//                                    512 bits for use with the Server
//                                    Certificate (section 2.2.1.4.3.1)
//                                    sent in the Server Security Data
//                                    block (section 2.2.1.4.3).
//
//  RNS_UD_CS_VALID_CONNECTION_TYPE Indicates that the connectionType
//     0x0020                       field contains valid data.
//
//  RNS_UD_CS_SUPPORT_MONITOR_LAYOUT_PDU Indicates that the client
//     0x0040                            supports the Monitor Layout PDU
//                                       (section 2.2.12.1).
//
// If this field is present, all of the preceding fields MUST also be
// present. If this field is not present, all of the subsequent fields
// MUST NOT be present.

// clientDigProductId (64 bytes): Contains a value that uniquely
//                                identifies the client. If this field
//                                is present, all of the preceding
//                                fields MUST also be present. If this
//                                field is not present, all of the
//                                subsequent fields MUST NOT be present.

// connectionType (1 byte): An 8-bit unsigned integer. Hints at the type
//                      of network connection being used by the client.
//                      This field only contains valid data if the
//                      RNS_UD_CS_VALID_CONNECTION_TYPE (0x0020) flag
//                      is present in the earlyCapabilityFlags field.
//
//    Value                          Meaning
//  CONNECTION_TYPE_MODEM 0x01 : Modem (56 Kbps)
//  CONNECTION_TYPE_BROADBAND_LOW 0x02 : Low-speed broadband
//                                 (256 Kbps - 2 Mbps)
//  CONNECTION_TYPE_SATELLITE 0x03 : Satellite
//                                 (2 Mbps - 16 Mbps with high latency)
//  CONNECTION_TYPE_BROADBAND_HIGH 0x04 : High-speed broadband
//                                 (2 Mbps - 10 Mbps)
//  CONNECTION_TYPE_WAN 0x05 : WAN (10 Mbps or higher with high latency)
//  CONNECTION_TYPE_LAN 0x06 : LAN (10 Mbps or higher)

// If this field is present, all of the preceding fields MUST also be
// present. If this field is not present, all of the subsequent fields
// MUST NOT be present.

// pad1octet (1 byte): An 8-bit, unsigned integer. Padding to align the
//   serverSelectedProtocol field on the correct byte boundary. If this
//   field is present, all of the preceding fields MUST also be present.
//   If this field is not present, all of the subsequent fields MUST NOT
//   be present.

// serverSelectedProtocol (4 bytes): A 32-bit, unsigned integer that
//   contains the value returned by the server in the selectedProtocol
//   field of the RDP Negotiation Response (section 2.2.1.2.1). In the
//   event that an RDP Negotiation Response was not received from the
//   server, this field MUST be initialized to PROTOCOL_RDP (0). This
//   field MUST be present if an RDP Negotiation Request (section
//   2.2.1.1.1) was sent to the server. If this field is present,
//   then all of the preceding fields MUST also be present.

#warning use official field names from MS-RDPBCGR
static inline void parse_mcs_data_cs_core(Stream & stream, ClientInfo * client_info)
{
    LOG(LOG_INFO, "PARSE CS_CORE\n");
    uint16_t rdp_version = stream.in_uint16_le();
    LOG(LOG_INFO, "core_data: rdp_version (1=RDP1, 4=RDP5) %u\n", rdp_version);
    uint16_t dummy1 = stream.in_uint16_le();
    LOG(LOG_INFO, "core_data: ?? = %u\n", dummy1);
    client_info->width = stream.in_uint16_le();
    LOG(LOG_INFO, "core_data: width = %u\n", client_info->width);
    client_info->height = stream.in_uint16_le();
    LOG(LOG_INFO, "core_data: height = %u\n", client_info->height);
    uint16_t bpp_code = stream.in_uint16_le();
    LOG(LOG_INFO, "core_data: bpp_code = %x\n", bpp_code);
    uint16_t dummy2 = stream.in_uint16_le();
    LOG(LOG_INFO, "core_data: ?? = %x\n", dummy2);
    /* get keylayout */
    client_info->keylayout = stream.in_uint32_le();
    LOG(LOG_INFO, "core_data: layout = %x\n", client_info->keylayout);
    /* get build : windows build */
    client_info->build = stream.in_uint32_le();
    LOG(LOG_INFO, "core_data: build = %x\n", client_info->build);

    /* get hostname (it is UTF16, windows flavored widechars) */
    /* Unicode name of client is padded to 32 bytes */
    stream.in_uni_to_ascii_str(client_info->hostname, 32);
    LOG(LOG_INFO, "core_data: hostname = %s\n", client_info->hostname);

    uint32_t keyboard_type = stream.in_uint32_le();
    LOG(LOG_INFO, "core_data: keyboard_type = %x\n", keyboard_type);
    uint32_t keyboard_subtype = stream.in_uint32_le();
    LOG(LOG_INFO, "core_data: keyboard_subtype = %x\n", keyboard_subtype);
    uint32_t keyboard_functionkeys = stream.in_uint32_le();
    LOG(LOG_INFO, "core_data: keyboard_functionkeys = %x\n", keyboard_functionkeys);
    stream.skip_uint8(64);

    client_info->bpp = 8;
    int i = stream.in_uint16_le();
    switch (i) {
    case 0xca01:
    {
        uint16_t clientProductId = stream.in_uint16_le();
        uint32_t serialNumber = stream.in_uint32_le();
        uint16_t rdp_bpp = stream.in_uint16_le();
        uint16_t supportedColorDepths = stream.in_uint16_le();
        if (rdp_bpp <= 24){
            client_info->bpp = rdp_bpp;
        }
        else {
            client_info->bpp = 24;
        }
    }
        break;
    case 0xca02:
        client_info->bpp = 15;
        break;
    case 0xca03:
        client_info->bpp = 16;
        break;
    case 0xca04:
        client_info->bpp = 24;
        break;
    }
    LOG(LOG_INFO, "core_data: bpp = %u\n", client_info->bpp);
}

// 2.2.1.3.3 Client Security Data (TS_UD_CS_SEC)
// ---------------------------------------------
// The TS_UD_CS_SEC data block contains security-related information used to
// advertise client cryptographic support. This information is only relevant
// when Standard RDP Security mechanisms (section 5.3) will be used. See
// sections 3 and 5.3.2 for a detailed discussion of how this information is
// used.

// header (4 bytes): GCC user data block header as described in User Data
//                   Header (section 2.2.1.3.1). The User Data Header type
//                   field MUST be set to CS_SECURITY (0xC002).

// encryptionMethods (4 bytes): A 32-bit, unsigned integer. Cryptographic
//                              encryption methods supported by the client
//                              and used in conjunction with Standard RDP
//                              Security The server MUST select one of these
//                              methods. Section 5.3.2 describes how the
//                              client and server negotiate the security
//                              parameters for a given connection.
//
//           Value                           Meaning
// -------------------------------------------------------------------------
//    40BIT_ENCRYPTION_FLAG   40-bit session keys MUST be used to encrypt
//       0x00000001           data (with RC4) and generate Message
//                            Authentication Codes (MAC).
// -------------------------------------------------------------------------
//    128BIT_ENCRYPTION_FLAG  128-bit session keys MUST be used to encrypt
//       0x00000002           data (with RC4) and generate MACs.
// -------------------------------------------------------------------------
//    56BIT_ENCRYPTION_FLAG   56-bit session keys MUST be used to encrypt
//       0x00000008           data (with RC4) and generate MACs.
// -------------------------------------------------------------------------
//   FIPS_ENCRYPTION_FLAG All encryption and Message Authentication Code
//                            generation routines MUST be Federal
//       0x00000010           Information Processing Standard (FIPS) 140-1
//                            compliant.

// extEncryptionMethods (4 bytes): A 32-bit, unsigned integer. This field is
//                               used exclusively for the French locale.
//                               In French locale clients, encryptionMethods
//                               MUST be set to 0 and extEncryptionMethods
//                               MUST be set to the value to which
//                               encryptionMethods would have been set.
//                               For non-French locale clients, this field
//                               MUST be set to 0

static inline void parse_mcs_data_cs_security(Stream & stream)
{
    LOG(LOG_INFO, "CS_SECURITY\n");
}

// 2.2.1.3.4 Client Network Data (TS_UD_CS_NET)
// --------------------------------------------
// The TS_UD_CS_NET packet contains a list of requested virtual channels.

// header (4 bytes): A 32-bit, unsigned integer. GCC user data block header,
//                   as specified in User Data Header (section 2.2.1.3.1).
//                   The User Data Header type field MUST be set to CS_NET
//                   (0xC003).

// channelCount (4 bytes): A 32-bit, unsigned integer. The number of
//                         requested static virtual channels (the maximum
//                         allowed is 31).

// channelDefArray (variable): A variable-length array containing the
//                             information for requested static virtual
//                             channels encapsulated in CHANNEL_DEF
//                             structures (section 2.2.1.3.4.1). The number
//                             of CHANNEL_DEF structures which follows is
//                             given by the channelCount field.

// 2.2.1.3.4.1 Channel Definition Structure (CHANNEL_DEF)
// ------------------------------------------------------
// The CHANNEL_DEF packet contains information for a particular static
// virtual channel.

// name (8 bytes): An 8-byte array containing a null-terminated collection
//                 of seven ANSI characters that uniquely identify the
//                 channel.

// options (4 bytes): A 32-bit, unsigned integer. Channel option flags.
//
//           Flag                             Meaning
// -------------------------------------------------------------------------
// CHANNEL_OPTION_INITIALIZED   Absence of this flag indicates that this
//        0x80000000            channel is a placeholder and that the
//                              server MUST NOT set it up.
// ------------------------------------------------------------------------
// CHANNEL_OPTION_ENCRYPT_RDP   This flag is unused and its value MUST be
//        0x40000000            ignored by the server.
// -------------------------------------------------------------------------
// CHANNEL_OPTION_ENCRYPT_SC    This flag is unused and its value MUST be
//        0x20000000            ignored by the server.
// -------------------------------------------------------------------------
// CHANNEL_OPTION_ENCRYPT_CS    This flag is unused and its value MUST be
//        0x10000000            ignored by the server.
// -------------------------------------------------------------------------
// CHANNEL_OPTION_PRI_HIGH      Channel data MUST be sent with high MCS
//        0x08000000            priority.
// -------------------------------------------------------------------------
// CHANNEL_OPTION_PRI_MED       Channel data MUST be sent with medium
//        0x04000000            MCS priority.
// -------------------------------------------------------------------------
// CHANNEL_OPTION_PRI_LOW       Channel data MUST be sent with low MCS
//        0x02000000            priority.
// -------------------------------------------------------------------------
// CHANNEL_OPTION_COMPRESS_RDP  Virtual channel data MUST be compressed
//        0x00800000            if RDP data is being compressed.
// -------------------------------------------------------------------------
// CHANNEL_OPTION_COMPRESS      Virtual channel data MUST be compressed,
//        0x00400000            regardless of RDP compression settings.
// -------------------------------------------------------------------------
// CHANNEL_OPTION_SHOW_PROTOCOL The value of this flag MUST be ignored by
//        0x00200000            the server. The visibility of the Channel
//                              PDU Header (section 2.2.6.1.1) is
//                              determined by the CHANNEL_FLAG_SHOW_PROTOCOL
//                              (0x00000010) flag as defined in the flags
//                              field (section 2.2.6.1.1).
// -------------------------------------------------------------------------
//REMOTE_CONTROL_PERSISTENT     Channel MUST be persistent across remote
//                              control 0x00100000 transactions.


static inline void send_cs_net(Stream & stream, const ChannelList & channel_list)
{
    /* Here we need to put channel information in order to redirect channel data
    from client to server passing through the "proxy" */
    LOG(LOG_INFO, "sending channels information to server : %u channels", channel_list.size());

    size_t num_channels = channel_list.size();

    if (num_channels > 0) {
        LOG(LOG_INFO, "cs_net");
        stream.out_uint16_le(CS_NET);
        LOG(LOG_INFO, "cs_net::len=%u", num_channels * 12 + 8);
        stream.out_uint16_le(num_channels * 12 + 8); /* length */
        LOG(LOG_INFO, "cs_net::nb_chan=%u", num_channels);
        stream.out_uint32_le(num_channels); /* number of virtual channels */
        for (size_t index = 0; index < num_channels; index++){
            const McsChannelItem & channel_item = channel_list[index];
            stream.out_copy_bytes(channel_item.name, 8);
            stream.out_uint32_be(channel_item.flags);
            LOG(LOG_INFO, "channel[%u] (%s, %x, chanid=%u)", index, channel_list[index].name, channel_list[index].flags, channel_list[index].chanid);
        }
    }


}

// this adds the mcs channels in the list of channels to be used when
// creating the server mcs data
static inline void parse_mcs_data_cs_net(Stream & stream, ClientInfo * client_info, ChannelList & channel_list)
{
    LOG(LOG_INFO, "CS_NET\n");
//    // this is an option set in rdpproxy.ini
//    // to disable all channels (no clipboard, no device redirection, etc)
//    if (client_info->channel_code != 1) { /* are channels on? */
//        return;
//    }
    uint32_t channelCount = stream.in_uint32_le();
    LOG(LOG_INFO, "front:basic_settings:channel_list:cs_net:channel_count %u [%u]", channelCount, channel_list.size());

    for (uint32_t index = 0; index < channelCount; index++) {
        McsChannelItem channel_item;
        memcpy(channel_item.name, stream.in_uint8p(8), 8);
        channel_item.flags = stream.in_uint32_be();
        channel_item.chanid = MCS_GLOBAL_CHANNEL + (index + 1);
        #warning push_back is not the best choice here, as we have static space already available in channel_list, we could even let ChannelList manage parsing
        channel_list.push_back(channel_item);
        LOG(LOG_INFO, "channel[%u] (%s, %x, chanid=%u)", index, channel_list[index].name, channel_list[index].flags, channel_list[index].chanid);
    }
    LOG(LOG_INFO, "Number of virtual channels: %u", channel_list.size());
}


// 2.2.1.3.5 Client Cluster Data (TS_UD_CS_CLUSTER)
// ------------------------------------------------
// The TS_UD_CS_CLUSTER data block is sent by the client to the server either to advertise that it can
// support the Server Redirection PDUs (sections 2.2.13.2 and 2.2.13.3) or to request a connection to
// a given session identifier.

// header (4 bytes): GCC user data block header, as specified in User Data
//                   Header (section 2.2.1.3.1). The User Data Header type
//                   field MUST be set to CS_CLUSTER (0xC004).

// Flags (4 bytes): A 32-bit, unsigned integer. Cluster information flags.

//           Flag                            Meaning
// -------------------------------------------------------------------------
// REDIRECTION_SUPPORTED               The client can receive server session
//       0x00000001                    redirection packets. If this flag is
//                                     set, the
//                                     ServerSessionRedirectionVersionMask
//                                     MUST contain the server session
//                                     redirection version that the client
//                                     supports.
// -------------------------------------------------------------------------
// ServerSessionRedirectionVersionMask The server session redirection
//       0x0000003C                    version that the client supports.
//                                     See the discussion which follows
//                                     this table for more information.
// -------------------------------------------------------------------------
// REDIRECTED_SESSIONID_FIELD_VALID    The RedirectedSessionID field
//       0x00000002                    contains an ID that identifies a
//                                     session on the server to associate
//                                     with the connection.
// -------------------------------------------------------------------------
// REDIRECTED_SMARTCARD                The client logged on with a smart
//       0x00000040                    card.
// -------------------------------------------------------------------------

// The ServerSessionRedirectionVersionMask is a 4-bit enumerated value
// containing the server session redirection version supported by the
// client. The following are possible version values.

//          Value                              Meaning
// -------------------------------------------------------------------------
// REDIRECTION_VERSION3                If REDIRECTION_SUPPORTED is set,
//          0x02                       server session redirection version 3
//                                     is supported by the client.
// -------------------------------------------------------------------------
// REDIRECTION_VERSION4                If REDIRECTION_SUPPORTED is set,
//          0x03                       server session redirection version 4
//                                     is supported by the client.
// -------------------------------------------------------------------------
// REDIRECTION_VERSION5                If REDIRECTION_SUPPORTED is set,
//          0x04                       server session redirection version 5
//                                     is supported by the client.
// -------------------------------------------------------------------------

// The version values cannot be combined; only one value MUST be specified
// if the REDIRECTED_SESSIONID_FIELD_VALID (0x00000002) flag is present in
// the Flags field.

// RedirectedSessionID (4 bytes): A 32-bit unsigned integer. If the
//                                REDIRECTED_SESSIONID_FIELD_VALID flag is
//                                set in the Flags field, then the
//                                RedirectedSessionID field contains a valid
//                                session identifier to which the client
//                                requests to connect.

// This is this header that contains the console flag (undocumented ?)
static inline void parse_mcs_data_cs_cluster(Stream & stream, ClientInfo * client_info)
{
    LOG(LOG_INFO, "CS_CLUSTER\n");
    uint32_t flags = stream.in_uint32_le();
    LOG(LOG_INFO, "cluster_data: flags = %x\n", flags);
    client_info->console_session = (flags & 0x2) != 0;
}

// 2.2.1.3.6 Client Monitor Data (TS_UD_CS_MONITOR)
// ------------------------------------------------
// The TS_UD_CS_MONITOR packet describes the client-side display monitor
// layout. This packet is an Extended Client Data Block and MUST NOT be sent
// to a server which does not advertise support for Extended Client Data
// Blocks by using the EXTENDED_CLIENT_DATA_SUPPORTED flag (0x00000001) as
// described in section 2.2.1.2.1.

// header (4 bytes): GCC user data block header, as specified in User Data
//                   Header (section 2.2.1.3.1). The User Data Header type
//                   field MUST be set to CS_MONITOR (0xC005).

// flags (4 bytes): A 32-bit, unsigned integer. This field is unused and
//                  reserved for future use.

// monitorCount (4 bytes): A 32-bit, unsigned integer. The number of display
//                         monitor definitions in the monitorDefArray field
//                        (the maximum allowed is 16).

// monitorDefArray (variable): A variable-length array containing a series
//                             of TS_MONITOR_DEF structures (section
//                             2.2.1.3.6.1) which describe the display
//                             monitor layout of the client. The number of
//                             TS_MONITOR_DEF structures is given by the
//                             monitorCount field.


// 2.2.1.3.6.1 Monitor Definition (TS_MONITOR_DEF)
// -----------------------------------------------
// The TS_MONITOR_DEF packet describes the configuration of a client-side
// display monitor. The x and y coordinates used to describe the monitor
// position MUST be relative to the upper-left corner of the monitor
// designated as the "primary display monitor" (the upper-left corner of the
// primary monitor is always (0, 0)).

// left (4 bytes): A 32-bit, unsigned integer. Specifies the x-coordinate of
//                 the upper-left corner of the display monitor.

// top (4 bytes): A 32-bit, unsigned integer. Specifies the y-coordinate of
//                the upper-left corner of the display monitor.

// right (4 bytes): A 32-bit, unsigned integer. Specifies the x-coordinate
//                  of the lower-right corner of the display monitor.

// bottom (4 bytes): A 32-bit, unsigned integer. Specifies the y-coordinate
//                   of the lower-right corner of the display monitor.

// flags (4 bytes): A 32-bit, unsigned integer. Monitor configuration flags.

//        Value                          Meaning
// -------------------------------------------------------------------------
// TS_MONITOR_PRIMARY            The top, left, right and bottom fields
//      0x00000001               describe the position of the primary
//                               monitor.
// -------------------------------------------------------------------------

static inline void parse_mcs_data_cs_monitor(Stream & stream)
{
    LOG(LOG_INFO, "CS_MONITOR\n");
}

// 2.2.1.4.2 Server Core Data (TS_UD_SC_CORE)
static inline void parse_mcs_data_sc_core(Stream & stream)
{
    LOG(LOG_INFO, "SC_CORE\n");
}

// 2.2.1.4.3 Server Security Data (TS_UD_SC_SEC1)
static inline void parse_mcs_data_sc_security(Stream & stream)
{
    LOG(LOG_INFO, "SC_SECURITY\n");
}

// 2.2.1.4.4 Server Network Data (TS_UD_SC_NET)
static inline void parse_mcs_data_sc_net(Stream & stream)
{
    LOG(LOG_INFO, "SC_NET\n");
}


static inline void recv_mcs_connect_initial_pdu_with_gcc_conference_create_request(
                Transport * trans,
                ClientInfo * client_info,
                ChannelList & channel_list)
{
    Stream stream(8192);
    X224In(trans, stream);

    if (stream.in_uint16_be() != BER_TAG_MCS_CONNECT_INITIAL) {
        throw Error(ERR_MCS_BER_HEADER_UNEXPECTED_TAG);
    }
    int len = stream.in_ber_len();
    if (stream.in_uint8() != BER_TAG_OCTET_STRING) {
        throw Error(ERR_MCS_BER_HEADER_UNEXPECTED_TAG);
    }
    len = stream.in_ber_len();
    stream.skip_uint8(len);

    if (stream.in_uint8() != BER_TAG_OCTET_STRING) {
        throw Error(ERR_MCS_BER_HEADER_UNEXPECTED_TAG);
    }
    len = stream.in_ber_len();
    stream.skip_uint8(len);
    if (stream.in_uint8() != BER_TAG_BOOLEAN) {
        throw Error(ERR_MCS_BER_HEADER_UNEXPECTED_TAG);
    }
    len = stream.in_ber_len();
    stream.skip_uint8(len);

    if (stream.in_uint8() != BER_TAG_MCS_DOMAIN_PARAMS) {
        throw Error(ERR_MCS_BER_HEADER_UNEXPECTED_TAG);
    }
    len = stream.in_ber_len();
    stream.skip_uint8(len);

    if (stream.in_uint8() != BER_TAG_MCS_DOMAIN_PARAMS) {
        throw Error(ERR_MCS_BER_HEADER_UNEXPECTED_TAG);
    }
    len = stream.in_ber_len();
    stream.skip_uint8(len);

    if (stream.in_uint8() != BER_TAG_MCS_DOMAIN_PARAMS) {
        throw Error(ERR_MCS_BER_HEADER_UNEXPECTED_TAG);
    }
    len = stream.in_ber_len();
    stream.skip_uint8(len);

    if (stream.in_uint8() != BER_TAG_OCTET_STRING) {
        throw Error(ERR_MCS_BER_HEADER_UNEXPECTED_TAG);
    }
    len = stream.in_ber_len();

    stream.skip_uint8(23);

// 2.2.1.3.1 User Data Header (TS_UD_HEADER)
// =========================================

// type (2 bytes): A 16-bit, unsigned integer. The type of the data
//                 block that this header precedes.

// +-------------------+-------------------------------------------------------+
// | CS_CORE 0xC001 : The data block that follows contains Client Core
//                 Data (section 2.2.1.3.2).
// +-------------------+-------------------------------------------------------+
// | CS_SECURITY 0xC002 : The data block that follows contains Client
//                  Security Data (section 2.2.1.3.3).
// +-------------------+-------------------------------------------------------+
// | CS_NET 0xC003 : The data block that follows contains Client Network
//                 Data (section 2.2.1.3.4).
// +-------------------+-------------------------------------------------------+
// | CS_CLUSTER 0xC004 | The data block that follows contains Client Cluster   |
// |                   | Data (section 2.2.1.3.5).                             |
// +-------------------+-------------------------------------------------------+
// | CS_MONITOR 0xC005 | The data block that follows contains Client
//                 Monitor Data (section 2.2.1.3.6).
// +-------------------+-------------------------------------------------------+
// | SC_CORE 0x0C01 : The data block that follows contains Server Core
//                 Data (section 2.2.1.4.2)
// +-------------------+-------------------------------------------------------+
// | SC_SECURITY 0x0C02 : The data block that follows contains Server
//                 Security Data (section 2.2.1.4.3).
// +-------------------+-------------------------------------------------------+
// | SC_NET 0x0C03 : The data block that follows contains Server Network
//                 Data (section 2.2.1.4.4)
// +-------------------+-------------------------------------------------------+

// length (2 bytes): A 16-bit, unsigned integer. The size in bytes of the data
//   block, including this header.

    while (stream.check_rem(4)) {
        uint8_t * current_header = stream.p;
        uint16_t tag = stream.in_uint16_le();
        uint16_t length = stream.in_uint16_le();
        if (length < 4 || !stream.check_rem(length - 4)) {
            LOG(LOG_ERR,
                "error reading block tag %d size %d\n",
                tag, length);
            break;
        }

        switch (tag){
            case CS_CORE:
                #warning we should check length to call the two variants of core_data (or begin by reading the common part then the extended part)
                parse_mcs_data_cs_core(stream, client_info);
            break;
            case CS_SECURITY:
                parse_mcs_data_cs_security(stream);
            break;
            case CS_NET:
                LOG(LOG_INFO, "front:basic_settings:channel_list:cs_net %u", channel_list.size());
                parse_mcs_data_cs_net(stream, client_info, channel_list);
            break;
            case CS_CLUSTER:
                parse_mcs_data_cs_cluster(stream, client_info);
            break;
            case CS_MONITOR:
                parse_mcs_data_cs_monitor(stream);
            break;
            case SC_CORE:
                parse_mcs_data_sc_core(stream);
            break;
            case SC_SECURITY:
                parse_mcs_data_sc_security(stream);
            break;
            case SC_NET:
                parse_mcs_data_sc_net(stream);
            break;
            default:
                LOG(LOG_INFO, "Unknown data block tag\n");
            break;
        }
        stream.p = current_header + length;
    }
}

static inline void send_mcs_connect_initial_pdu_with_gcc_conference_create_request(
                Transport * trans,
                const ChannelList & channel_list,
                int width,
                int height,
                int rdp_bpp,
                int keylayout,
                bool console_session,
                char * hostname){

    Stream data(8192);

    int length = 158 + 76 + 12 + 4;

    #warning another option could be to emit channel list even if number of channel is zero. It looks more logical to me than not passing any channel information (what happens in this case ?)
    if (channel_list.size() > 0){
        length += channel_list.size() * 12 + 8;
    }

    /* Generic Conference Control (T.124) ConferenceCreateRequest */
    data.out_uint16_be(5);
    data.out_uint16_be(0x14);
    data.out_uint8(0x7c);
    data.out_uint16_be(1);

    data.out_uint16_be((length | 0x8000)); /* remaining length */

    data.out_uint16_be(8); /* length? */
    data.out_uint16_be(16);
    data.out_uint8(0);
    data.out_uint16_le(0xc001);
    data.out_uint8(0);

    data.out_uint32_le(0x61637544); /* OEM ID: "Duca", as in Ducati. */
    data.out_uint16_be(((length - 14) | 0x8000)); /* remaining length */

    /* Client information */
    data.out_uint16_le(CS_CORE);
    LOG(LOG_INFO, "Sending Client Core Data to remote server\n");
    data.out_uint16_le(212); /* length */
    LOG(LOG_INFO, "core::header::length = %u\n", 212);
    data.out_uint32_le(0x00080004); // RDP version. 1 == RDP4, 4 == RDP5.
    LOG(LOG_INFO, "core::header::version (0x00080004 = RDP 5.0, 5.1, 5.2, and 6.0 clients)");
    data.out_uint16_le(width);
    LOG(LOG_INFO, "core::desktopWidth = %u\n", width);
    data.out_uint16_le(height);
    LOG(LOG_INFO, "core::desktopHeight = %u\n", height);
    data.out_uint16_le(0xca01);
    LOG(LOG_INFO, "core::colorDepth = RNS_UD_COLOR_8BPP (superseded by postBeta2ColorDepth)");
    data.out_uint16_le(0xaa03);
    LOG(LOG_INFO, "core::SASSequence = RNS_UD_SAS_DEL");
    data.out_uint32_le(keylayout);
    LOG(LOG_INFO, "core::keyboardLayout = %x", keylayout);
    data.out_uint32_le(2600); /* Client build. We are now 2600 compatible :-) */
    LOG(LOG_INFO, "core::clientBuild = 2600");
    LOG(LOG_INFO, "core::clientName=%s\n", hostname);

    /* Added in order to limit hostlen and hostname size */
    int hostlen = 2 * strlen(hostname);
    if (hostlen > 30){
        hostlen = 30;
    }
    /* Unicode name of client, padded to 30 bytes */
    data.out_unistr(hostname);
    data.out_clear_bytes(30 - hostlen);

    /* See
    http://msdn.microsoft.com/library/default.asp?url=/library/en-us/wceddk40/html/cxtsksupportingremotedesktopprotocol.asp */
    #warning code should be updated to take care of keyboard type
    data.out_uint32_le(4); // g_keyboard_type
    LOG(LOG_INFO, "core::keyboardType = IBM enhanced (101- or 102-key) keyboard");
    data.out_uint32_le(0); // g_keyboard_subtype
    LOG(LOG_INFO, "core::keyboardSubType = 0");
    data.out_uint32_le(12); // g_keyboard_functionkeys
    LOG(LOG_INFO, "core::keyboardFunctionKey = 12 function keys");
    data.out_clear_bytes(64); /* imeFileName */
    LOG(LOG_INFO, "core::imeFileName = \"\"");
    data.out_uint16_le(0xca01); /* color depth 8bpp */
    LOG(LOG_INFO, "core::postBeta2ColorDepth = RNS_UD_COLOR_8BPP (superseded by highColorDepth)");
    data.out_uint16_le(1);
    LOG(LOG_INFO, "core::clientProductId = 1");
    data.out_uint32_le(0);
    LOG(LOG_INFO, "core::serialNumber = 0");
    data.out_uint16_le(rdp_bpp);
    LOG(LOG_INFO, "core::highColorDepth = %u", rdp_bpp);
    data.out_uint16_le(0x0007);
    LOG(LOG_INFO, "core::supportedColorDepths = 24/16/15");
    data.out_uint16_le(1);
    LOG(LOG_INFO, "core::earlyCapabilityFlags = RNS_UD_CS_SUPPORT_ERRINFO_PDU");
    data.out_clear_bytes(64);
    LOG(LOG_INFO, "core::clientDigProductId = \"\"");
    data.out_clear_bytes(2);
    LOG(LOG_INFO, "core::pad2octets");
//        data.out_uint32_le(0); // optional
//        LOG(LOG_INFO, "core::serverSelectedProtocol = 0");
    /* End of client info */

    data.out_uint16_le(CS_CLUSTER);
    data.out_uint16_le(12);
    #warning check that should depend on g_console_session
    data.out_uint32_le(console_session ? 0xb : 9);
    data.out_uint32_le(0);

    /* Client encryption settings */
    data.out_uint16_le(CS_SECURITY);
    data.out_uint16_le(12); /* length */

    #warning check that, should depend on g_encryption
    /* encryption supported, 128-bit supported */
    data.out_uint32_le(0x3);
    data.out_uint32_le(0); /* Unknown */

    send_cs_net(data, channel_list);

    data.mark_end();

    send_connection_initial(trans, data);
}


//   2.2.1.5 Client MCS Erect Domain Request PDU
//   -------------------------------------------
//   The MCS Erect Domain Request PDU is an RDP Connection Sequence PDU sent
//   from client to server during the Channel Connection phase (see section
//   1.3.1.1). It is sent after receiving the MCS Connect Response PDU (section
//   2.2.1.4).

//   tpktHeader (4 bytes): A TPKT Header, as specified in [T123] section 8.

//   x224Data (3 bytes): An X.224 Class 0 Data TPDU, as specified in [X224]
//      section 13.7.

// See description of tpktHeader and x224 Data TPDU in cheat sheet

//   mcsEDrq (5 bytes): PER-encoded MCS Domain PDU which encapsulates an MCS
//      Erect Domain Request structure, as specified in [T125] (the ASN.1
//      structure definitions are given in [T125] section 7, parts 3 and 10).


// 2.2.1.6 Client MCS Attach User Request PDU
// ------------------------------------------
// The MCS Attach User Request PDU is an RDP Connection Sequence PDU
// sent from client to server during the Channel Connection phase (see
// section 1.3.1.1) to request a user channel ID. It is sent after
// transmitting the MCS Erect Domain Request PDU (section 2.2.1.5).

// tpktHeader (4 bytes): A TPKT Header, as specified in [T123] section 8.

// x224Data (3 bytes): An X.224 Class 0 Data TPDU, as specified in
//   [X224] section 13.7.

// See description of tpktHeader and x224 Data TPDU in cheat sheet

// mcsAUrq (1 byte): PER-encoded MCS Domain PDU which encapsulates an
//  MCS Attach User Request structure, as specified in [T125] (the ASN.1
//  structure definitions are given in [T125] section 7, parts 5 and 10).

// AttachUserRequest ::= [APPLICATION 10] IMPLICIT SEQUENCE
// {
// }

// 11.17 AttachUserRequest
// -----------------------

// AttachUserRequest is generated by an MCS-ATTACH-USER request. It rises to the
// top MCS provider, which returns an AttachUserConfirm reply. If the domain
// limit on number of user ids allows, a new user id is generated.

// AttachUserRequest contains no information other than its MCSPDU type. The
// domain to which the user attaches is determined by the MCS connection
// conveying the MCSPDU. The only initial characteristic of the user id
// generated is its uniqueness. An MCS provider shall make a record of each
// unanswered AttachUserRequest received and by which MCS connection it arrived,
// so that a replying AttachUserConfirm can be routed back to the same source.
// To distribute replies fairly, each provider should maintain a first-in,
// first-out queue for this purpose.



static inline void send_mcs_erect_domain_and_attach_user_request_pdu(Transport * trans)
{
    #warning there should be a way to merge both packets in the same stream to only perform one unique send
    Stream edrq_stream(8192);
    X224Out edrq_tpdu(X224Packet::DT_TPDU, edrq_stream);
    edrq_stream.out_uint8((MCS_EDRQ << 2));
    edrq_stream.out_uint16_be(0x100); /* height */
    edrq_stream.out_uint16_be(0x100); /* interval */
    edrq_tpdu.end();
    edrq_tpdu.send(trans);

    Stream aurq_stream(8192);
    X224Out aurq_tpdu(X224Packet::DT_TPDU, aurq_stream);
    aurq_stream.out_uint8((MCS_AURQ << 2));
    aurq_tpdu.end();
    aurq_tpdu.send(trans);
}

static inline void recv_mcs_erect_domain_and_attach_user_request_pdu(Transport * trans, uint16_t & userid)
{
    #warning this code could lead to some problem if both MCS are combined in the same TPDU, we should manage this case
    {
        Stream stream(8192);
        X224In in(trans, stream);
        uint8_t opcode = stream.in_uint8();
        if ((opcode >> 2) != MCS_EDRQ) {
            throw Error(ERR_MCS_RECV_EDQR_APPID_NOT_EDRQ);
        }
        stream.skip_uint8(2);
        stream.skip_uint8(2);
        if (opcode & 2) {
            userid = stream.in_uint16_be();
        }
        in.end();
    }

    {
        Stream stream(8192);
        X224In in(trans, stream);
        uint8_t opcode = stream.in_uint8();
        if ((opcode >> 2) != MCS_AURQ) {
            throw Error(ERR_MCS_RECV_AURQ_APPID_NOT_AURQ);
        }
        if (opcode & 2) {
            userid = stream.in_uint16_be();
        }
        in.end();
    }

}

// 2.2.1.8 Client MCS Channel Join Request PDU
// -------------------------------------------
// The MCS Channel Join Request PDU is an RDP Connection Sequence PDU sent
// from client to server during the Channel Connection phase (see section
// 1.3.1.1). It is sent after receiving the MCS Attach User Confirm PDU
// (section 2.2.1.7). The client uses the MCS Channel Join Request PDU to
// join the user channel obtained from the Attach User Confirm PDU, the
// I/O channel and all of the static virtual channels obtained from the
// Server Network Data structure (section 2.2.1.4.4).

// tpktHeader (4 bytes): A TPKT Header, as specified in [T123] section 8.

// x224Data (3 bytes): An X.224 Class 0 Data TPDU, as specified in [X224]
//                     section 13.7.

// mcsCJrq (5 bytes): PER-encoded MCS Domain PDU which encapsulates an
//                    MCS Channel Join Request structure as specified in
//                    [T125] sections 10.19 and I.3 (the ASN.1 structure
//                    definitions are given in [T125] section 7, parts 6
//                    and 10).

// ChannelJoinRequest ::= [APPLICATION 14] IMPLICIT SEQUENCE
// {
//     initiator UserId
//     channelId ChannelId
//               -- may be zero
// }


// 11.21 ChannelJoinRequest
// ------------------------

// ChannelJoinRequest is generated by an MCS-CHANNEL-JOIN request. If valid, it
// rises until it reaches an MCS provider with enough information to generate a
// ChannelJoinConfirm reply. This may be the top MCS provider.

// Table 11-21/T.125 – ChannelJoinRequest MCSPDU
// +-----------------+-------------------------------+------------------------+
// | Contents        |           Source              |           Sink         |
// +-----------------+-------------------------------+------------------------+
// | Initiator       |      Requesting Provider      |       Higher provider  |
// +-----------------+-------------------------------+------------------------+
// | Channel Id      |      Request                  |       Higher provider  |
// +-----------------+-------------------------------+------------------------+


// The user id of the initiating MCS attachment is supplied by the MCS provider
// that receives the primitive request. Providers that receive
// ChannelJoinRequest subsequently shall validate the user id to ensure that it
// is legitimately assigned to the subtree of origin. If the user id is invalid,
// the MCSPDU shall be ignored.

// NOTE – This allows for the possibility that ChannelJoinRequest may be racing
// upward against a purge of the initiating user id flowing down. A provider
// that receives PurgeChannelsIndication first might receive a
// ChannelJoinRequest soon thereafter that contains an invalid user id. This is
// a normal occurrence and is not cause for rejecting the MCSPDU.

// ChannelJoinRequest may rise to an MCS provider that has the requested channel
// id in its information base. Any such provider, being consistent with the top
// MCS provider, will agree whether the request should succeed. If the request
// should fail, the provider shall generate an unsuccessful ChannelJoinConfirm.
// If it should succeed and the provider is already joined to the same channel,
// the provider shall generate a successful ChannelJoinConfirm. In these two
// cases, MCS-CHANNEL-JOIN completes without necessarily visiting the top MCS
// provider. Otherwise, if the request should succeed but the channel is not yet
// joined, a provider shall forward ChannelJoinRequest upward.

// If ChannelJoinRequest rises to the top MCS provider, the channel id
// requested may be zero, which is in no information base because it is an
// invalid id. If the domain limit on the number of channels in use allows,
// a new assigned channel id shall be generated and returned in a successful
// ChannelJoinConfirm. If the channel id requested is in the static range and
// the domain limit on the number of channels in use allows, the channel id
// shall be entered into the information base and shall likewise be returned
// in a successful ChannelJoinConfirm.

// Otherwise, the request will succeed only if the channel id is already in the
// information base of the top MCS provider. A user id channel can only be
// joined by the same user. A private channel id can be joined only by users
// previously admitted by its manager. An assigned channel id can be joined
// by any user.

static inline void send_mcs_channel_join_request_pdu(Transport * trans, int userid, int chanid)
{
    LOG(LOG_INFO, "send_mcs_channel_join_request_pdu userid=%u chanid=%u", userid, chanid);
    Stream cjrq_stream(8192);
    X224Out cjrq_tpdu(X224Packet::DT_TPDU, cjrq_stream);
    cjrq_stream.out_uint8((MCS_CJRQ << 2));
    cjrq_stream.out_uint16_be(userid);
    cjrq_stream.out_uint16_be(chanid);
    cjrq_tpdu.end();
    cjrq_tpdu.send(trans);
}

static inline void recv_mcs_channel_join_request_pdu(Transport * trans, uint16_t & userid, uint16_t & chanid){
    Stream stream(8192);
    // read tpktHeader (4 bytes = 3 0 len)
    // TPDU class 0    (3 bytes = LI F0 PDU_DT)
    X224In in(trans, stream);

    uint8_t opcode = stream.in_uint8();
    if ((opcode >> 2) != MCS_CJRQ) {
        LOG(LOG_INFO, "unexpected opcode = %u", opcode);
        throw Error(ERR_MCS_RECV_CJRQ_APPID_NOT_CJRQ);
    }
    userid = stream.in_uint16_be();
    chanid = stream.in_uint16_be();

    if (opcode & 2) {
        stream.skip_uint8(2);
    }

    LOG(LOG_INFO, "recv cjrq done");
    LOG(LOG_INFO, "recv_mcs_channel_join_request_pdu(userid=%u, chanid=%u)", userid, chanid);

    in.end();
}

// 2.2.1.9 Server MCS Channel Join Confirm PDU
// -------------------------------------------
// The MCS Channel Join Confirm PDU is an RDP Connection Sequence
// PDU sent from server to client during the Channel Connection
// phase (see section 1.3.1.1). It is sent as a response to the MCS
// Channel Join Request PDU (section 2.2.1.8).

// tpktHeader (4 bytes): A TPKT Header, as specified in [T123]
//   section 8.

// x224Data (3 bytes): An X.224 Class 0 Data TPDU, as specified in
//  [X224] section 13.7.

// mcsCJcf (8 bytes): PER-encoded MCS Domain PDU which encapsulates
//  an MCS Channel Join Confirm PDU structure, as specified in
//  [T125] (the ASN.1 structure definitions are given in [T125]
//  section 7, parts 6 and 10).

// ChannelJoinConfirm ::= [APPLICATION 15] IMPLICIT SEQUENCE
// {
//   result Result,
//   initiator UserId,
//   requested ChannelId, -- may be zero
//   channelId ChannelId OPTIONAL
// }

// 11.22 ChannelJoinConfirm
// ------------------------

// ChannelJoinConfirm is generated at a higher MCS provider upon receipt of
// ChannelJoinRequest. Routed back to the requesting provider, it generates an
// MCS-CHANNEL-JOIN confirm.

// Table 11-22/T.125 – ChannelJoinConfirm MCSPDU
// +-----------------------+------------------------+--------------------------+
// | Contents              |       Source           |         Sink             |
// +-----------------------+------------------------+--------------------------+
// | Result                |   Higher provider      |        Confirm           |
// +-----------------------+------------------------+--------------------------+
// | Initiator             |   Higher provider      |        MCSPDU routing    |
// +-----------------------+------------------------+--------------------------+
// | Requested             |   Higher provider      |        Confirm           |
// +-----------------------+------------------------+--------------------------+
// | Channel Id (optional) |   Higher provider      |        Confirm           |
// +-----------------------+------------------------+--------------------------+


// ChannelJoinConfirm contains a joined channel id if and only if the result is
// successful.


// The channel id requested is the same as in ChannelJoinRequest. This helps
// the initiating attachment relate MCS-CHANNEL-JOIN confirm to an antecedent
// request. Since ChannelJoinRequest need not rise to the top provider,
// confirms may occur out of order.

// If the result is successful, ChannelJoinConfirm joins the receiving MCS
// provider to the specified channel. Thereafter, higher providers shall route
// to it any data that users send over the channel. A provider shall remain
// joined to a channel as long as any of its attachments or subordinate
// providers does. To leave the channel, a provider shall generate
// ChannelLeaveRequest.

// Providers that receive a successful ChannelJoinConfirm shall enter the
// channel id into their information base. If not already there, the channel id
// shall be given type static or assigned, depending on its range.

// ChannelJoinConfirm shall be forwarded in the direction of the initiating user
// id. If the user id is unreachable because an MCS connection no longer exists,
// the provider shall decide whether it has reason to remain joined to the
// channel. If not, it shall generate ChannelLeaveRequest.

static inline void recv_mcs_channel_join_confirm_pdu(Transport * trans, uint16_t & mcs_userid, uint16_t & req_chanid, uint16_t & join_chanid)
{
    LOG(LOG_INFO, "recv_mcs_channel_join_confirm_pdu");
    Stream cjcf_stream(8192);
    X224In cjcf_tpdu(trans, cjcf_stream);
    int opcode = cjcf_stream.in_uint8();
    if ((opcode >> 2) != MCS_CJCF) {
        throw Error(ERR_MCS_RECV_CJCF_OPCODE_NOT_CJCF);
    }
    uint8_t result = cjcf_stream.in_uint8();
    if (0 != result) {
        LOG(LOG_INFO, "recv_mcs_channel_join_confirm_pdu exception, expected 0, got %u", result);
        throw Error(ERR_MCS_RECV_CJCF_EMPTY);
    }
    mcs_userid = cjcf_stream.in_uint16_be();
    req_chanid = cjcf_stream.in_uint16_be();
    if (opcode & 2) {
        join_chanid = cjcf_stream.in_uint16_be();
    }
    else {
        join_chanid = req_chanid;
    }
    LOG(LOG_INFO, "recv_mcs_channel_join_confirm_pdu opcode=%u userid=%u req_chanid=%u join_chanid=%u", opcode, mcs_userid, req_chanid, join_chanid);
    cjcf_tpdu.end();
}

static inline void send_mcs_channel_join_confirm_pdu(Transport * trans, uint16_t userid, uint16_t chanid)
{
    LOG(LOG_INFO, "send_mcs_channel_join_confirm_pdu(userid=%u, chanid=%u)", userid, chanid);
    Stream stream(8192);
    X224Out tpdu(X224Packet::DT_TPDU, stream);
    stream.out_uint8((MCS_CJCF << 2) | 2);
    stream.out_uint8(0);
    stream.out_uint16_be(userid);
    stream.out_uint16_be(chanid);
    #warning this should be sent only if different from requested chan_id
    stream.out_uint16_be(chanid);
    tpdu.end();
    tpdu.send(trans);
}

static inline void send_mcs_channel_join_request_and_recv_confirm_pdu(Transport * trans,
                    uint16_t userid, ChannelList & channel_list)
{
    #warning the array size below is arbitrary, it should be checked to avoid buffer overflow

    LOG(LOG_INFO, "send_cjrq_recv_cf userid=%u", userid);

    size_t num_channels = channel_list.size();
    uint16_t channels_id[100];
    channels_id[0] = userid + MCS_USERCHANNEL_BASE;
    channels_id[1] = MCS_GLOBAL_CHANNEL;
    for (size_t index = 0; index < num_channels; index++){
        channels_id[index+2] = channel_list[index].chanid;
    }

    LOG(LOG_INFO, "num_channels=%u", num_channels);
    for (size_t index = 0; index < num_channels+2; index++){
        send_mcs_channel_join_request_pdu(trans, userid, channels_id[index]);
        {
            uint16_t tmp_userid;
            uint16_t tmp_req_chanid;
            uint16_t tmp_join_chanid;
            recv_mcs_channel_join_confirm_pdu(trans, tmp_userid, tmp_req_chanid, tmp_join_chanid);
        }
        LOG(LOG_INFO, "----------------------- %u ---------------------------", index);
    }
}

// 2.2.1.7 Server MCS Attach User Confirm PDU
// ------------------------------------------
// The MCS Attach User Confirm PDU is an RDP Connection Sequence
// PDU sent from server to client during the Channel Connection
// phase (see section 1.3.1.1). It is sent as a response to the MCS
// Attach User Request PDU (section 2.2.1.6).

// tpktHeader (4 bytes): A TPKT Header, as specified in [T123]
//   section 8.

// x224Data (3 bytes): An X.224 Class 0 Data TPDU, as specified in
//   section [X224] 13.7.

// mcsAUcf (4 bytes): PER-encoded MCS Domain PDU which encapsulates
//   an MCS Attach User Confirm structure, as specified in [T125]
//   (the ASN.1 structure definitions are given in [T125] section 7,
// parts 5 and 10).

// AttachUserConfirm ::= [APPLICATION 11] IMPLICIT SEQUENCE
// {
//     result       Result,
//     initiator    UserId OPTIONAL
// }

// 11.18 AttachUserConfirm
// -----------------------

// AttachUserConfirm is generated at the top MCS provider upon receipt of
// AttachUserRequest. Routed back to the requesting provider, it generates an
//  MCS-ATTACH-USER confirm.

//      Table 11-18/T.125 – AttachUserConfirm MCSPDU
// +----------------------+-----------------+------------+
// |     Contents         |      Source     |    Sink    |
// +----------------------+-----------------+------------+
// | Result               | Top provider    |  Confirm   |
// +----------------------+-----------------+------------+
// | Initiator (optional) | Top provider    |  Confirm   |
// +----------------------+-----------------+------------+

// AttachUserConfirm contains a user id if and only if the result is successful.
// Providers that receive a successful AttachUserConfirm shall enter the user id
// into their information base. MCS providers shall route AttachUserConfirm to
// the source of an antecedent AttachUserRequest, using the knowledge that
// there is a one-to-one reply. A provider that transmits AttachUserConfirm
// shall note to which downward MCS connection the new user id is thereby
// assigned, so that it may validate the user id when it arises later in other
// requests.

static inline void recv_mcs_attach_user_confirm_pdu(Transport * trans, int & userid)
{
    Stream aucf_stream(8192);
    X224In aucf_tpdu(trans, aucf_stream);
    int opcode = aucf_stream.in_uint8();
    if ((opcode >> 2) != MCS_AUCF) {
        throw Error(ERR_MCS_RECV_AUCF_OPCODE_NOT_OK);
    }
    int res = aucf_stream.in_uint8();
    if (res != 0) {
        throw Error(ERR_MCS_RECV_AUCF_RES_NOT_0);
    }
    LOG(LOG_INFO, "aucf opcode=%u, userid=%u", opcode, userid);
    if (opcode & 2) {
        userid = aucf_stream.in_uint16_be();
    }
    LOG(LOG_INFO, "aucf2 opcode=%u, userid=%u", opcode, userid);
    aucf_tpdu.end();
}

static inline void send_mcs_attach_user_confirm_pdu(Transport * trans, uint16_t userid)
{
    Stream stream(8192);
    X224Out tpdu(X224Packet::DT_TPDU, stream);
    stream.out_uint8(((MCS_AUCF << 2) | 2));
    stream.out_uint8(0);
    stream.out_uint16_be(userid);
    tpdu.end();
    tpdu.send(trans);
}


#endif