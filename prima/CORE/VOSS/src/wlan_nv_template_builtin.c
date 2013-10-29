/*===========================================================================
                       EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $Header:$ $DateTime: $ $Author: $

  when        who        what, where, why
  --------    ---        -----------------------------------------------
  04/10/13    kumarpra   nv built in  creation
===========================================================================*/


/*
  -----------------------------------------
  This file is generated by nvTmplRun Parser
  -----------------------------------------
  To be auto-generated, or
  This is the source file and the header file can be generated from this
  template file.

  Template constructs
     1. TABLE_: struct
     2. INDEX_ENUM: enums, e.g. {RATE_OFDM_6M,RATE_OFDM_54M}
     3. INDEX_INT: int, e.g.{min, max, increment}
     3. Basic data types: tANI_U8, tANI_S8, tANI_U32, tANI_S32
     4. Storage types:
        4.1 SINGULAR: one element of basic data type
        4.2 ARRAY_1: one dimensional array, x-axis
        4.3 ARRAY_2: two dimensional array, (x, y)
        4.4 ARRAY_3: three dimensional array, (x, y, z)
        4.5 ARRAY_4: four dimensional array, (x, y, z, t)

  Implementation notes
     1. Flow of changing NV data format: (TBD) Either change the template and
        generate the header file, or modify header file and auto-generate
        the template.
     2. Flow of writing NV data: encode the template in the data stream, so the
        NV data is "self-sufficient". No separate template, no compability
        issue, no need of version control.
     3. Flow of reading NV data: parse the binary NV data stream based on the
        template info in the data stream.
     4. The above NV logic is decoupled from the actual data content, a generic,
        content ergonostic parser (reading) and encoder (writing).
        The NV logic is common code shared by tools, s/w
        (both host and firmware), and off-line utilities.
     5. NV data parsing and "acceptanace" into an s/w moduel data structure can
        be "configured" in several ways:
        5.1 only total matching of all fields, otherwise, reject the whole data
        stream (a table).
        5.2 partial matching of fields allowed and the rest fields assume
        reasonal default values,
        The choice can be determined later, but the capability is provided.
     6. We could also design in this selection on an individual table base.
        To design such capability, reserve some header bits in the data stream.
     7. The NV data streams can be modified, replaced, or intact with a new data
        stream of the same table ID added to NV data.
        The choice can be determined later, but the NV scheme provides such
        capability.
     8. The template construct definitions can be common to all tables
        (tbd: in a common section) or table specific, or updated in a
        subsequent format section.
        The use cases are:
        - An index enum (e.g. RF channels) is common to all tables when the NV
          data is created. Later new enums are added (e.g.
        additional channels), one can choose to add the new index enum for new
        tables appended to the NV data, or replace the
        old table with new template info and data.
        The template precedence is table specific then common, and later
        "common" overwrites "earlier" commmon.
        - A new field is added to the table, the user decides to replace the
          old table data, he can simply encode the template info in the
          data stream.
        - In the same scenario (a new field is added), the user decides to
          append a new table, he can encode the template
        in the new data table and append it to NV data, or write a new common
        template section and append the data.

  Key "ingredients", (re-iterate the most important features and capabilities)
     1. How to parse the data is embedded in the NV data itself. It removes the
        dependency on header file matching,
        version checking, compatibility among tools, host and firmware.
     2. Table field ID enables "partial" data acceptance in an s/w module data
        structure. Whether full matching or reject the whole table, or "partial"
        acceptance, the capabiilty is in place and further ensures the robust
        NV data extensibility and compatibility.
     3. The table granularity, data stream based NV data has variable length
        and flexibility of modifying an existing table data, replacing the
        whole data, or leaving the existing data table intact and appending
        a new table.
  Misc notes:
     1. For endianness, support only 4 bytes integer or 4 1-byte
     2. String identifier needs to be shortened to save storage
     3. string_field_name,  field type,  field storage class,  storage size
*/


#include "wlan_nv_types.h"
#include "wlan_nv_stream.h"
#include "wlan_nv.h"
#include "wlan_nv_template_internal.h"
#include "wlan_nv_template_api.h"
#include "wlan_nv_template_builtin.h"



static int enumNoOfFieldArray[INDEX_ENUM_BUILTIN_MAX] =
     {7,1,9,5,80,14,3,55};

