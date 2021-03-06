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

#include "utils/sampling/sampling.h"
#ifdef HAVE_CDF_GAUSSIAN_SAMPLING
#include "utils/sampling/gaussian_cdf.h"
#endif
#ifdef HAVE_BERNOULLI_GAUSSIAN_SAMPLING
#include "utils/sampling/gaussian_bernoulli.h"
#endif
#ifdef HAVE_BAC_GAUSSIAN_SAMPLING
#include "utils/sampling/gaussian_bac.h"
#endif
#ifdef HAVE_HUFFMAN_GAUSSIAN_SAMPLING
#include "utils/sampling/gaussian_huffman.h"
#endif
#ifdef HAVE_KNUTH_YAO_GAUSSIAN_SAMPLING
#include "utils/sampling/gaussian_knuth_yao.h"
#endif
#ifdef HAVE_KNUTH_YAO_FAST_GAUSSIAN_SAMPLING
#include "utils/sampling/gaussian_knuth_yao_fast.h"
#endif
#ifdef HAVE_ZIGGURAT_GAUSSIAN_SAMPLING
#include "utils/sampling/gaussian_ziggurat.h"
#endif
#include "utils/sampling/mw_bootstrap.h"
#include "utils/arith/sc_math.h"
#include "utils/crypto/prng.h"
#include "safecrypto_private.h"


#if defined(CONSTRAINED_RAM)
#define MAX_GAUSS_LUT_BYTES  1024
#else
#define MAX_GAUSS_LUT_BYTES  16384
#endif


static SINT32 sample_vector_16(const utils_sampling_t *sampling, SINT16 *v, size_t n, SINT32 centre);
static SINT32 sample_vector_32(const utils_sampling_t *sampling, SINT32 *v, size_t n, SINT32 centre);

static utils_sampling_t utils_sampling_table = {
    gaussian_cdf_create_32, gaussian_cdf_destroy_32, gaussian_cdf_get_prng_32,
    gaussian_cdf_sample_32, sample_vector_16, sample_vector_32,
#ifdef HAVE_64BIT
    SAMPLING_64BIT,
#else
    SAMPLING_32BIT,
#endif
    256, SAMPLING_DISABLE_BOOTSTRAP, 0.0f, 0.0f, 0.0f, NULL, NULL
};

/// Return a random unbiased integer in the range 0 to x inclusive
static size_t rand_range(prng_ctx_t *ctx, size_t x)
{
    size_t y, rem;
    rem  = 0xFFFFFFFF % x;
restart:
    // Obtain a random integer in the range 0 to (2^16-1) inclusive
    y = prng_32(ctx);

    // Discard y if it is greater than or equal to the largest multiple of x
    if (y >= (0xFFFFFFFF - rem)) {
        goto restart;
    }

    // Discard y if its largest multiple is less than or equal to x
    return y % x;
}

static UINT32 set_threshold(UINT32 discard)
{
    UINT32 thresh = (SCA_PATTERN_SAMPLE_DISCARD_LO == discard)? 1 << (32 - 4) :
                    (SCA_PATTERN_SAMPLE_DISCARD_MD == discard)? 1 << (32 - 2) :
                    (SCA_PATTERN_SAMPLE_DISCARD_HI == discard)? 1 << (32 - 1) :
                                                                0;
    return thresh;
}

/// A function used to provide a flag indicating if a sample should be discarded or not
static SINT32 discard_sample(prng_ctx_t *csprng, UINT32 thresh)
{
    if (0 == thresh) {
        return 0;
    }
    else {
//fprintf(stderr, "DISCARDING \n" );
        UINT32 rnd = prng_32(csprng);
        return sc_const_time_u32_lessthan(rnd, thresh);
    }
}

static SINT32 shuffle_sample_vector_16(const utils_sampling_t *sampling, SINT16 *v, size_t n, SINT32 centre)
{
   SINT32 i, j;
   void *gauss     = sampling->gauss;
   prng_ctx_t *ctx = sampling->prng_ctx;
   UINT32 thresh   = set_threshold(sampling->discard);

    v[0] = sampling->sample(gauss);
    for (i=1; i<n; i++) {
        SINT32 cond;
        j = rand_range(ctx, i);
        cond = (-(i ^ j) >> 31) & 1;
        v[i] = cond * v[j] + (1 ^ cond) * v[i];
        v[j] = sampling->sample(gauss) + centre;
        i   -= discard_sample(ctx, thresh);
    }

    return SC_FUNC_SUCCESS;
}

