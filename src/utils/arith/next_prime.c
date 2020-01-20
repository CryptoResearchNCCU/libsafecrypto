/*****************************************************************************
 * Copyright (C) Queen's University Belfast, ECIT, 2016                      *
 *                                                                           *
 * This file is part of libsafecrypto.                                       *
 *                                                                           *
 * This file is subject to the terms and conditions defined in the file      *
 * 'LICENSE', which is part of this source code package.                     *
 *****************************************************************************/

/*
 * Git commit information:
 *   Author: $SC_AUTHOR$
 *   Date:   $SC_DATE$
 *   Branch: $SC_BRANCH$
 *   Id:     $SC_IDENT$
 */

#include "next_prime.h"
#include "utils/arith/limb.h"
#include "safecrypto_types.h"
#include "safecrypto_private.h"
#include "safecrypto_debug.h"

#include <math.h>


#if SC_LIMB_BITS == 64
#define SC_MAX_PRIME SC_LIMB_WORD(18446744073709551557)
#else
#define SC_MAX_PRIME SC_LIMB_WORD(4294967291)
#endif

#define SC_MOD_MSB   (SC_LIMB_WORD(1) << (SC_LIMB_BITS - 1))

const UINT16 small_primes[NUM_SMALL_PRIMES] = {
    2,   3,   5,   7,   11,  13,  17,  19,  23,  29,  31,  37,
    41,  43,  47,  53,  59,  61,  67,  71,  73,  79,  83,  89,
    97,  101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151,
    157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223,
    227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281,
    283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359,
    367, 373, 379, 383, 389, 397, 401, 409, 419, 421, 431, 433,
    439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 503,
    509, 521, 523, 541, 547, 557, 563, 569, 571, 577, 587, 593,
    599, 601, 607, 613, 617, 619, 631, 641, 643, 647, 653, 659,
    661, 673, 677, 683, 691, 701, 709, 719, 727, 733, 739, 743,
    751, 757, 761, 769, 773, 787, 797, 809, 811, 821, 823, 827,
    829, 839, 853, 857, 859, 863, 877, 881, 883, 887, 907, 911,
    919, 929, 937, 941, 947, 953, 967, 971, 977, 983, 991, 997,
    1009, 1013, 1019, 1021
};

