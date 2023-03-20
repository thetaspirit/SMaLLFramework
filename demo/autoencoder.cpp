//****************************************************************************
// SMaLL, Software for Machine Learning Libraries
// Copyright 2023 by The SMaLL Contributors, All Rights Reserved.
// SPDX-License-Identifier: BSD-3-Clause
//
// For additional details (including references to third party source code and
// other files) see the LICENSE file or contact permission@sei.cmu.edu. See
// Contributors.txt for a full list of contributors. Created, in part, with
// funding and support from the U.S. Government (see Acknowledgments.txt file).
// DM23-0126
//****************************************************************************

#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <climits>
#include <vector>

#include <small.h>
#include "utils.h"

/// @todo Which of these defines are needed?
#ifndef RUNS
#define RUNS 10
#endif
#ifndef PARALLEL
#define PARALLEL 0
#endif

//****************************************************************************

#define REDUCTION_C(layer_num) layer_params[layer_num][0]
#define GROUP_C(layer_num) layer_params[layer_num][1]
#define GROUPS(layer_num) layer_params[layer_num][2]
#define REDUCTION_HW(layer_num) layer_params[layer_num][3]
#define STRIDE(layer_num) layer_params[layer_num][4]

// In the case of non-square kernels
#define REDUCTION_H(layer_num) layer_params[layer_num][3]
#define REDUCTION_W(layer_num) layer_params[layer_num][9]

#define SET_PADDING(layer_num, t_pad, b_pad, l_pad, r_pad) layer_params[layer_num][5] = t_pad, layer_params[layer_num][6] = b_pad, layer_params[layer_num][7] = l_pad, layer_params[layer_num][8] = r_pad;
#define PADDING(layer_num) layer_params[layer_num][5], layer_params[layer_num][6], layer_params[layer_num][7], layer_params[layer_num][8]

#define BOTTOM_PADDING(layer_num) layer_params[layer_num][6]

#define RIGHT_PADDING(layer_num) layer_params[layer_num][8]

#define PADDING_TORCH(layer_num) layer_params[layer_num][7], layer_params[layer_num][8], layer_params[layer_num][5], layer_params[layer_num][6]

#define I_WIDTH(layer_num) intermediate_dims[layer_num][0]
#define I_HEIGHT(layer_num) intermediate_dims[layer_num][1]

#define O_HEIGHT(layer_num) (((I_HEIGHT(layer_num - 1) + layer_params[layer_num - 1][5] + layer_params[layer_num - 1][6]) - REDUCTION_HW(layer_num - 1)) / STRIDE(layer_num - 1) + 1)
#define O_WIDTH(layer_num) (((I_WIDTH(layer_num - 1) + layer_params[layer_num - 1][7] + layer_params[layer_num - 1][8]) - REDUCTION_HW(layer_num - 1)) / STRIDE(layer_num - 1) + 1)

#define OUTPUT_DIMS(layer_num)                  \
    {                                           \
        O_HEIGHT(layer_num), O_WIDTH(layer_num) \
    }

#define INPUT_NUMEL(layer_num) \
    (O_HEIGHT(layer_num) * O_WIDTH(layer_num) * GROUP_C(layer_num - 1) * GROUPS(layer_num - 1))

//****************************************************************************
template <class BufferT>
BufferT &model_inference(
    uint32_t layer_num_total,
    uint16_t layer_params[30][10],
    std::vector<BufferT *> const &filter_buf_ptrs,
    BufferT  const &input_dc,
    BufferT        &inter_0_dc,
    BufferT        &inter_1_dc)
{
    auto layer_num = 0;
    small::Conv2D(1, 1,
                  0, 0, 0, 0,
                  GROUP_C(layer_num), REDUCTION_C(layer_num),
                  1, 1,
                  input_dc,
                  *filter_buf_ptrs[layer_num],
                  inter_0_dc);

    small::ReLUActivation(GROUP_C(layer_num),  // was hardcoded to 128 before
                          1, 1,
                          inter_0_dc, inter_0_dc);

    for (uint32_t cur_layer = 1; cur_layer < layer_num_total; cur_layer++)
    {
        small::Conv2D(1, 1,
                      0, 0, 0, 0,
                      GROUP_C(layer_num), REDUCTION_C(layer_num),
                      1, 1,
                      inter_0_dc,
                      *filter_buf_ptrs[layer_num],
                      inter_1_dc);

        small::ReLUActivation(GROUP_C(layer_num), // was hardcoded to 128 before
                              1, 1,
                              inter_1_dc,
                              inter_1_dc);

        layer_num++;
        inter_0_dc.swap(inter_1_dc);
    }
    return inter_0_dc;
}