static SINT32 shuffle_sample_vector_32(const utils_sampling_t *sampling, SINT32 *v, size_t n, SINT32 centre)
{
    size_t i, j;
    void *gauss     = sampling->gauss;
    prng_ctx_t *ctx = sampling->prng_ctx;
    UINT32 thresh   = set_threshold(sampling->discard);

    v[0] = sampling->sample(gauss);
    for (i=1; i<n; i++) {
        SINT32 cond;
        j = rand_range(ctx, i);
        cond = (-(i ^ j) >> 31) & 1;
        v[i] = cond * v[j] + (1 ^ cond) * v[i];
        v[j] = sampling->sample(gauss) + centre;
        i   -= discard_sample(ctx, thresh);
    }

    return SC_FUNC_SUCCESS;
}

static SINT32 blinding_sample_vector_16(const utils_sampling_t *sampling, SINT16 *v, size_t n, SINT32 centre)
{
    size_t i, j;
    void *gauss     = sampling->gauss;
    prng_ctx_t *ctx = sampling->prng_ctx;
    UINT32 thresh   = set_threshold(sampling->discard);
    UINT32 mask     = n - 1;

    shuffle_sample_vector_16(sampling, v, n, centre);
    for (i=0; i<n; i++) {
        v[i] -= sampling->sample(gauss);
    }
    for (i=0; i<n; i++) {
        SINT16 temp;
        j = i & mask;
        temp = v[i];
        v[i] = v[j];
        v[j] = temp;
    }

    return SC_FUNC_SUCCESS;
}

static SINT32 blinding_sample_vector_32(const utils_sampling_t *sampling, SINT32 *v, size_t n, SINT32 centre)
{
    size_t i, j;
    void *gauss     = sampling->gauss;
    prng_ctx_t *ctx = sampling->prng_ctx;
    UINT32 thresh   = set_threshold(sampling->discard);
    UINT32 mask     = n - 1;

    shuffle_sample_vector_32(sampling, v, n, centre);
    for (i=0; i<n; i++) {
        v[i] -= sampling->sample(gauss);
    }
    for (i=0; i<n; i++) {
        SINT32 temp;
        j = i & mask;
        temp = v[i];
        v[i] = v[j];
        v[j] = temp;
    }

    return SC_FUNC_SUCCESS;
}

static SINT32 sample_vector_16(const utils_sampling_t *sampling, SINT16 *v, size_t n, SINT32 centre)
{
    size_t i;
    void *gauss     = sampling->gauss;
    prng_ctx_t *ctx = sampling->prng_ctx;
    UINT32 thresh   = set_threshold(sampling->discard);

    for (i=0; i<n; i++) {
        v[i] = sampling->sample(gauss) + centre;
        i   -= discard_sample(ctx, thresh);
    }

    return SC_FUNC_SUCCESS;
}

static UINT32 skip = 0;
static UINT32 no_skip = 0;

static SINT32 sample_vector_32(const utils_sampling_t *sampling, SINT32 *v, size_t n, SINT32 centre)
{
    size_t i;
    void *gauss     = sampling->gauss;
    prng_ctx_t *ctx = sampling->prng_ctx;
    UINT32 thresh   = set_threshold(sampling->discard);

    for (i=0; i<n; i++) {
        UINT32 discard;
        v[i] = sampling->sample(gauss) + centre;
        discard = discard_sample(ctx, thresh);
        skip += discard;
        i   -= discard;
    }
    no_skip += n;

    return SC_FUNC_SUCCESS;
}