static const UINT16 modular_primes[NUM_MOD_PRIMES] = {
#if SC_LIMB_BITS == 64
   29, 99, 123, 131, 155, 255, 269, 359, 435, 449, 453, 485, 491, 543, 585, 599, 
    753, 849, 879, 885, 903, 995, 1209, 1251, 1311, 1373, 1403, 1485, 1533, 1535, 1545, 1551, 
    1575, 1601, 1625, 1655, 1701, 1709, 1845, 1859, 1913, 1995, 2045, 2219, 2229, 2321, 2363, 2385, 
    2483, 2499, 2523, 2543, 2613, 2639, 2679, 2829, 2931, 3089, 3165, 3189, 3245, 3273, 3291, 3341, 
    3365, 3531, 3543, 3549, 3651, 3683, 3783, 3819, 3825, 3855, 3923, 3945, 3981, 4005, 4023, 4083, 
    4145, 4163, 4209, 4223, 4265, 4281, 4289, 4331, 4355, 4361, 4385, 4391, 4431, 4433, 4443, 4475, 
    4589, 4743, 4803, 4839, 4863, 4875, 4883, 4911, 4929, 4985, 5009, 5013, 5021, 5031, 5051, 5139, 
    5163, 5171, 5201, 5229, 5301, 5315, 5381, 5441, 5495, 5559, 5565, 5681, 5799, 5801, 5813, 5835, 
    5859, 5903, 5993, 6093, 6113, 6123, 6155, 6209, 6303, 6351, 6353, 6363, 6393, 6443, 6465, 6491, 
    6563, 6639, 6653, 6659, 6671, 6693, 6749, 6773, 6821, 6855, 6969, 7059, 7179, 7185, 7239, 7271, 
    7281, 7353, 7359, 7409, 7421, 7451, 7515, 7523, 7589, 7595, 7661, 7691, 7695, 7733, 7745, 7763, 
    7775, 7785, 7793, 7799, 7809, 7841, 7859, 7869, 7899, 7925, 7953, 8031, 8141, 8153, 8223, 8225, 
    8229, 8261, 8265, 8291, 8375, 8421, 8519, 8603, 8639, 8699, 8751, 8853, 8919, 8955, 9005, 9033, 
    9095, 9125, 9201, 9279, 9285, 9291, 9305, 9371, 9399, 9455, 9525, 9549, 9555, 9581, 9593, 9609, 
    9641, 9671, 9675, 9711, 9723, 9789, 9803, 9839, 9879, 9999, 10011, 10125, 10133, 10139, 10161, 10193, 
    10271, 10281, 10319, 10329, 10449, 10455, 10551, 10553, 10589, 10593, 10601, 10605, 10659, 10685, 10733, 10815, 
    10823, 10865, 10901, 10931, 10955, 11123, 11169, 11229, 11235, 11243, 11351, 11373, 11375, 11529, 11631, 11645, 
    11753, 11763, 11793, 11799, 11859, 11903, 11909, 11919, 11981, 12111, 12119, 12155, 12185, 12201, 12215, 12261, 
    12269, 12311, 12321, 12335, 12365, 12369, 12395, 12399, 12425, 12461, 12533, 12623, 12695, 12761, 12789, 12803, 
    12813, 12821, 12885, 13029, 13101, 13185, 13205, 13253, 13281, 13305, 13389, 13403, 13409, 13433, 13449, 13535, 
    13619, 13673, 13679, 13683, 13779, 13791, 13815, 13853, 13875, 13881, 13893, 14043, 14075, 14103, 14129, 14133, 
    14199, 14255, 14295, 14309, 14315, 14351, 14375, 14439, 14529, 14549, 14729, 14759, 14771, 14789, 14871, 14889, 
    14915, 14925, 14931, 14933, 15003, 15045, 15083, 15173, 15233, 15305, 15339, 15533, 15593, 15599, 15639, 15671, 
    15681, 15713, 15723, 15731, 15765, 15855, 15905, 15915, 15963, 15983, 15993, 16013, 16049, 16053, 16061, 16095, 
    16115, 16139, 16185, 16521, 16559, 16601, 16689, 16713, 16725, 16733, 16845, 16863, 16955, 16959, 16985, 17013, 
    17091, 17103, 17111, 17193, 17223, 17241, 17265, 17313, 17351, 17411, 17439, 17453, 17463, 17483, 17543, 17655, 
    17675, 17679, 17703, 17715, 17729, 17741, 17775, 17873, 17885, 17901, 17913, 17921, 17969, 18009, 18015, 18053, 
    18135, 18273, 18371, 18485, 18513, 18539, 18611, 18755, 18771, 18821, 18869, 18875, 18935, 18939, 18971, 18981, 
    19059, 19071, 19103, 19113, 19173, 19233, 19241, 19299, 19301, 19313, 19395, 19463, 19551, 19569, 19601, 19623, 
    19701, 19721, 19883, 19889, 19931, 20003, 20015, 20031, 20073, 20091, 20139, 20169, 20193, 20343, 20349, 20429, 
    20489, 20501, 20553, 20643, 20685, 20709, 20721, 20853, 20891, 20951, 21051, 21089, 21099, 21119, 21135, 21201, 
    21215, 21333, 21339, 21389, 21401, 21449, 21483, 21555, 21633, 21641, 21789, 21815, 21885, 21983, 21995, 22065, 
    22085, 22121, 22139, 22239, 22263, 22335, 22373, 22473, 22503, 22589, 22613, 22655, 22661, 22673, 22713, 22719, 
    22913, 22923, 22995, 23109, 23259, 23303, 23343, 23361, 23375, 23423, 23453, 23529, 23571, 23573, 23585, 23705, 
    23753, 23805, 23819, 23853, 23973, 23991, 24035, 24045, 24069, 24113, 24195, 24273, 24329, 24371, 24431, 24521, 
    24563, 24593, 24603, 24635, 24669, 24711, 24773, 24803, 24843, 24959, 24999, 25001, 25023, 25085, 25163, 25175, 
    25245, 25275, 25323, 25341, 25349, 25439, 25455, 25469, 25533, 25649, 25659, 25733, 25739, 25835, 25851, 25859, 
    25881, 25889, 25905, 25935, 25949, 25959, 25965, 25991, 26043, 26061, 26075, 26091, 26093, 26181, 26211, 26219, 
    26229, 26241, 26475, 26583, 26615, 26619, 26633, 26651, 26709, 26789, 26855, 26925, 26939, 26969, 26979, 27003, 
    27039, 27101, 27185, 27203, 27315, 27441, 27479, 27545, 27555, 27561, 27573, 27581, 27609, 27681, 27711, 27725, 
    27753, 27795, 27891, 27933, 27941, 27945, 27963, 28019, 28061, 28065, 28091, 28131, 28155, 28173, 28175, 28221, 
    28275, 28289, 28331, 28355, 28359, 28403, 28541, 28635, 28649, 28659, 28731, 28883, 28893, 29013, 29015, 29079, 
    29121, 29171, 29193, 29235, 29243, 29283, 29309, 29325, 29361, 29363, 29475, 29493, 29555, 29573, 29745, 29753, 
    29789, 29841, 29849, 29895, 29921, 29951, 29975, 29981, 29985, 30005, 30023, 30131, 30153, 30159, 30333, 30381, 
    30413, 30461, 30509, 30551, 30569, 30599, 30611, 30665, 30669, 30689, 30723, 30731, 30735, 30831, 30839, 30881, 
    30975, 31005, 31013, 31043, 31059, 31083, 31109, 31131, 31181, 31251, 31295, 31301, 31319, 31385, 31433, 31451, 
    31493, 31539, 31589, 31593, 31613, 31623, 31635, 31641, 31659, 31811, 31833, 31839, 31893, 31943, 31965, 32081, 
    32135, 32181, 32195, 32285, 32309, 32363, 32379, 32391, 32399, 32421, 32603, 32685, 32741, 32775, 32781, 32861, 
    32865, 32931, 33075, 33159, 33191, 33203, 33269, 33299, 33323, 33329, 33429, 33491, 33513, 33521, 33551, 33581, 
    33609, 33629, 33639, 33645, 33671, 33681, 33801, 33803, 33903, 33915, 33995, 34029, 34073, 34079, 34115, 34181, 
    34209, 34311, 34349, 34385, 34409, 34413, 34445, 34493, 34503, 34545, 34605, 34683, 34835, 34869, 34889, 35021, 
    35099, 35199, 35219, 35381, 35415, 35421, 35439, 35441, 35451, 35471, 35499, 35505, 35583, 35679, 35711, 35723, 
    35763, 35795, 35843, 35945, 35975, 35985, 35991, 36009, 36011, 36023, 36033, 36075, 36125, 36179, 36183, 36279, 
    36299, 36311, 36335, 36345, 36351, 36401, 36425, 36459, 36471, 36485, 36491, 36515, 36533, 36543, 36555, 36563, 
    36579, 36675, 36771, 36803, 36825, 36989, 36993, 37023, 37025, 37091, 37145, 37173, 37233, 37269, 37335, 37361, 
    37455, 37473, 37479, 37509, 37541, 37571, 37641, 37691, 37749, 37779, 37791, 37803, 37815, 37823, 37853, 37859, 
    37961, 37985, 38015, 38055, 38075, 38111, 38231, 38253, 38273, 38325, 38379, 38399, 38465, 38489, 38493, 38501, 
    38549, 38561, 38565, 38673, 38699, 38715, 38813, 38825, 38883, 38925, 39035, 39039, 39041, 39063, 39089, 39155, 
    39201, 39221, 39275, 39299, 39375, 39413, 39519, 39545, 39615, 39653, 39683, 39743, 39929, 39945, 39989, 40001, 
    40073, 40085, 40173, 40215, 40275, 40301, 40401, 40415, 40425, 40449, 40469, 40475, 40481, 40535, 40625, 40631, 
    40635, 40685, 40853, 40875, 40941, 40959, 40971, 41051, 41069, 41093, 41223, 41235, 41265, 41375, 41703, 41745, 
    41769, 41829, 41933, 41973, 42033, 42065, 42129, 42149, 42159, 42233, 42305, 42315, 42401, 42459, 42603, 42641, 
    42659, 42701, 42773, 42791, 42819, 42833, 42849, 42855, 42863, 42939, 42959, 42971, 43019, 43029, 43031, 43089, 
    43185, 43223, 43229, 43235, 43275, 43293, 43335, 43403, 43505, 43509, 43515, 43521, 43551, 43601, 43635, 43641
#else
    11, 45, 65, 95, 129, 135, 165, 209, 219, 221, 239, 245, 281, 303, 345, 351,
    359, 389, 393, 395, 413, 435, 461, 513, 519, 549, 555, 573, 575, 585, 591, 611,
    623, 629, 683, 689, 701, 729, 785, 791, 813, 843, 851, 869, 879, 893, 905, 921,
    953, 963, 965, 969, 993, 1031, 1049, 1073, 1085, 1103, 1143, 1173, 1203, 1221, 1229, 1271,
    1281, 1301, 1313, 1343, 1409, 1425, 1431, 1449, 1473, 1485, 1493, 1533, 1541, 1553, 1599, 1605,
    1611, 1613, 1619, 1661, 1689, 1695, 1709, 1721, 1733, 1763, 1773, 1775, 1785, 1799, 1803, 1851,
    1869, 1875, 1899, 1911, 1919, 1925, 1943, 2001, 2025, 2039, 2111, 2159, 2169, 2171, 2181, 2193,
    2195, 2211, 2213, 2219, 2223, 2229, 2241, 2249, 2265, 2279, 2291, 2321, 2351, 2361, 2369, 2373,
    2375, 2463, 2481, 2499, 2549, 2555, 2601, 2621, 2645, 2663, 2669, 2673, 2681, 2691, 2739, 2751,
    2765, 2775, 2783, 2811, 2853, 2859, 2865, 2879, 2883, 2909, 2939, 2951, 2969, 2993, 3021, 3041,
    3059, 3063, 3093, 3101, 3143, 3171, 3179, 3195, 3201, 3203, 3209, 3231, 3261, 3279, 3321, 3333,
    3381, 3401, 3411, 3429, 3465, 3509, 3521, 3539, 3555, 3579, 3599, 3611, 3621, 3653, 3695, 3711,
    3725, 3753, 3755, 3761, 3801, 3803, 3825, 3861, 3893, 3923, 3951, 3959, 3963, 3975, 3989, 4001,
    4041, 4043, 4053, 4055, 4103, 4109, 4125, 4131, 4133, 4151, 4155, 4169, 4179, 4181, 4185, 4221,
    4223, 4251, 4259, 4283, 4293, 4353, 4361, 4379, 4395, 4413, 4419, 4463, 4503, 4505, 4515, 4523,
    4599, 4601, 4631, 4659, 4685, 4733, 4763, 4769, 4781, 4869, 4895, 4911, 4925, 4931, 4965, 4971,
    4995, 5043, 5079, 5093, 5103, 5145, 5211, 5219, 5259, 5265, 5271, 5349, 5369, 5375, 5411, 5429,
    5435, 5439, 5441, 5495, 5555, 5559, 5595, 5615, 5643, 5649, 5651, 5699, 5753, 5775, 5789, 5793,
    5825, 5835, 5855, 5889, 5895, 5931, 5939, 5991, 6003, 6005, 6051, 6065, 6069, 6149, 6171, 6173,
    6195, 6201, 6225, 6251, 6261, 6269, 6293, 6299, 6309, 6321, 6363, 6405, 6413, 6429, 6441, 6461,
    6513, 6569, 6581, 6591, 6615, 6635, 6695, 6713, 6765, 6803, 6815, 6839, 6881, 6891, 6909, 6929,
    6933, 6941, 6945, 7011, 7041, 7079, 7083, 7091, 7121, 7125, 7149, 7151, 7163, 7185, 7223, 7235,
    7251, 7275, 7305, 7319, 7325, 7349, 7353, 7359, 7371, 7385, 7389, 7395, 7413, 7419, 7421, 7433,
    7443, 7463, 7475, 7479, 7491, 7545, 7553, 7569, 7571, 7575, 7589, 7601, 7613, 7625, 7629, 7671,
    7695, 7701, 7713, 7721, 7755, 7769, 7829, 7869, 7889, 7895, 7899, 7905, 7911, 7925, 7935, 7949,
    7955, 7959, 7961, 8043, 8051, 8085, 8109, 8133, 8183, 8189, 8249, 8255, 8289, 8313, 8375, 8381,
    8421, 8511, 8513, 8535, 8583, 8591, 8613, 8649, 8651, 8669, 8679, 8681, 8691, 8705, 8723, 8739,
    8745, 8781, 8799, 8841, 8843, 8873, 8879, 8885, 8891, 8903, 8943, 8949, 8961, 8973, 8991, 9005,
    9023, 9053, 9093, 9111, 9195, 9275, 9281, 9291, 9311, 9353, 9369, 9389, 9395, 9405, 9429, 9431,
    9461, 9471, 9521, 9593, 9629, 9633, 9641, 9653, 9663, 9743, 9761, 9785, 9809, 9831, 9899, 9905,
    9941, 9951, 9969, 9981, 10013, 10023, 10031, 10035, 10059, 10061, 10065, 10083, 10125, 10163, 10185, 10193,
    10199, 10203, 10205, 10209, 10215, 10241, 10271, 10289, 10331, 10341, 10359, 10373, 10385, 10425, 10439, 10443,
    10451, 10475, 10479, 10509, 10511, 10521, 10535, 10553, 10559, 10563, 10583, 10593, 10605, 10611, 10619, 10625,
    10655, 10709, 10721, 10761, 10815, 10821, 10839, 10859, 10863, 10881, 10889, 10923, 10955, 11015, 11025, 11049,
    11061, 11081, 11085, 11105, 11123, 11171, 11181, 11183, 11201, 11243, 11265, 11285, 11309, 11315, 11369, 11451,
    11459, 11463, 11465, 11475, 11525, 11565, 11643, 11661, 11699, 11705, 11711, 11733, 11753, 11763, 11811, 11813,
    11901, 11913, 11921, 11951, 11963, 11979, 11991, 12033, 12041, 12051, 12113, 12123, 12125, 12135, 12149, 12155,
    12159, 12189, 12191, 12203, 12251, 12323, 12341, 12369, 12383, 12399, 12411, 12441, 12449, 12471, 12489, 12519,
    12525, 12545, 12551, 12579, 12603, 12609, 12623, 12653, 12683, 12693, 12695, 12701, 12735, 12771, 12783, 12789,
    12831, 12869, 12903, 12909, 12921, 12959, 12975, 12981, 13005, 13059, 13065, 13083, 13085, 13103, 13131, 13169,
    13181, 13191, 13199, 13205, 13211, 13215, 13269, 13289, 13293, 13313, 13323, 13331, 13349, 13371, 13391, 13395,
    13433, 13485, 13493, 13553, 13589, 13595, 13673, 13701, 13713, 13745, 13755, 13763, 13799, 13811, 13833, 13841,
    13845, 13859, 13875, 13911, 13923, 14013, 14015, 14043, 14049, 14051, 14069, 14085, 14109, 14121, 14123, 14241,
    14253, 14259, 14273, 14291, 14295, 14301, 14385, 14429, 14469, 14493, 14499, 14531, 14541, 14561, 14631, 14685,
    14693, 14699, 14715, 14739, 14745, 14751, 14765, 14801, 14823, 14853, 14871, 14895, 14933, 14945, 14979, 14981,
    15005, 15009, 15011, 15029, 15075, 15081, 15089, 15093, 15119, 15149, 15165, 15179, 15183, 15245, 15263, 15269,
    15305, 15309, 15315, 15351, 15369, 15389, 15411, 15423, 15443, 15471, 15491, 15513, 15569, 15593, 15605, 15633,
    15635, 15645, 15663, 15675, 15701, 15731, 15743, 15753, 15765, 15785, 15789, 15803, 15809, 15813, 15831, 15833,
    15861, 15863, 15873, 15885, 15891, 15899, 15929, 15969, 16023, 16055, 16059, 16079, 16181, 16193, 16215, 16235,
    16269, 16275, 16305, 16313, 16341, 16349, 16359, 16401, 16415, 16433, 16445, 16463, 16475, 16479, 16491, 16529,
    16535, 16593, 16599, 16611, 16649, 16653, 16691, 16695, 16703, 16713, 16845, 16851, 16859, 16911, 16913, 16943,
    16965, 16979, 17013, 17049, 17075, 17115, 17133, 17165, 17223, 17231, 17241, 17259, 17261, 17283, 17289, 17333,
    17339, 17361, 17429, 17441, 17453, 17481, 17483, 17495, 17525, 17529, 17573, 17585, 17601, 17625, 17655, 17783,
    17789, 17795, 17831, 17871, 17963, 17969, 17975, 17979, 18021, 18029, 18035, 18069, 18101, 18129, 18173, 18239,
    18311, 18321, 18323, 18333, 18339, 18365, 18383, 18405, 18413, 18425, 18459, 18573, 18689, 18699, 18713, 18743,
    18749, 18771, 18789, 18791, 18803, 18833, 18843, 18875, 18909, 18965, 18971, 18999, 19011, 19025, 19109, 19131,
    19139, 19173, 19191, 19205, 19233, 19253, 19265, 19275, 19323, 19331, 19341, 19355, 19361, 19389, 19401, 19443,
    19449, 19455, 19469, 19481, 19503, 19569, 19571, 19595, 19599, 19601, 19653, 19665, 19679, 19701, 19755, 19781,
    19785, 19791, 19803, 19811, 19841, 19845, 19859, 19895, 19925, 19979, 19991, 20003, 20015, 20075, 20079, 20103,
    20141, 20169, 20171, 20201, 20231, 20253, 20265, 20279, 20345, 20409, 20411, 20453, 20465, 20483, 20505, 20523,
    20525, 20531, 20541, 20591, 20609, 20615, 20619, 20691, 20699, 20703, 20709, 20729, 20741, 20751, 20769, 20771,
    20793, 20819, 20829, 20853, 20895, 20939, 20943, 20951, 20961, 20981, 20999, 21015, 21029, 21051, 21053, 21065,
    21083, 21095, 21111, 21113, 21155, 21221, 21263, 21273, 21329, 21339, 21389, 21483, 21501, 21573, 21633, 21641,
    21645, 21659, 21675, 21735, 21741, 21783, 21825, 21843, 21851, 21869, 21909, 21911, 21939, 21969, 21981, 21983
#endif
};