//****************************************************************************
//****************************************************************************
template <class BufferT>
void inference()
{
    uint32_t C_i = 128;
    uint32_t N = 1;
    uint32_t M = 1;
    uint32_t num_classes = 16;

    // Create input tensor
    uint32_t input_dimensions = C_i * N * M;
    BufferT  input_dc(input_dimensions);
    small::init(input_dc, input_dimensions);

    // ================================================

    uint16_t layer_params[30][10] = {1};
    uint32_t intermediate_dims[30][2];

    // Set up model parameters
    auto layer_num_total = 9U;
    uint32_t layer_num = 0;
    uint32_t max_numel_inter_0 = 128, max_numel_inter_1 = 128;

    intermediate_dims[layer_num][0] = 1;
    intermediate_dims[layer_num][1] = 1;

    // conv
    REDUCTION_C(layer_num) = C_i; // input channels
    GROUP_C(layer_num) = 128;      // output channels
    GROUPS(layer_num) = 1;
    REDUCTION_HW(layer_num) = 1; // kernel size
    STRIDE(layer_num) = 1;      // stride
    SET_PADDING(layer_num, 0, 0, 0, 0)
    layer_num++;
    intermediate_dims[layer_num][0] = 1;
    intermediate_dims[layer_num][1] = 1;

    // common set up for model architecture
    for (uint32_t cur_layer = 1; cur_layer+1 < layer_num_total; cur_layer++)
    {
        REDUCTION_C(layer_num) = GROUP_C(layer_num - 1); // input channels
        GROUP_C(layer_num) = GROUP_C(layer_num - 1);
        GROUPS(layer_num) = 1;  // output channels
        REDUCTION_HW(layer_num) = 1;                 // kernel size
        STRIDE(layer_num) = 1; // stride
        SET_PADDING(layer_num, 0, 0, 0, 0)
        layer_num++; // 2

        intermediate_dims[layer_num][0] = 1;
        intermediate_dims[layer_num][1] = 1;
    }
    REDUCTION_C(layer_num) = GROUP_C(layer_num-1);
    GROUP_C(layer_num) = num_classes;
    GROUPS(layer_num) = 1;
    REDUCTION_HW(layer_num) =   1;
    STRIDE(layer_num) = 1;
    SET_PADDING(layer_num, 0, 0, 0, 0)
    layer_num++;

    intermediate_dims[layer_num][0] = O_WIDTH(layer_num);
    intermediate_dims[layer_num][1] = O_HEIGHT(layer_num);

#if SUMMARY == 1
    printf("Layer num total: %d\n", layer_num_total);
    for (uint32_t i = 0; i < layer_num_total; i++)
    {
        printf("%d: ", i);
        for (auto j = 0; j < 10; j++)
        {
            printf("%d, ", layer_params[i][j]);
        }
        printf("\b\b\n");
    }
#endif

    std::vector<BufferT *> filter_buf_ptrs;

    // Direct Convolution Setup
    for (uint32_t l = 0; l < layer_num_total; l++)
    {
        uint32_t filter_dimensions =
            REDUCTION_HW(l) * REDUCTION_HW(l) * REDUCTION_C(l) *
            GROUP_C(l) * GROUPS(l);
        BufferT *filter_buf_ptr =
            small::alloc_buffer(filter_dimensions);
        init(*filter_buf_ptr, filter_dimensions);
        filter_buf_ptrs.push_back(filter_buf_ptr);
    }

#if defined QUANTIZED
    std::cerr << "Intermediate buffer sizes: "
              << max_numel_inter_0 << ", " << max_numel_inter_1
              << std::endl;
    small::QUInt8Buffer inter_0_dc(max_numel_inter_0*4); /// @todo potential alignment issues
    small::QUInt8Buffer inter_1_dc(max_numel_inter_1*4);
#else
    small::FloatBuffer inter_0_dc(max_numel_inter_0);
    small::FloatBuffer inter_1_dc(max_numel_inter_1);
#endif

    // always returns a reference to inter_0_dc
    //auto &output_dc =
        model_inference(layer_num_total, layer_params,
                        filter_buf_ptrs,
                        input_dc,
                        inter_0_dc,
                        inter_1_dc);

    printf("\n");

    unsigned long long sum_small = ULLONG_MAX;
    std::vector<unsigned long long> small_timing;
#if defined(NANO33BLE)
    mbed::Timer t;
    t.start();
#else
    /// @todo timing variables in utils.h;
#endif

    for (int r = 0; r < RUNS; r++)
    {
#if defined(NANO33BLE)
        t.start();
#else
        // t0 = rdtsc();
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time1);
#endif

        // always returns a reference to inter_0_dc
        //auto &output_dc =
            model_inference(layer_num_total, layer_params,
                            filter_buf_ptrs,
                            input_dc,
                            inter_0_dc,
                            inter_1_dc);

#if defined(NANO33BLE)
        t.stop();
        auto diff = t.elapsed_time().count();
#else
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time2);
        auto diff = time_difference(time1, time2);
#endif
        sum_small = std::min<unsigned long long>(sum_small, diff);
        small_timing.push_back(diff);
    }

    print_cycles(sum_small);
    print_stats(small_timing, "SMaLL");

#if PARALLEL==1
    printf("OMP_NUM_THREADS: %d\n", atoi(std::getenv("OMP_NUM_THREADS")));
#endif

    for (size_t l = 0; l < filter_buf_ptrs.size(); l++)
    {
        delete filter_buf_ptrs[l];
    }

#if defined(NANO33BLE)
    small::detail::free_all();
#endif
}

//****************************************************************************
// For non-arduino platforms.  ... move to driver.cpp?
//****************************************************************************
#ifndef NANO33BLE
int main()
{
#if defined(QUANTIZED)
    inference<small::QUInt8Buffer>();
#else
    inference<small::FloatBuffer>();
#endif

    return 0;
}
#endif