SINT32 configure_sampler(utils_sampling_t *sampling_table, random_sampling_e type,
    sample_precision_e precision, sample_blinding_e blinding,
    SINT32 dimension, sample_bootstrap_e bootstrapped)
{
    SINT32 success = SC_FUNC_FAILURE;

    sampling_table->bootstrapped = bootstrapped;

    switch (type) {
#ifdef HAVE_ZIGGURAT_GAUSSIAN_SAMPLING
    case ZIGGURAT_GAUSSIAN_SAMPLING:
        sampling_table->create   = ziggurat_create;
        sampling_table->destroy  = ziggurat_destroy;
        sampling_table->get_prng = ziggurat_get_prng;
        if (SAMPLING_32BIT == precision) {
            sampling_table->sample  = ziggurat_sample_32;
            success = SC_FUNC_SUCCESS;
        }
#ifdef HAVE_64BIT
        else if (SAMPLING_64BIT == precision) {
            sampling_table->sample  = ziggurat_sample_64;
            success = SC_FUNC_SUCCESS;
        }
#endif
        break;
#endif

#ifdef HAVE_BERNOULLI_GAUSSIAN_SAMPLING
    case BERNOULLI_GAUSSIAN_SAMPLING:
#ifdef HAVE_64BIT
        if (SAMPLING_64BIT == precision) {
            sampling_table->create   = bernoulli_create_64;
            sampling_table->destroy  = bernoulli_destroy_64;
            sampling_table->get_prng = bernoulli_get_prng;
            sampling_table->sample   = bernoulli_sample_64;
            success = SC_FUNC_SUCCESS;
        }
#endif
        break;
#endif

#ifdef HAVE_CDF_GAUSSIAN_SAMPLING
    case CDF_GAUSSIAN_SAMPLING:
        if (SAMPLING_32BIT == precision) {
            sampling_table->create   = gaussian_cdf_create_32;
            sampling_table->destroy  = gaussian_cdf_destroy_32;
            sampling_table->get_prng = gaussian_cdf_get_prng_32;
            sampling_table->sample   = gaussian_cdf_sample_32;
            success = SC_FUNC_SUCCESS;
        }
#ifdef HAVE_64BIT
        else if (SAMPLING_64BIT == precision) {
            sampling_table->create   = gaussian_cdf_create_64;
            sampling_table->destroy  = gaussian_cdf_destroy_64;
            sampling_table->get_prng = gaussian_cdf_get_prng_64;
            sampling_table->sample   = gaussian_cdf_sample_64;
            success = SC_FUNC_SUCCESS;
        }
#endif
#if !defined(DISABLE_HIGH_PREC_GAUSSIAN)
        else if (SAMPLING_128BIT == precision) {
            sampling_table->create   = gaussian_cdf_create_128;
            sampling_table->destroy  = gaussian_cdf_destroy_128;
            sampling_table->get_prng = gaussian_cdf_get_prng_128;
            sampling_table->sample   = gaussian_cdf_sample_128;
            success = SC_FUNC_SUCCESS;
        }
        else if (SAMPLING_192BIT == precision) {
            sampling_table->create   = gaussian_cdf_create_192;
            sampling_table->destroy  = gaussian_cdf_destroy_192;
            sampling_table->get_prng = gaussian_cdf_get_prng_128;
            sampling_table->sample   = gaussian_cdf_sample_192;
            success = SC_FUNC_SUCCESS;
        }
        else if (SAMPLING_256BIT == precision) {
            sampling_table->create   = gaussian_cdf_create_256;
            sampling_table->destroy  = gaussian_cdf_destroy_256;
            sampling_table->get_prng = gaussian_cdf_get_prng_128;
            sampling_table->sample   = gaussian_cdf_sample_256;
            success = SC_FUNC_SUCCESS;
        }
#endif
        break;
#endif

#ifdef HAVE_BAC_GAUSSIAN_SAMPLING
    case BAC_GAUSSIAN_SAMPLING:
#ifdef HAVE_64BIT
        if (SAMPLING_64BIT == precision) {
            sampling_table->create   = gaussian_bac_create_64;
            sampling_table->destroy  = gaussian_bac_destroy_64;
            sampling_table->get_prng = gaussian_bac_get_prng_64;
            sampling_table->sample   = gaussian_bac_sample_64;
            success = SC_FUNC_SUCCESS;
        }
#endif
        break;
#endif

#ifdef HAVE_HUFFMAN_GAUSSIAN_SAMPLING
    case HUFFMAN_GAUSSIAN_SAMPLING:
#ifdef HAVE_64BIT
        sampling_table->create   = gaussian_huffman_create;
        sampling_table->destroy  = gaussian_huffman_destroy;
        sampling_table->get_prng = gaussian_huffman_get_prng;
        sampling_table->sample   = gaussian_huffman_sample;
        success = SC_FUNC_SUCCESS;
#else
        success = SC_FUNC_FAILURE;
#endif
        break;
#endif

#ifdef HAVE_KNUTH_YAO_GAUSSIAN_SAMPLING
    case KNUTH_YAO_GAUSSIAN_SAMPLING:
        if (BLINDING_SAMPLES == blinding) {
            return SC_FUNC_FAILURE;
        }
        if (SAMPLING_32BIT == precision) {
            sampling_table->create   = gaussian_knuth_yao_create_32;
            sampling_table->destroy  = gaussian_knuth_yao_destroy;
            sampling_table->get_prng = gaussian_knuth_yao_get_prng;
            sampling_table->sample   = gaussian_knuth_yao_sample;
            success = SC_FUNC_SUCCESS;
        }
#ifdef HAVE_64BIT
        else if (SAMPLING_64BIT == precision) {
            sampling_table->create   = gaussian_knuth_yao_create_64;
            sampling_table->destroy  = gaussian_knuth_yao_destroy;
            sampling_table->get_prng = gaussian_knuth_yao_get_prng;
            sampling_table->sample   = gaussian_knuth_yao_sample;
            success = SC_FUNC_SUCCESS;
        }
#endif
#ifdef HAVE_128BIT
        else if (SAMPLING_128BIT == precision) {
            sampling_table->create   = gaussian_knuth_yao_create_128;
            sampling_table->destroy  = gaussian_knuth_yao_destroy;
            sampling_table->get_prng = gaussian_knuth_yao_get_prng;
            sampling_table->sample   = gaussian_knuth_yao_sample;
            success = SC_FUNC_SUCCESS;
        }
#endif
        break;
#endif

#ifdef HAVE_KNUTH_YAO_FAST_GAUSSIAN_SAMPLING
    case KNUTH_YAO_FAST_GAUSSIAN_SAMPLING:
        if (BLINDING_SAMPLES == blinding) {
            return SC_FUNC_FAILURE;
        }
        if (1024 == dimension) {
            return SC_FUNC_FAILURE;
        }
        else if (512 == dimension) {
            sampling_table->create  = gaussian_knuth_yao_fast_512_create;
        }
        else {
            sampling_table->create  = gaussian_knuth_yao_fast_256_create;
        }
        sampling_table->destroy  = gaussian_knuth_yao_fast_destroy;
        sampling_table->get_prng = gaussian_knuth_yao_fast_get_prng;
        sampling_table->sample   = gaussian_knuth_yao_fast_sample;
        success = SC_FUNC_SUCCESS;
        break;
#endif

    default:;
    }

    switch (blinding)
    {
        case SHUFFLE_SAMPLES:
        {
            sampling_table->vector_32 = shuffle_sample_vector_32;
            sampling_table->vector_16 = shuffle_sample_vector_16;
        } break;
        case BLINDING_SAMPLES:
        {
            sampling_table->vector_32 = blinding_sample_vector_32;
            sampling_table->vector_16 = blinding_sample_vector_16;
        } break;
        case NORMAL_SAMPLES:
        {
            sampling_table->vector_32 = sample_vector_32;
            sampling_table->vector_16 = sample_vector_16;
        } break;
    }

    sampling_table->precision = precision;
    sampling_table->dimension = dimension;

    return success;
}