static UINT8 nextmod30[] =
 {
    1,
    6, 5, 4, 3, 2, 1,
    4, 3, 2, 1,
    2, 1,
    4, 3, 2, 1,
    2, 1,
    4, 3, 2, 1,
    6, 5, 4, 3, 2, 1,
    2
 };
 
 static UINT8 nextindex[] =
 {
    1,
    7,  7,  7,  7,  7,  7,
    11, 11, 11, 11,
    13, 13,
    17, 17, 17, 17,
    19, 19,
    23, 23, 23, 23,
    29, 29, 29, 29, 29, 29,
    1
 };

static sc_ulimb_t binary_search(sc_ulimb_t n, const UINT16 *t, size_t tlen)
{
    size_t lo = 0;
    size_t hi = tlen-1;
    while (lo < hi) {
        size_t mid = lo + (hi-lo)/2;
        if (t[mid] <= n) lo = mid+1;
        else             hi = mid;
    }
    return t[lo];
}

SINT32 prime_miller_rabin(sc_ulimb_t a, sc_ulimb_t b)
{
    sc_ulimb_t n1, y, r;
    size_t s, t, j;
    sc_mod_t mod;
    limb_mod_init(&mod, a);
    SINT32 retval = 0;

    // Ensure b > 1
    if (b <= 1) {
        return 0;
    }

    // Get n1 = a - 1
    n1 = a - 1;

    // Set 2**s * r = n1
    r = n1;

    // Count the number of least significant bits
    // which are zero
    s = limb_ctz(r);

    // Divide n - 1 by 2**s
    r >>= s;
    t = SC_LIMB_BITS - limb_clz(r);

    // Compute y = b**r mod a
    y = b;
    for (j=1; j<t; j++) {
        y = limb_sqr_mod_norm(y, mod.m, mod.m_inv, mod.norm);
        if (r & (1 << j)) {
            y = limb_mul_mod_norm(y, r, mod.m, mod.m_inv, mod.norm);
        }
    }

    // If y != 1 and y != n1 do
    if (1 != y && n1 != y) {
        j = 1;
        // While j <= s-1 and y != n1
        while ((j <= (s - 1)) && n1 != y) {
            y = limb_sqr_mod_norm(y, mod.m, mod.m_inv, mod.norm);

            // If y == 1 then composite
            if (1 == y) {
                retval = 1;
                goto finish;
            }

            j++;
        }

        // If y != n1 then composite
        if (n1 != y) {
            retval = 1;
            goto finish;
        }
    }

    // Probably prime now
    retval = 2;
finish:
    return retval;
}

