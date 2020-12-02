/*
 * Copyright (C) 2020 HAW Hamburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */
/**
 * @ingroup     sys_entropy_source_adc
 *
 * @{
 * @file
 *
 * @author      Peter Kietzmann <peter.kietzmann@haw-hamburg.de>
 *
 * @}
 */

#include "periph/adc.h"
#include "entropy_source.h"
#include "entropy_source/adc_noise.h"

entropy_source_tests_rep_t adc_state_rep;
entropy_source_tests_prop_t adc_state_prop;

static int _get_sample(uint8_t *out)
{
    int ret = ENTROPY_SOURCE_OK;
    uint8_t byte = 0;

    for (unsigned bit = 0; bit < 8; bit++) {
        int sample = adc_sample(CONFIG_ENTROPY_SOURCE_ADC_LINE,
                                CONFIG_ENTROPY_SOURCE_ADC_RES);
        if (sample < 0) {
            /* Resolution is not applicable */
            return ENTROPY_SOURCE_ERR_CONFIG;
        }
        /* use LSB of each ADC sample and shift it to build a value */
        byte |= (uint8_t)(sample & 0x01) << bit;
    }
    /* copy generated byte to out */
    *out = byte;

    if (IS_ACTIVE(CONFIG_ENTROPY_SOURCE_ADC_HEALTH_TEST)) {
        ret = entropy_source_test(&adc_state_rep, &adc_state_prop, byte);
    }

    return ret;
}

int entropy_source_adc_init(void)
{
    /* init ADC */
    if (adc_init(CONFIG_ENTROPY_SOURCE_ADC_LINE) != 0) {
        return ENTROPY_SOURCE_ERR_INIT;
    }

    if (IS_ACTIVE(CONFIG_ENTROPY_SOURCE_ADC_HEALTH_TEST)) {
        unsigned int cutoff;
        cutoff = entropy_source_test_rep_cutoff(CONFIG_ENTROPY_SOURCE_ADC_HMIN);
        entropy_source_test_rep_init(&adc_state_rep, cutoff);
        cutoff = entropy_source_test_prop_cutoff(CONFIG_ENTROPY_SOURCE_ADC_HMIN);
        entropy_source_test_prop_init(&adc_state_prop, cutoff);
    }

    return ENTROPY_SOURCE_OK;
}

int entropy_source_adc_get(uint8_t *out, size_t len)
{
    assert(out != NULL);

    int ret = ENTROPY_SOURCE_OK;

    if (IS_ACTIVE(CONFIG_ENTROPY_SOURCE_ADC_COND)) {
        entropy_source_sample_func_t p = &_get_sample;
        ret = entropy_source_neumann_unbias(p, out, len);
    }
    else {
        for (unsigned iter = 0; iter < len; iter++) {
            int tmp = _get_sample(&out[iter]);
            /* Remember the worst failure during
            * sampling multiple values to return */
            if (tmp < ret) {
                ret = tmp;
            }
        }
    }
    return ret;
}