utils_sampling_t * create_sampler(random_sampling_e type,
    sample_precision_e precision, sample_blinding_e blinding,
    SINT32 dimension, sample_bootstrap_e bootstrapped,
    prng_ctx_t *prng_ctx, FLOAT tail, FLOAT sigma)
{
    SINT32 retval;
    utils_sampling_t *sampler = SC_MALLOC(sizeof(utils_sampling_t));
    if (NULL == sampler) {
        return NULL;
    }
    retval = configure_sampler(sampler, type, precision, blinding, dimension, bootstrapped);
    if (SC_FUNC_FAILURE == retval) {
        return NULL;
    }

    // Set a pointer to the CSPRNG
    sampler->prng_ctx = prng_ctx;

    // Record the standard deviation and tail to be used
    sampler->sigma    = sigma;
    sampler->tail     = tail;

    // Default sample discarding to NONE
    sampler->discard  = 0;

    if (SAMPLING_MW_BOOTSTRAP == bootstrapped) {
        sampler->sigma2 = sigma * sigma;

        // Create the base sampler
        sampler->gauss = sampler->create(prng_ctx, tail, 16.0f, MAX_GAUSS_LUT_BYTES, blinding);

        // Use the base sampler to create the Gaussian sampler with larger standard deviation
        sampler->bootstrap = mw_bootstrap_create(sampler, sampler->gauss, 16.0f, 4, 1, 64, 35, 2.5f);
    }
    else {
        // Create the Gaussian sampler
        sampler->gauss = sampler->create(prng_ctx, tail, sigma, MAX_GAUSS_LUT_BYTES, blinding);

        // Ensure that the pointer to the bootstrapper is NULL
        sampler->bootstrap = NULL;
    }

    // Return a pointer to the sampling object
    return sampler;
}