_NV_TEMPLATE_ENUM NvEnumsBuiltIn[/*INDEX_ENUM_MAX*/][ENUM_ENTRIES_MAX] = {
   { // INDEX_ENUM_ALL
      {"j1",INDEX_ENUM_NUM_PHY_MAX_TX_CHAINS,0,{nul}},
      {"j2",INDEX_ENUM_NUM_REG_DOMAINS,0,{nul}},
      {"j3",INDEX_ENUM_NUM_RF_SUBBANDS,0,{nul}},
      {"j4",INDEX_ENUM_NUM_RF_CHANNELS,0,{nul}},
      {"j5",INDEX_ENUM_NUM_2_4GHZ_CHANNELS,0,{nul}},
      {"j6",INDEX_ENUM_NUM_802_11_MODES,0,{nul}},
      {"j7",INDEX_ENUM_NUM_HAL_PHY_RATES,0,{nul}},
      {{nul},0, 0,{nul}},
   }, // INDEX_ENUM_ALL

   { // INDEX_ENUM_NUM_PHY_MAX_TX_CHAINS
      {"a1",PHY_TX_CHAIN_0,0,{nul}},
      {{nul},0, 0,{nul}},
   }, // INDEX_ENUM_NUM_PHY_MAX_TX_CHAINS

   { // INDEX_ENUM_NUM_REG_DOMAINS
      {"a3",REG_DOMAIN_FCC,0,{nul}},
      {"a4",REG_DOMAIN_ETSI,0,{nul}},
      {"a5",REG_DOMAIN_JAPAN,0,{nul}},
      {"a6",REG_DOMAIN_WORLD,0,{nul}},
      {"a7",REG_DOMAIN_N_AMER_EXC_FCC,0,{nul}},
      {"a8",REG_DOMAIN_APAC,0,{nul}},
      {"a9",REG_DOMAIN_KOREA,0,{nul}},
      {"ba",REG_DOMAIN_HI_5GHZ,0,{nul}},
      {"bb",REG_DOMAIN_NO_5GHZ,0,{nul}},
      {{nul},0, 0,{nul}},
   }, // INDEX_ENUM_NUM_REG_DOMAINS

   { // INDEX_ENUM_NUM_RF_SUBBANDS
      {"bd",RF_SUBBAND_2_4_GHZ,0,{nul}},
      {"be",RF_SUBBAND_5_LOW_GHZ,0,{nul}},
      {"bf",RF_SUBBAND_5_MID_GHZ,0,{nul}},
      {"bg",RF_SUBBAND_5_HIGH_GHZ,0,{nul}},
      {"bh",RF_SUBBAND_4_9_GHZ,0,{nul}},
      {{nul},0, 0,{nul}},
   }, // INDEX_ENUM_NUM_RF_SUBBANDS

   { // INDEX_ENUM_NUM_RF_CHANNELS
      {"bj",RF_CHAN_1,0,{nul}},
      {"bk",RF_CHAN_2,0,{nul}},
      {"bl",RF_CHAN_3,0,{nul}},
      {"bm",RF_CHAN_4,0,{nul}},
      {"bn",RF_CHAN_5,0,{nul}},
      {"bo",RF_CHAN_6,0,{nul}},
      {"bp",RF_CHAN_7,0,{nul}},
      {"bq",RF_CHAN_8,0,{nul}},
      {"br",RF_CHAN_9,0,{nul}},
      {"bs",RF_CHAN_10,0,{nul}},
      {"bt",RF_CHAN_11,0,{nul}},
      {"bu",RF_CHAN_12,0,{nul}},
      {"bv",RF_CHAN_13,0,{nul}},
      {"bw",RF_CHAN_14,0,{nul}},
      {"bx",RF_CHAN_240,0,{nul}},
      {"by",RF_CHAN_244,0,{nul}},
      {"bz",RF_CHAN_248,0,{nul}},
      {"b0",RF_CHAN_252,0,{nul}},
      {"b1",RF_CHAN_208,0,{nul}},
      {"b2",RF_CHAN_212,0,{nul}},
      {"b3",RF_CHAN_216,0,{nul}},
      {"b4",RF_CHAN_36,0,{nul}},
      {"b5",RF_CHAN_40,0,{nul}},
      {"b6",RF_CHAN_44,0,{nul}},
      {"b7",RF_CHAN_48,0,{nul}},
      {"b8",RF_CHAN_52,0,{nul}},
      {"b9",RF_CHAN_56,0,{nul}},
      {"ca",RF_CHAN_60,0,{nul}},
      {"cb",RF_CHAN_64,0,{nul}},
      {"cc",RF_CHAN_100,0,{nul}},
      {"cd",RF_CHAN_104,0,{nul}},
      {"ce",RF_CHAN_108,0,{nul}},
      {"cf",RF_CHAN_112,0,{nul}},
      {"cg",RF_CHAN_116,0,{nul}},
      {"ch",RF_CHAN_120,0,{nul}},
      {"ci",RF_CHAN_124,0,{nul}},
      {"cj",RF_CHAN_128,0,{nul}},
      {"ck",RF_CHAN_132,0,{nul}},
      {"cl",RF_CHAN_136,0,{nul}},
      {"cm",RF_CHAN_140,0,{nul}},
      {"cn",RF_CHAN_149,0,{nul}},
      {"co",RF_CHAN_153,0,{nul}},
      {"cp",RF_CHAN_157,0,{nul}},
      {"cq",RF_CHAN_161,0,{nul}},
      {"cr",RF_CHAN_165,0,{nul}},
      {"cs",RF_CHAN_BOND_3,0,{nul}},
      {"ct",RF_CHAN_BOND_4,0,{nul}},
      {"cu",RF_CHAN_BOND_5,0,{nul}},
      {"cv",RF_CHAN_BOND_6,0,{nul}},
      {"cw",RF_CHAN_BOND_7,0,{nul}},
      {"cx",RF_CHAN_BOND_8,0,{nul}},
      {"cy",RF_CHAN_BOND_9,0,{nul}},
      {"cz",RF_CHAN_BOND_10,0,{nul}},
      {"c0",RF_CHAN_BOND_11,0,{nul}},
      {"c1",RF_CHAN_BOND_242,0,{nul}},
      {"c2",RF_CHAN_BOND_246,0,{nul}},
      {"c3",RF_CHAN_BOND_250,0,{nul}},
      {"c4",RF_CHAN_BOND_210,0,{nul}},
      {"c5",RF_CHAN_BOND_214,0,{nul}},
      {"c6",RF_CHAN_BOND_38,0,{nul}},
      {"c7",RF_CHAN_BOND_42,0,{nul}},
      {"c8",RF_CHAN_BOND_46,0,{nul}},
      {"c9",RF_CHAN_BOND_50,0,{nul}},
      {"da",RF_CHAN_BOND_54,0,{nul}},
      {"db",RF_CHAN_BOND_58,0,{nul}},
      {"dc",RF_CHAN_BOND_62,0,{nul}},
      {"dd",RF_CHAN_BOND_102,0,{nul}},
      {"de",RF_CHAN_BOND_106,0,{nul}},
      {"df",RF_CHAN_BOND_110,0,{nul}},
      {"dg",RF_CHAN_BOND_114,0,{nul}},
      {"dh",RF_CHAN_BOND_118,0,{nul}},
      {"di",RF_CHAN_BOND_122,0,{nul}},
      {"dj",RF_CHAN_BOND_126,0,{nul}},
      {"dk",RF_CHAN_BOND_130,0,{nul}},
      {"dl",RF_CHAN_BOND_134,0,{nul}},
      {"dm",RF_CHAN_BOND_138,0,{nul}},
      {"dn",RF_CHAN_BOND_151,0,{nul}},
      {"do",RF_CHAN_BOND_155,0,{nul}},
      {"dp",RF_CHAN_BOND_159,0,{nul}},
      {"dq",RF_CHAN_BOND_163,0,{nul}},
      {{nul},0, 0,{nul}},
   }, // INDEX_ENUM_NUM_RF_CHANNELS

   { // INDEX_ENUM_NUM_2_4GHZ_CHANNELS
      {"ke",RF_CHAN_1_1,0,{nul}},
      {"kf",RF_CHAN_2_1,0,{nul}},
      {"kg",RF_CHAN_3_1,0,{nul}},
      {"kh",RF_CHAN_4_1,0,{nul}},
      {"ki",RF_CHAN_5_1,0,{nul}},
      {"kj",RF_CHAN_6_1,0,{nul}},
      {"kk",RF_CHAN_7_1,0,{nul}},
      {"kl",RF_CHAN_8_1,0,{nul}},
      {"km",RF_CHAN_9_1,0,{nul}},
      {"kn",RF_CHAN_10_1,0,{nul}},
      {"ko",RF_CHAN_11_1,0,{nul}},
      {"kp",RF_CHAN_12_1,0,{nul}},
      {"kq",RF_CHAN_13_1,0,{nul}},
      {"kr",RF_CHAN_14_1,0,{nul}},
      {{nul},0, 0,{nul}},
   }, // INDEX_ENUM_NUM_2_4GHZ_CHANNELS

   { // INDEX_ENUM_NUM_802_11_MODES
      {"d6",MODE_802_11B,0,{nul}},
      {"d7",MODE_802_11AG,0,{nul}},
      {"d8",MODE_802_11N,0,{nul}},
      {{nul},0, 0,{nul}},
   }, // INDEX_ENUM_NUM_802_11_MODES

   { // INDEX_ENUM_NUM_HAL_PHY_RATES
      {"ez",HAL_PHY_RATE_11B_LONG_1_MBPS,0,{nul}},
      {"e0",HAL_PHY_RATE_11B_LONG_2_MBPS,0,{nul}},
      {"e1",HAL_PHY_RATE_11B_LONG_5_5_MBPS,0,{nul}},
      {"e2",HAL_PHY_RATE_11B_LONG_11_MBPS,0,{nul}},
      {"e3",HAL_PHY_RATE_11B_SHORT_2_MBPS,0,{nul}},
      {"e4",HAL_PHY_RATE_11B_SHORT_5_5_MBPS,0,{nul}},
      {"e5",HAL_PHY_RATE_11B_SHORT_11_MBPS,0,{nul}},
      {"e6",HAL_PHY_RATE_11A_6_MBPS,0,{nul}},
      {"e7",HAL_PHY_RATE_11A_9_MBPS,0,{nul}},
      {"e8",HAL_PHY_RATE_11A_12_MBPS,0,{nul}},
      {"e9",HAL_PHY_RATE_11A_18_MBPS,0,{nul}},
      {"fa",HAL_PHY_RATE_11A_24_MBPS,0,{nul}},
      {"fb",HAL_PHY_RATE_11A_36_MBPS,0,{nul}},
      {"fc",HAL_PHY_RATE_11A_48_MBPS,0,{nul}},
      {"fd",HAL_PHY_RATE_11A_54_MBPS,0,{nul}},
      {"fe",HAL_PHY_RATE_11A_DUP_6_MBPS,0,{nul}},
      {"ff",HAL_PHY_RATE_11A_DUP_9_MBPS,0,{nul}},
      {"fg",HAL_PHY_RATE_11A_DUP_12_MBPS,0,{nul}},
      {"fh",HAL_PHY_RATE_11A_DUP_18_MBPS,0,{nul}},
      {"fi",HAL_PHY_RATE_11A_DUP_24_MBPS,0,{nul}},
      {"fj",HAL_PHY_RATE_11A_DUP_36_MBPS,0,{nul}},
      {"fk",HAL_PHY_RATE_11A_DUP_48_MBPS,0,{nul}},
      {"fl",HAL_PHY_RATE_11A_DUP_54_MBPS,0,{nul}},
      {"fm",HAL_PHY_RATE_MCS_1NSS_6_5_MBPS,0,{nul}},
      {"fn",HAL_PHY_RATE_MCS_1NSS_13_MBPS,0,{nul}},
      {"fo",HAL_PHY_RATE_MCS_1NSS_19_5_MBPS,0,{nul}},
      {"fp",HAL_PHY_RATE_MCS_1NSS_26_MBPS,0,{nul}},
      {"fq",HAL_PHY_RATE_MCS_1NSS_39_MBPS,0,{nul}},
      {"fr",HAL_PHY_RATE_MCS_1NSS_52_MBPS,0,{nul}},
      {"fs",HAL_PHY_RATE_MCS_1NSS_58_5_MBPS,0,{nul}},
      {"ft",HAL_PHY_RATE_MCS_1NSS_65_MBPS,0,{nul}},
      {"fu",HAL_PHY_RATE_MCS_1NSS_MM_SG_7_2_MBPS,0,{nul}},
      {"fv",HAL_PHY_RATE_MCS_1NSS_MM_SG_14_4_MBPS,0,{nul}},
      {"fw",HAL_PHY_RATE_MCS_1NSS_MM_SG_21_7_MBPS,0,{nul}},
      {"fx",HAL_PHY_RATE_MCS_1NSS_MM_SG_28_9_MBPS,0,{nul}},
      {"fy",HAL_PHY_RATE_MCS_1NSS_MM_SG_43_3_MBPS,0,{nul}},
      {"fz",HAL_PHY_RATE_MCS_1NSS_MM_SG_57_8_MBPS,0,{nul}},
      {"f0",HAL_PHY_RATE_MCS_1NSS_MM_SG_65_MBPS,0,{nul}},
      {"f1",HAL_PHY_RATE_MCS_1NSS_MM_SG_72_2_MBPS,0,{nul}},
      {"f2",HAL_PHY_RATE_MCS_1NSS_CB_13_5_MBPS,0,{nul}},
      {"f3",HAL_PHY_RATE_MCS_1NSS_CB_27_MBPS,0,{nul}},
      {"f4",HAL_PHY_RATE_MCS_1NSS_CB_40_5_MBPS,0,{nul}},
      {"f5",HAL_PHY_RATE_MCS_1NSS_CB_54_MBPS,0,{nul}},
      {"f6",HAL_PHY_RATE_MCS_1NSS_CB_81_MBPS,0,{nul}},
      {"f7",HAL_PHY_RATE_MCS_1NSS_CB_108_MBPS,0,{nul}},
      {"f8",HAL_PHY_RATE_MCS_1NSS_CB_121_5_MBPS,0,{nul}},
      {"f9",HAL_PHY_RATE_MCS_1NSS_CB_135_MBPS,0,{nul}},
      {"ga",HAL_PHY_RATE_MCS_1NSS_MM_SG_CB_15_MBPS,0,{nul}},
      {"gb",HAL_PHY_RATE_MCS_1NSS_MM_SG_CB_30_MBPS,0,{nul}},
      {"gc",HAL_PHY_RATE_MCS_1NSS_MM_SG_CB_45_MBPS,0,{nul}},
      {"gd",HAL_PHY_RATE_MCS_1NSS_MM_SG_CB_60_MBPS,0,{nul}},
      {"ge",HAL_PHY_RATE_MCS_1NSS_MM_SG_CB_90_MBPS,0,{nul}},
      {"gf",HAL_PHY_RATE_MCS_1NSS_MM_SG_CB_120_MBPS,0,{nul}},
      {"gg",HAL_PHY_RATE_MCS_1NSS_MM_SG_CB_135_MBPS,0,{nul}},
      {"gh",HAL_PHY_RATE_MCS_1NSS_MM_SG_CB_150_MBPS,0,{nul}},
#ifdef WLAN_FEATURE_11AC
      {"gj",HAL_PHY_RATE_11AC_DUP_6_MBPS,0,{nul}},
      {"gk",HAL_PHY_RATE_11AC_DUP_9_MBPS,0,{nul}},
      {"gl",HAL_PHY_RATE_11AC_DUP_12_MBPS,0,{nul}},
      {"gm",HAL_PHY_RATE_11AC_DUP_18_MBPS,0,{nul}},
      {"gn",HAL_PHY_RATE_11AC_DUP_24_MBPS,0,{nul}},
      {"go",HAL_PHY_RATE_11AC_DUP_36_MBPS,0,{nul}},
      {"gp",HAL_PHY_RATE_11AC_DUP_48_MBPS,0,{nul}},
      {"gq",HAL_PHY_RATE_11AC_DUP_54_MBPS,0,{nul}},
      {"gr",HAL_PHY_RATE_VHT_20MHZ_MCS_1NSS_NGI_6_5_MBPS,0,{nul}},
      {"gs",HAL_PHY_RATE_VHT_20MHZ_MCS_1NSS_NGI_13_MBPS,0,{nul}},
      {"gt",HAL_PHY_RATE_VHT_20MHZ_MCS_1NSS_NGI_19_5_MBPS,0,{nul}},
      {"gu",HAL_PHY_RATE_VHT_20MHZ_MCS_1NSS_NGI_26_MBPS,0,{nul}},
      {"gv",HAL_PHY_RATE_VHT_20MHZ_MCS_1NSS_NGI_39_MBPS,0,{nul}},
      {"gw",HAL_PHY_RATE_VHT_20MHZ_MCS_1NSS_NGI_52_MBPS,0,{nul}},
      {"gx",HAL_PHY_RATE_VHT_20MHZ_MCS_1NSS_NGI_58_5_MBPS,0,{nul}},
      {"gy",HAL_PHY_RATE_VHT_20MHZ_MCS_1NSS_NGI_65_MBPS,0,{nul}},
      {"gz",HAL_PHY_RATE_VHT_20MHZ_MCS_1NSS_NGI_78_MBPS,0,{nul}},
#ifdef WCN_PRONTO
      {"j8",HAL_PHY_RATE_VHT_20MHZ_MCS_1NSS_NGI_86_5_MBPS,0,{nul}},
#endif
      {"g0",HAL_PHY_RATE_VHT_20MHZ_MCS_1NSS_SGI_7_2_MBPS,0,{nul}},
      {"g1",HAL_PHY_RATE_VHT_20MHZ_MCS_1NSS_SGI_14_4_MBPS,0,{nul}},
      {"g2",HAL_PHY_RATE_VHT_20MHZ_MCS_1NSS_SGI_21_6_MBPS,0,{nul}},
      {"g3",HAL_PHY_RATE_VHT_20MHZ_MCS_1NSS_SGI_28_8_MBPS,0,{nul}},
      {"g4",HAL_PHY_RATE_VHT_20MHZ_MCS_1NSS_SGI_43_3_MBPS,0,{nul}},
      {"g5",HAL_PHY_RATE_VHT_20MHZ_MCS_1NSS_SGI_57_7_MBPS,0,{nul}},
      {"g6",HAL_PHY_RATE_VHT_20MHZ_MCS_1NSS_SGI_65_MBPS,0,{nul}},
      {"g7",HAL_PHY_RATE_VHT_20MHZ_MCS_1NSS_SGI_72_2_MBPS,0,{nul}},
      {"g8",HAL_PHY_RATE_VHT_20MHZ_MCS_1NSS_SGI_86_6_MBPS,0,{nul}},
#ifdef WCN_PRONTO
      {"j9",HAL_PHY_RATE_VHT_20MHZ_MCS_1NSS_SGI_96_1_MBPS,0,{nul}},
#endif
      {"g9",HAL_PHY_RATE_VHT_40MHZ_MCS_1NSS_CB_NGI_13_5_MBPS,0,{nul}},
      {"ha",HAL_PHY_RATE_VHT_40MHZ_MCS_1NSS_CB_NGI_27_MBPS,0,{nul}},
      {"hb",HAL_PHY_RATE_VHT_40MHZ_MCS_1NSS_CB_NGI_40_5_MBPS,0,{nul}},
      {"hc",HAL_PHY_RATE_VHT_40MHZ_MCS_1NSS_CB_NGI_54_MBPS,0,{nul}},
      {"hd",HAL_PHY_RATE_VHT_40MHZ_MCS_1NSS_CB_NGI_81_MBPS,0,{nul}},
      {"he",HAL_PHY_RATE_VHT_40MHZ_MCS_1NSS_CB_NGI_108_MBPS,0,{nul}},
      {"hf",HAL_PHY_RATE_VHT_40MHZ_MCS_1NSS_CB_NGI_121_5_MBPS,0,{nul}},
      {"hg",HAL_PHY_RATE_VHT_40MHZ_MCS_1NSS_CB_NGI_135_MBPS,0,{nul}},
      {"hh",HAL_PHY_RATE_VHT_40MHZ_MCS_1NSS_CB_NGI_162_MBPS,0,{nul}},
      {"hi",HAL_PHY_RATE_VHT_40MHZ_MCS_1NSS_CB_NGI_180_MBPS,0,{nul}},
      {"hj",HAL_PHY_RATE_VHT_40MHZ_MCS_1NSS_CB_SGI_15_MBPS,0,{nul}},
      {"hk",HAL_PHY_RATE_VHT_40MHZ_MCS_1NSS_CB_SGI_30_MBPS,0,{nul}},
      {"hl",HAL_PHY_RATE_VHT_40MHZ_MCS_1NSS_CB_SGI_45_MBPS,0,{nul}},
      {"hm",HAL_PHY_RATE_VHT_40MHZ_MCS_1NSS_CB_SGI_60_MBPS,0,{nul}},
      {"hn",HAL_PHY_RATE_VHT_40MHZ_MCS_1NSS_CB_SGI_90_MBPS,0,{nul}},
      {"ho",HAL_PHY_RATE_VHT_40MHZ_MCS_1NSS_CB_SGI_120_MBPS,0,{nul}},
      {"hp",HAL_PHY_RATE_VHT_40MHZ_MCS_1NSS_CB_SGI_135_MBPS,0,{nul}},
      {"hq",HAL_PHY_RATE_VHT_40MHZ_MCS_1NSS_CB_SGI_150_MBPS,0,{nul}},
      {"hr",HAL_PHY_RATE_VHT_40MHZ_MCS_1NSS_CB_SGI_180_MBPS,0,{nul}},
      {"hs",HAL_PHY_RATE_VHT_40MHZ_MCS_1NSS_CB_SGI_200_MBPS,0,{nul}},
      {"ht",HAL_PHY_RATE_VHT_80MHZ_MCS_1NSS_CB_NGI_29_3_MBPS,0,{nul}},
      {"hu",HAL_PHY_RATE_VHT_80MHZ_MCS_1NSS_CB_NGI_58_5_MBPS,0,{nul}},
      {"hv",HAL_PHY_RATE_VHT_80MHZ_MCS_1NSS_CB_NGI_87_8_MBPS,0,{nul}},
      {"hw",HAL_PHY_RATE_VHT_80MHZ_MCS_1NSS_CB_NGI_117_MBPS,0,{nul}},
      {"hx",HAL_PHY_RATE_VHT_80MHZ_MCS_1NSS_CB_NGI_175_5_MBPS,0,{nul}},
      {"hy",HAL_PHY_RATE_VHT_80MHZ_MCS_1NSS_CB_NGI_234_MBPS,0,{nul}},
      {"hz",HAL_PHY_RATE_VHT_80MHZ_MCS_1NSS_CB_NGI_263_3_MBPS,0,{nul}},
      {"h0",HAL_PHY_RATE_VHT_80MHZ_MCS_1NSS_CB_NGI_292_5_MBPS,0,{nul}},
      {"h1",HAL_PHY_RATE_VHT_80MHZ_MCS_1NSS_CB_NGI_351_MBPS,0,{nul}},
      {"h2",HAL_PHY_RATE_VHT_80MHZ_MCS_1NSS_CB_NGI_390_MBPS,0,{nul}},
      {"h3",HAL_PHY_RATE_VHT_80MHZ_MCS_1NSS_CB_SGI_32_5_MBPS,0,{nul}},
      {"h4",HAL_PHY_RATE_VHT_80MHZ_MCS_1NSS_CB_SGI_65_MBPS,0,{nul}},
      {"h5",HAL_PHY_RATE_VHT_80MHZ_MCS_1NSS_CB_SGI_97_5_MBPS,0,{nul}},
      {"h6",HAL_PHY_RATE_VHT_80MHZ_MCS_1NSS_CB_SGI_130_MBPS,0,{nul}},
      {"h7",HAL_PHY_RATE_VHT_80MHZ_MCS_1NSS_CB_SGI_195_MBPS,0,{nul}},
      {"h8",HAL_PHY_RATE_VHT_80MHZ_MCS_1NSS_CB_SGI_260_MBPS,0,{nul}},
      {"h9",HAL_PHY_RATE_VHT_80MHZ_MCS_1NSS_CB_SGI_292_5_MBPS,0,{nul}},
      {"ia",HAL_PHY_RATE_VHT_80MHZ_MCS_1NSS_CB_SGI_325_MBPS,0,{nul}},
      {"ib",HAL_PHY_RATE_VHT_80MHZ_MCS_1NSS_CB_SGI_390_MBPS,0,{nul}},
      {"ic",HAL_PHY_RATE_VHT_80MHZ_MCS_1NSS_CB_SGI_433_3_MBPS,0,{nul}},
#endif
      {{nul},0, 0,{nul}},
   }, // INDEX_ENUM_NUM_HAL_PHY_RATES

   { //INDEX_ENUM_LAST
      {{nul},0, 0,{nul}},
   }, //INDEX_ENUM_LAST
}; // END _NV_TEMPLATE_ENUM