SINT32 is_prime(sc_ulimb_t n)
{
    size_t i, t = NUM_SMALL_PRIMES;

    for (i=0; i<t; i++) {
        if (0 != prime_miller_rabin(n, small_primes[i])) {
            return SC_FUNC_SUCCESS;
        }
    }

    return SC_FUNC_FAILURE;
}

sc_ulimb_t next_prime(sc_ulimb_t a)
{
    // Perform a binary search for small prime numbers
    if (a < small_primes[NUM_SMALL_PRIMES-1]) {
        return binary_search(a, small_primes, NUM_SMALL_PRIMES);
    }

    // If the next prime number lies in the range of 1 << (SC_MAX_BITS-1) to
    // the (1 << (SC_MAX_BITS-1)) + modular_primes[63] get it by fast table lookup
    if (a >= SC_MOD_MSB && a < SC_MOD_MSB + modular_primes[NUM_MOD_PRIMES-1])
    {
        for (size_t i=0; i<NUM_MOD_PRIMES; i++) {
            if (SC_MOD_MSB + modular_primes[i] > a) {
                return SC_MOD_MSB + modular_primes[i];
            }
        }
    }

    if (SC_FUNC_SUCCESS == is_prime(a)) {
        return a;
    }

    size_t index = a % 30;
    do {
        a += nextmod30[index];
        index = nextindex[index];
    } while (SC_FUNC_FAILURE == is_prime(a));

    return a;
}