SINT32 destroy_sampler(utils_sampling_t **sampler)
{
    SINT32 retcode;
    utils_sampling_t *local_sampler;

    if (NULL == sampler) {
        return SC_FUNC_FAILURE;
    }

    local_sampler = *sampler;

    retcode = local_sampler->destroy(&local_sampler->gauss);
    if (SC_FUNC_FAILURE == retcode) {
        return SC_FUNC_FAILURE;
    }

    if (SAMPLING_MW_BOOTSTRAP == local_sampler->bootstrapped) {
        retcode = mw_bootstrap_destroy(&local_sampler->bootstrap);
        if (SC_FUNC_FAILURE == retcode) {
            return SC_FUNC_FAILURE;
        }
    }

    SC_FREE(*sampler, sizeof(utils_sampling_t));

    return SC_FUNC_SUCCESS;
}

SINT32 set_discard(utils_sampling_t *sampler, UINT32 discard)
{
    sampler->discard = discard;
    return SC_FUNC_SUCCESS;
}

SINT32 get_sample(utils_sampling_t *sampler)
{
    return sampler->sample(sampler->gauss);
}

SINT32 get_bootstrap_sample(utils_sampling_t *sampler, FLOAT sigma, FLOAT centre)
{
    SINT32 sample;
    
    if (SAMPLING_MW_BOOTSTRAP == sampler->bootstrapped) {
        FLOAT limit = sigma * sampler->tail;
        sample = mw_bootstrap_sample(sampler->bootstrap, sigma * sigma, centre);
        if (sample < (-limit + centre)) {
            sample = (-limit + centre);
        }
        if (sample > (limit + centre)) {
            sample = (limit + centre);
        }
    }
    else {
        sample = 0;
    }

    return sample;
}

SINT32 get_vector_16(utils_sampling_t *sampler, SINT16 *v, size_t n, FLOAT centre)
{
    size_t i;

    if (SAMPLING_MW_BOOTSTRAP == sampler->bootstrapped) {
        FLOAT limit = sampler->sigma * sampler->tail;
        SINT32 limits[2] = {-limit + centre, limit + centre};
        for (i=0; i<n; i++) {
            SINT32 result = mw_bootstrap_sample(sampler->bootstrap, sampler->sigma2, centre);
            if (result < limits[0]) {
                result = limits[0];
            }
            else if (result > limits[1]) {
                result = limits[1];
            }
            v[i] = result;
        }
    }
    else {
        sampler->vector_16(sampler, v, n, (SINT32) centre);
    }

    return SC_FUNC_SUCCESS;
}

SINT32 get_vector_32(utils_sampling_t *sampler, SINT32 *v, size_t n, FLOAT centre)
{
    size_t i;

    if (SAMPLING_MW_BOOTSTRAP == sampler->bootstrapped) {
        FLOAT limit = sampler->sigma * sampler->tail;
        SINT32 limits[2] = {-limit + centre, limit + centre};
        for (i=0; i<n; i++) {
            SINT32 result;
            result = mw_bootstrap_sample(sampler->bootstrap, sampler->sigma2, centre);
            if (result < limits[0]) {
                result = limits[0];
            }
            else if (result > limits[1]) {
                result = limits[1];
            }
            v[i] = result;
        }
    }
    else {
        sampler->vector_32(sampler, v, n, (SINT32) centre);
    }

    return SC_FUNC_SUCCESS;
}