int getEnumNoOfFields(int enumIdx){
#ifdef WLAN_FEATURE_11AC
   enumNoOfFieldArray[7] = enumNoOfFieldArray[7] + 17;
#ifdef WCN_PRONTO
   enumNoOfFieldArray[7] = enumNoOfFieldArray[7] + 1;
#endif
   enumNoOfFieldArray[7] = enumNoOfFieldArray[7] + 9;
#ifdef WCN_PRONTO
   enumNoOfFieldArray[7] = enumNoOfFieldArray[7] + 1;
#endif
   enumNoOfFieldArray[7] = enumNoOfFieldArray[7] + 40;
#endif

   return enumNoOfFieldArray[enumIdx];
}


static int tableNoOfFieldArray[TABLE_BUILTIN_MAX] =
     {14,12,2,2,16,1,2,2,23,1,1,1,4,2,13};

_NV_TEMPLATE_TABLE NvTablesBuiltIn[/*TABLES_MAX*/][TABLE_ENTRIES_MAX] = {
   { // TABLE_sHalNv
      {"jz",_TABLE_IDX(TABLE_sNvFields),SINGULAR,0,0,0,
         ((int)&(nvDefaults.fields) - (int)&nvDefaults),"fields"},
      {"jm",_TABLE_IDX(TABLE_tRateGroupPwr),(ARRAY_1),
         ((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_ENUM))|
         (FIELD_SIZE_VALUE_BITS(INDEX_ENUM_NUM_RF_SUBBANDS))),0,0,
         ((int)&(nvDefaults.tables.pwrOptimum[0]) - (int)&nvDefaults.tables),
         "pwrOptimum"},
      {"dx",_TABLE_IDX(TABLE_sRegulatoryChannel),(ARRAY_1),
         ((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_ENUM))|
         (FIELD_SIZE_VALUE_BITS(INDEX_ENUM_NUM_RF_CHANNELS))),0,0,
         ((int)&(nvDefaults.tables.regDomains[0].channels[0]) -
          (int)&nvDefaults.tables.regDomains[0]),"channels"},
      {"jn",_TABLE_IDX(TABLE_sRegulatoryDomains),(ARRAY_1),
         ((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_ENUM))|
         (FIELD_SIZE_VALUE_BITS(INDEX_ENUM_NUM_REG_DOMAINS))),0,0,
         ((int)&(nvDefaults.tables.regDomains[0]) - (int)&nvDefaults.tables),
         "regDomains"},
      {"jo",_TABLE_IDX(TABLE_sDefaultCountry),SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.defaultCountryTable) -
          (int)&nvDefaults.tables),"defaultCountryTable"},
      {"jp",_TABLE_IDX(TABLE_tTpcPowerTable),(ARRAY_1),
         ((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_ENUM))|
         (FIELD_SIZE_VALUE_BITS(INDEX_ENUM_NUM_RF_CHANNELS))),0,0,
         ((int)&(nvDefaults.tables.plutCharacterized[0]) -
          (int)&nvDefaults.tables),"plutCharacterized"},
      {"jr",_TABLE_IDX(TABLE_tRateGroupPwrVR),(ARRAY_1),
         ((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_ENUM))|
         (FIELD_SIZE_VALUE_BITS(INDEX_ENUM_NUM_RF_SUBBANDS))),0,0,
         ((int)&(nvDefaults.tables.pwrOptimum_virtualRate[0]) -
          (int)&nvDefaults.tables),"pwrOptimum_virtualRate"},
      {"js",_TABLE_IDX(TABLE_sFwConfig),SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.fwConfig) - (int)&nvDefaults.tables),
         "fwConfig"},
      {"jt",_TABLE_IDX(TABLE_sRssiChannelOffsets),(_ADD_SIZE1(2)|ARRAY_1),
         ((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_INT))|(FIELD_SIZE_VALUE_BITS(2))),
         0,0,((int)&(nvDefaults.tables.rssiChanOffsets[0]) -
          (int)&nvDefaults.tables),"rssiChanOffsets"},
      {"er",_TABLE_IDX(TABLE_sCalData),SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.hwCalValues.calData) -
          (int)&nvDefaults.tables.hwCalValues),"calData"},
      {"ju",_TABLE_IDX(TABLE_sHwCalValues),SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.hwCalValues) - (int)&nvDefaults.tables),
         "hwCalValues"},
      {"jx",_TABLE_IDX(TABLE_sOfdmCmdPwrOffset),SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.ofdmCmdPwrOffset) - (int)&nvDefaults.tables),
         "ofdmCmdPwrOffset"},
      {"jy",_TABLE_IDX(TABLE_sTxBbFilterMode),SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.txbbFilterMode) - (int)&nvDefaults.tables),
         "txbbFilterMode"},
      {"j0",_TABLE_IDX(TABLE_sNvTables),SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables) - (int)&nvDefaults),"tables"},
      {{nul}, 0, 0, 0, 0, 0, 0,{nul}},
   }, // TABLE_sHalNv

   { // TABLE_sNvFields
      {"ap",_ID_U16,SINGULAR,0,0,0,((int)&(nvDefaults.fields.productId) -
          (int)&nvDefaults.fields),"productId"},
      {"aq",_ID_U8,SINGULAR,0,0,0,((int)&(nvDefaults.fields.productBands) -
          (int)&nvDefaults.fields),"productBands"},
      {"ar",_ID_U8,SINGULAR,0,0,0,((int)&(nvDefaults.fields.wlanNvRevId) -
          (int)&nvDefaults.fields),"wlanNvRevId"},
      {"as",_ID_U8,SINGULAR,0,0,0,((int)&(nvDefaults.fields.numOfTxChains) -
          (int)&nvDefaults.fields),"numOfTxChains"},
      {"at",_ID_U8,SINGULAR,0,0,0,((int)&(nvDefaults.fields.numOfRxChains) -
          (int)&nvDefaults.fields),"numOfRxChains"},
      {"au",_ID_U8,(_ADD_SIZE1(NV_FIELD_MAC_ADDR_SIZE)|ARRAY_1),
         ((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_INT))|
         (FIELD_SIZE_VALUE_BITS(NV_FIELD_MAC_ADDR_SIZE))),0,0,
         ((int)&(nvDefaults.fields.macAddr[0]) - (int)&nvDefaults.fields),
         "macAddr"},
      {"av",_ID_U8,(_ADD_SIZE1(NV_FIELD_MAC_ADDR_SIZE)|ARRAY_1),
         ((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_INT))|
         (FIELD_SIZE_VALUE_BITS(NV_FIELD_MAC_ADDR_SIZE))),0,0,
         ((int)&(nvDefaults.fields.macAddr2[0]) - (int)&nvDefaults.fields),
         "macAddr2"},
      {"aw",_ID_U8,(_ADD_SIZE1(NV_FIELD_MAC_ADDR_SIZE)|ARRAY_1),
         ((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_INT))|
         (FIELD_SIZE_VALUE_BITS(NV_FIELD_MAC_ADDR_SIZE))),0,0,
         ((int)&(nvDefaults.fields.macAddr3[0]) - (int)&nvDefaults.fields),
         "macAddr3"},
      {"ax",_ID_U8,(_ADD_SIZE1(NV_FIELD_MAC_ADDR_SIZE)|ARRAY_1),
         ((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_INT))|
         (FIELD_SIZE_VALUE_BITS(NV_FIELD_MAC_ADDR_SIZE))),0,0,
         ((int)&(nvDefaults.fields.macAddr4[0]) - (int)&nvDefaults.fields),
         "macAddr4"},
      {"ay",_ID_U8,(_ADD_SIZE1(NV_FIELD_MFG_SN_SIZE)|ARRAY_1),
         ((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_INT))|
         (FIELD_SIZE_VALUE_BITS(NV_FIELD_MFG_SN_SIZE))),0,0,
         ((int)&(nvDefaults.fields.mfgSN[0]) - (int)&nvDefaults.fields),"mfgSN"}
         ,
      {"az",_ID_U8,SINGULAR,0,0,0,((int)&(nvDefaults.fields.couplerType) -
          (int)&nvDefaults.fields),"couplerType"},
      {"a0",_ID_U8,SINGULAR,0,0,0,((int)&(nvDefaults.fields.nvVersion) -
          (int)&nvDefaults.fields),"nvVersion"},
      {{nul}, 0, 0, 0, 0, 0, 0,{nul}},
   }, // TABLE_sNvFields

   { // TABLE_sRegulatoryChannel
      {"dv",_ID_U8,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.regDomains[0].channels[0].enabled) -
          (int)&nvDefaults.tables.regDomains[0].channels[0]),"enabled"},
      {"dw",_ID_S8,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.regDomains[0].channels[0].pwrLimit) -
          (int)&nvDefaults.tables.regDomains[0].channels[0]),"pwrLimit"},
      {{nul}, 0, 0, 0, 0, 0, 0,{nul}},
   }, // TABLE_sRegulatoryChannel

   { // TABLE_sRssiChannelOffsets
      {"d1",_ID_S16,(ARRAY_1),((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_ENUM))|
         (FIELD_SIZE_VALUE_BITS(INDEX_ENUM_NUM_RF_CHANNELS))),0,0,
         ((int)&(nvDefaults.tables.rssiChanOffsets[0].bRssiOffset[0]) -
          (int)&nvDefaults.tables.rssiChanOffsets[0]),"bRssiOffset"},
      {"d2",_ID_S16,(ARRAY_1),((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_ENUM))|
         (FIELD_SIZE_VALUE_BITS(INDEX_ENUM_NUM_RF_CHANNELS))),0,0,
         ((int)&(nvDefaults.tables.rssiChanOffsets[0].gnRssiOffset[0]) -
          (int)&nvDefaults.tables.rssiChanOffsets[0]),"gnRssiOffset"},
      {{nul}, 0, 0, 0, 0, 0, 0,{nul}},
   }, // TABLE_sRssiChannelOffsets

   { // TABLE_sCalData
      {"ea",_ID_U16,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.hwCalValues.calData.psSlpTimeOvrHd2G) -
          (int)&nvDefaults.tables.hwCalValues.calData),"psSlpTimeOvrHd2G"},
      {"eb",_ID_U16,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.hwCalValues.calData.psSlpTimeOvrHd5G) -
          (int)&nvDefaults.tables.hwCalValues.calData),"psSlpTimeOvrHd5G"},
      {"ec",_ID_U16,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.hwCalValues.calData.psSlpTimeOvrHdxLNA5G) -
          (int)&nvDefaults.tables.hwCalValues.calData),"psSlpTimeOvrHdxLNA5G"},
      {"ed",_ID_U8,SINGULAR,0,0,0,
         (((int)&(nvDefaults.tables.hwCalValues.calData.psSlpTimeOvrHdxLNA5G) +
          sizeof(uint16)) - (int)&nvDefaults.tables.hwCalValues.calData),
         "nv_TxBBFSel9MHz"},
      {"ee",_ID_U8,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.hwCalValues.calData.hwParam2) -
          (int)&nvDefaults.tables.hwCalValues.calData),"hwParam2"},
      {"ef",_ID_U16,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.hwCalValues.calData.custom_tcxo_reg8) -
          (int)&nvDefaults.tables.hwCalValues.calData),"custom_tcxo_reg8"},
      {"eg",_ID_U16,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.hwCalValues.calData.custom_tcxo_reg9) -
          (int)&nvDefaults.tables.hwCalValues.calData),"custom_tcxo_reg9"},
      {"eh",_ID_U32,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.hwCalValues.calData.hwParam3) -
          (int)&nvDefaults.tables.hwCalValues.calData),"hwParam3"},
      {"ei",_ID_U32,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.hwCalValues.calData.hwParam4) -
          (int)&nvDefaults.tables.hwCalValues.calData),"hwParam4"},
      {"ej",_ID_U32,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.hwCalValues.calData.hwParam5) -
          (int)&nvDefaults.tables.hwCalValues.calData),"hwParam5"},
      {"ek",_ID_U32,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.hwCalValues.calData.hwParam6) -
          (int)&nvDefaults.tables.hwCalValues.calData),"hwParam6"},
      {"el",_ID_U32,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.hwCalValues.calData.hwParam7) -
          (int)&nvDefaults.tables.hwCalValues.calData),"hwParam7"},
      {"em",_ID_U32,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.hwCalValues.calData.hwParam8) -
          (int)&nvDefaults.tables.hwCalValues.calData),"hwParam8"},
      {"en",_ID_U32,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.hwCalValues.calData.hwParam9) -
          (int)&nvDefaults.tables.hwCalValues.calData),"hwParam9"},
      {"eo",_ID_U32,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.hwCalValues.calData.hwParam10) -
          (int)&nvDefaults.tables.hwCalValues.calData),"hwParam10"},
      {"ep",_ID_U32,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.hwCalValues.calData.hwParam11) -
          (int)&nvDefaults.tables.hwCalValues.calData),"hwParam11"},
      {{nul}, 0, 0, 0, 0, 0, 0,{nul}},
   }, // TABLE_sCalData

   { // TABLE_sTxBbFilterMode
      {"es",_ID_U32,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.txbbFilterMode.txFirFilterMode) -
          (int)&nvDefaults.tables.txbbFilterMode),"txFirFilterMode"},
      {{nul}, 0, 0, 0, 0, 0, 0,{nul}},
   }, // TABLE_sTxBbFilterMode

   { // TABLE_sOfdmCmdPwrOffset
      {"et",_ID_S16,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.ofdmCmdPwrOffset.ofdmPwrOffset) -
          (int)&nvDefaults.tables.ofdmCmdPwrOffset),"ofdmPwrOffset"},
      {"eu",_ID_S16,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.ofdmCmdPwrOffset.rsvd) -
          (int)&nvDefaults.tables.ofdmCmdPwrOffset),"rsvd"},
      {{nul}, 0, 0, 0, 0, 0, 0,{nul}},
   }, // TABLE_sOfdmCmdPwrOffset

   { // TABLE_sDefaultCountry
      {"if",_ID_U8,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.defaultCountryTable.regDomain) -
          (int)&nvDefaults.tables.defaultCountryTable),"regDomain"},
      {"ig",_ID_U8,(_ADD_SIZE1(NV_FIELD_COUNTRY_CODE_SIZE)|ARRAY_1),
         ((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_INT))|
         (FIELD_SIZE_VALUE_BITS(NV_FIELD_COUNTRY_CODE_SIZE))),0,0,
         ((int)&(nvDefaults.tables.defaultCountryTable.countryCode[0]) -
          (int)&nvDefaults.tables.defaultCountryTable),"countryCode"},
      {{nul}, 0, 0, 0, 0, 0, 0,{nul}},
   }, // TABLE_sDefaultCountry

   { // TABLE_sFwConfig
      {"ih",_ID_U8,SINGULAR,0,0,0,((int)&(nvDefaults.tables.fwConfig.skuID) -
          (int)&nvDefaults.tables.fwConfig),"skuID"},
      {"ii",_ID_U8,SINGULAR,0,0,0,((int)&(nvDefaults.tables.fwConfig.tpcMode2G)
          - (int)&nvDefaults.tables.fwConfig),"tpcMode2G"},
      {"ij",_ID_U8,SINGULAR,0,0,0,((int)&(nvDefaults.tables.fwConfig.tpcMode5G)
          - (int)&nvDefaults.tables.fwConfig),"tpcMode5G"},
      {"ik",_ID_U8,SINGULAR,0,0,0,((int)&(nvDefaults.tables.fwConfig.configItem1)
          - (int)&nvDefaults.tables.fwConfig),"configItem1"},
      {"il",_ID_U8,SINGULAR,0,0,0,((int)&(nvDefaults.tables.fwConfig.xPA2G) -
          (int)&nvDefaults.tables.fwConfig),"xPA2G"},
      {"im",_ID_U8,SINGULAR,0,0,0,((int)&(nvDefaults.tables.fwConfig.xPA5G) -
          (int)&nvDefaults.tables.fwConfig),"xPA5G"},
      {"in",_ID_U8,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.fwConfig.extPaCtrl0Polarity) -
          (int)&nvDefaults.tables.fwConfig),"extPaCtrl0Polarity"},
      {"io",_ID_U8,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.fwConfig.extPaCtrl1Polarity) -
          (int)&nvDefaults.tables.fwConfig),"extPaCtrl1Polarity"},
      {"ip",_ID_U8,SINGULAR,0,0,0,((int)&(nvDefaults.tables.fwConfig.xLNA2G) -
          (int)&nvDefaults.tables.fwConfig),"xLNA2G"},
      {"iq",_ID_U8,SINGULAR,0,0,0,((int)&(nvDefaults.tables.fwConfig.xLNA5G) -
          (int)&nvDefaults.tables.fwConfig),"xLNA5G"},
      {"ir",_ID_U8,SINGULAR,0,0,0,((int)&(nvDefaults.tables.fwConfig.xCoupler2G)
          - (int)&nvDefaults.tables.fwConfig),"xCoupler2G"},
      {"is",_ID_U8,SINGULAR,0,0,0,((int)&(nvDefaults.tables.fwConfig.xCoupler5G)
          - (int)&nvDefaults.tables.fwConfig),"xCoupler5G"},
      {"it",_ID_U8,SINGULAR,0,0,0,((int)&(nvDefaults.tables.fwConfig.xPdet2G) -
          (int)&nvDefaults.tables.fwConfig),"xPdet2G"},
      {"iu",_ID_U8,SINGULAR,0,0,0,((int)&(nvDefaults.tables.fwConfig.xPdet5G) -
          (int)&nvDefaults.tables.fwConfig),"xPdet5G"},
      {"iv",_ID_U8,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.fwConfig.enableDPD2G) -
          (int)&nvDefaults.tables.fwConfig),"enableDPD2G"},
      {"iw",_ID_U8,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.fwConfig.enableDPD5G) -
          (int)&nvDefaults.tables.fwConfig),"enableDPD5G"},
      {"ix",_ID_U8,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.fwConfig.pdadcSelect2G) -
          (int)&nvDefaults.tables.fwConfig),"pdadcSelect2G"},
      {"iy",_ID_U8,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.fwConfig.pdadcSelect5GLow) -
          (int)&nvDefaults.tables.fwConfig),"pdadcSelect5GLow"},
      {"iz",_ID_U8,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.fwConfig.pdadcSelect5GMid) -
          (int)&nvDefaults.tables.fwConfig),"pdadcSelect5GMid"},
      {"i0",_ID_U8,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.fwConfig.pdadcSelect5GHigh) -
          (int)&nvDefaults.tables.fwConfig),"pdadcSelect5GHigh"},
      {"i1",_ID_U32,SINGULAR,0,0,0,((int)&(nvDefaults.tables.fwConfig.configItem2)
          - (int)&nvDefaults.tables.fwConfig),"configItem2"},
      {"i2",_ID_U32,SINGULAR,0,0,0,((int)&(nvDefaults.tables.fwConfig.configItem3)
          - (int)&nvDefaults.tables.fwConfig),"configItem3"},
      {"i3",_ID_U32,SINGULAR,0,0,0,((int)&(nvDefaults.tables.fwConfig.configItem4)
          - (int)&nvDefaults.tables.fwConfig),"configItem4"},
      {{nul}, 0, 0, 0, 0, 0, 0,{nul}},
   }, // TABLE_sFwConfig

   { // TABLE_tTpcPowerTable
      {"kc",_ID_U8,(_ADD_SIZE2(TPC_MEM_POWER_LUT_DEPTH)|ARRAY_2),
         ((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_ENUM))|
         (FIELD_SIZE_VALUE_BITS(INDEX_ENUM_NUM_PHY_MAX_TX_CHAINS))),
         ((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_INT))|
         (FIELD_SIZE_VALUE_BITS(TPC_MEM_POWER_LUT_DEPTH))),0,
         ((int)&(nvDefaults.tables.plutCharacterized[0]) -
          (int)&nvDefaults.tables.plutCharacterized[0]),"tTpcPowerTable"},
      {{nul}, 0, 0, 0, 0, 0, 0,{nul}},
   }, // TABLE_tTpcPowerTable

   { // TABLE_tRateGroupPwr
      {"kb",_ID_U32,(ARRAY_1),((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_ENUM))|
         (FIELD_SIZE_VALUE_BITS(INDEX_ENUM_NUM_HAL_PHY_RATES))),0,0,
         ((int)&(nvDefaults.tables.pwrOptimum[0]) -
          (int)&nvDefaults.tables.pwrOptimum[0]),"tRateGroupPwr"},
      {{nul}, 0, 0, 0, 0, 0, 0,{nul}},
   }, // TABLE_tRateGroupPwr

   { // TABLE_tRateGroupPwrVR
      {"kd",_ID_U32,(_ADD_SIZE1(NUM_RF_VR_RATE)|ARRAY_1),
         ((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_INT))|
         (FIELD_SIZE_VALUE_BITS(NUM_RF_VR_RATE))),0,0,
         ((int)&(nvDefaults.tables.pwrOptimum_virtualRate[0]) -
          (int)&nvDefaults.tables.pwrOptimum_virtualRate[0]),"tRateGroupPwrVR"},
      {{nul}, 0, 0, 0, 0, 0, 0,{nul}},
   }, // TABLE_tRateGroupPwrVR

   { // TABLE_sRegulatoryDomains
      {"dx",_TABLE_IDX(TABLE_sRegulatoryChannel),(ARRAY_1),
         ((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_ENUM))|
         (FIELD_SIZE_VALUE_BITS(INDEX_ENUM_NUM_RF_CHANNELS))),0,0,
         ((int)&(nvDefaults.tables.regDomains[0].channels[0]) -
          (int)&nvDefaults.tables.regDomains[0]),"channels"},
      {"dy",_ID_U32,(ARRAY_1),((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_ENUM))|
         (FIELD_SIZE_VALUE_BITS(INDEX_ENUM_NUM_RF_SUBBANDS))),0,0,
         ((int)&(nvDefaults.tables.regDomains[0].antennaGain[0]) -
          (int)&nvDefaults.tables.regDomains[0]),"antennaGain"},
      {"dz",_ID_U32,(ARRAY_1),((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_ENUM))|
         (FIELD_SIZE_VALUE_BITS(INDEX_ENUM_NUM_2_4GHZ_CHANNELS))),0,0,
         ((int)&(nvDefaults.tables.regDomains[0].bRatePowerOffset[0]) -
          (int)&nvDefaults.tables.regDomains[0]),"bRatePowerOffset"},
      {"d0",_ID_U32,(ARRAY_1),((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_ENUM))|
         (FIELD_SIZE_VALUE_BITS(INDEX_ENUM_NUM_RF_CHANNELS))),0,0,
         ((int)&(nvDefaults.tables.regDomains[0].gnRatePowerOffset[0]) -
          (int)&nvDefaults.tables.regDomains[0]),"gnRatePowerOffset"},
      {{nul}, 0, 0, 0, 0, 0, 0,{nul}},
   }, // TABLE_sRegulatoryDomains

   { // TABLE_sHwCalValues
      {"eq",_ID_U32,SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.hwCalValues.validBmap) -
          (int)&nvDefaults.tables.hwCalValues),"validBmap"},
      {"er",_TABLE_IDX(TABLE_sCalData),SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.hwCalValues.calData) -
          (int)&nvDefaults.tables.hwCalValues),"calData"},
      {{nul}, 0, 0, 0, 0, 0, 0,{nul}},
   }, // TABLE_sHwCalValues

   { // TABLE_sNvTables
      {"jm",_TABLE_IDX(TABLE_tRateGroupPwr),(ARRAY_1),
         ((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_ENUM))|
         (FIELD_SIZE_VALUE_BITS(INDEX_ENUM_NUM_RF_SUBBANDS))),0,0,
         ((int)&(nvDefaults.tables.pwrOptimum[0]) - (int)&nvDefaults.tables),
         "pwrOptimum"},
      {"jn",_TABLE_IDX(TABLE_sRegulatoryDomains),(ARRAY_1),
         ((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_ENUM))|
         (FIELD_SIZE_VALUE_BITS(INDEX_ENUM_NUM_REG_DOMAINS))),0,0,
         ((int)&(nvDefaults.tables.regDomains[0]) - (int)&nvDefaults.tables),
         "regDomains"},
      {"jo",_TABLE_IDX(TABLE_sDefaultCountry),SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.defaultCountryTable) -
          (int)&nvDefaults.tables),"defaultCountryTable"},
      {"jp",_TABLE_IDX(TABLE_tTpcPowerTable),(ARRAY_1),
         ((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_ENUM))|
         (FIELD_SIZE_VALUE_BITS(INDEX_ENUM_NUM_RF_CHANNELS))),0,0,
         ((int)&(nvDefaults.tables.plutCharacterized[0]) -
          (int)&nvDefaults.tables),"plutCharacterized"},
      {"jq",_ID_S16,(ARRAY_1),((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_ENUM))|
         (FIELD_SIZE_VALUE_BITS(INDEX_ENUM_NUM_RF_CHANNELS))),0,0,
         ((int)&(nvDefaults.tables.plutPdadcOffset[0]) -
          (int)&nvDefaults.tables),"plutPdadcOffset"},
      {"jr",_TABLE_IDX(TABLE_tRateGroupPwrVR),(ARRAY_1),
         ((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_ENUM))|
         (FIELD_SIZE_VALUE_BITS(INDEX_ENUM_NUM_RF_SUBBANDS))),0,0,
         ((int)&(nvDefaults.tables.pwrOptimum_virtualRate[0]) -
          (int)&nvDefaults.tables),"pwrOptimum_virtualRate"},
      {"js",_TABLE_IDX(TABLE_sFwConfig),SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.fwConfig) - (int)&nvDefaults.tables),
         "fwConfig"},
      {"jt",_TABLE_IDX(TABLE_sRssiChannelOffsets),(_ADD_SIZE1(2)|ARRAY_1),
         ((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_INT))|(FIELD_SIZE_VALUE_BITS(2))),
         0,0,((int)&(nvDefaults.tables.rssiChanOffsets[0]) -
          (int)&nvDefaults.tables),"rssiChanOffsets"},
      {"ju",_TABLE_IDX(TABLE_sHwCalValues),SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.hwCalValues) - (int)&nvDefaults.tables),
         "hwCalValues"},
      {"jv",_ID_S16,(ARRAY_1),((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_ENUM))|
         (FIELD_SIZE_VALUE_BITS(INDEX_ENUM_NUM_RF_CHANNELS))),0,0,
         ((int)&(nvDefaults.tables.antennaPathLoss[0]) -
          (int)&nvDefaults.tables),"antennaPathLoss"},
      {"jw",_ID_S16,(ARRAY_2),((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_ENUM))|
         (FIELD_SIZE_VALUE_BITS(INDEX_ENUM_NUM_802_11_MODES))),
         ((FIELD_SIZE_TYPE_BIT(FIELD_SIZE_IDX_ENUM))|
         (FIELD_SIZE_VALUE_BITS(INDEX_ENUM_NUM_RF_CHANNELS))),0,
         ((int)&(nvDefaults.tables.pktTypePwrLimits[0][0]) -
          (int)&nvDefaults.tables),"pktTypePwrLimits"},
      {"jx",_TABLE_IDX(TABLE_sOfdmCmdPwrOffset),SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.ofdmCmdPwrOffset) - (int)&nvDefaults.tables),
         "ofdmCmdPwrOffset"},
      {"jy",_TABLE_IDX(TABLE_sTxBbFilterMode),SINGULAR,0,0,0,
         ((int)&(nvDefaults.tables.txbbFilterMode) - (int)&nvDefaults.tables),
         "txbbFilterMode"},
      {{nul}, 0, 0, 0, 0, 0, 0,{nul}},
   }, // TABLE_sNvTables

   { //INDEX_TABLE_LAST
      {{nul}, 0, 0, 0, 0, 0, 0,{nul}},
   }, //INDEX_TABLE_LAST
}; // END _NV_TEMPLATE_TABLE

int getTableNoOfFields(int tblIdx)
{
   return tableNoOfFieldArray[tblIdx];
}
