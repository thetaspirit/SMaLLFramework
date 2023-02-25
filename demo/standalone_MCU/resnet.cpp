#include <math.h>
#include <assert.h>
// #include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
// #include <climits>
// #include <vector>
// #include <string>
// #include <fstream>
// #include <algorithm> // std::min_element
// #include <iterator>
// #include <array>
// #include <iostream>
// // #include <functional>
// #include <numeric>

typedef uint8_t dtype;

#include "include/small.h"
#include "include/utils.h"

/// @todo Which of these defines are needed?
#ifndef RUNS
#define RUNS 1
#endif
#ifndef PARALLEL
#define PARALLEL 0
#endif

#define PREFETCH 1

#define H_TILE 0
#define POOLING 1


#define LIMIT 1e-2

#define CONV 0
#define PARTIAL_CONV 1 // under development
#define DW_CONV 2      // under development
#define GROUP_CONV 3   // under development
#define POOL 4
#define RELU 5

#ifndef LAYER
#define LAYER DW_CONV
#endif

//****************************************************************************
// The output of the block is stored in I
// The weights must have been copied into F_1x1 and F_dw beforehand
template <bool scale_channels>
inline void resnet_block(
    uint32_t in_dims[2], uint32_t input_channels, // Input dimensions
    uint32_t kernel_size,
    uint32_t stride,          // DWise Covolution parameters
    uint32_t output_channels, // 1x1 Convolution parameters
    uint8_t t_pad_0,
    uint8_t b_pad_0,
    uint8_t l_pad_0,
    uint8_t r_pad_0,
    uint8_t t_pad_1,
    uint8_t b_pad_1,
    uint8_t l_pad_1,
    uint8_t r_pad_1,
    dtype *I,
    dtype *F_conv0,
    dtype *F_conv1,
    dtype *F_conv_1x1,
    dtype *O_intermediate,
    dtype *O)
{

    // printf("before: %d, %.2f %.2f %.2f %.2f\n", 0, I[0], I[1], I[2], I[3]);

    Conv2D(2, kernel_size, stride, t_pad_0, b_pad_0, l_pad_0, r_pad_0, output_channels, input_channels, in_dims[0], in_dims[1], I, F_conv0, O_intermediate);
    //uint32_t o_h, o_w;
    //op_dim(in_dims[0] + t_pad_0 + b_pad_0, stride, kernel_size, o_h);
    //op_dim(in_dims[1] + l_pad_0 + r_pad_0, stride, kernel_size, o_w);
    uint32_t o_h = output_dim(in_dims[0] + t_pad_0 + b_pad_0, stride, kernel_size);
    uint32_t o_w = output_dim(in_dims[1] + l_pad_0 + r_pad_0, stride, kernel_size);

    ReLUActivation(1, input_channels, o_h, o_w, O_intermediate, O_intermediate);
    if (scale_channels)
    {
        Conv2D(2, 1, stride, 0, 0, 0, 0, output_channels, input_channels, in_dims[0], in_dims[1], I, F_conv_1x1, O);
    }

    PartialConv2D(0, kernel_size, 1, t_pad_1, b_pad_1, l_pad_1, r_pad_1, output_channels, output_channels, o_h, o_w, O_intermediate, F_conv1, O);
    ReLUActivation(1, output_channels, o_h, o_w, O, O);
}

#define REDUCTION_C(layer_num) layer_params[layer_num][0]
#define GROUP_C(layer_num) layer_params[layer_num][1]
#define GROUPS(layer_num) layer_params[layer_num][2]
#define REDUCTION_HW(layer_num) layer_params[layer_num][3]
#define STRIDE(layer_num) layer_params[layer_num][4]
#define TYPE(layer_num) layer_params[layer_num][9]

#define SET_PADDING(layer_num, t_pad, b_pad, l_pad, r_pad) layer_params[layer_num][5] = t_pad, layer_params[layer_num][6] = b_pad, layer_params[layer_num][7] = l_pad, layer_params[layer_num][8] = r_pad;
#define PADDING(layer_num) layer_params[layer_num][5], layer_params[layer_num][6], layer_params[layer_num][7], layer_params[layer_num][8]

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

dtype *model_inference(uint32_t layer_num_total, uint16_t layer_params[30][10], uint32_t intermediate_dims[30][2], dtype *filter_ptrs[30], dtype *input_dc, dtype *inter_0_dc, dtype *inter_1_dc, dtype * inter_2_dc)
{
    auto layer_num = 0;
    // printf("starting conv:\n\t");
    Conv2D(0, REDUCTION_HW(layer_num), STRIDE(layer_num), PADDING(layer_num), GROUP_C(layer_num), REDUCTION_C(layer_num), I_HEIGHT(layer_num), I_WIDTH(layer_num), input_dc, filter_ptrs[layer_num], inter_0_dc);
    layer_num++;
    ReLUActivation(1, GROUP_C(0), I_HEIGHT(layer_num), I_WIDTH(layer_num), inter_0_dc, inter_0_dc);
    // printf("\n");
    // std::cout << "H: " << I_HEIGHT(layer_num) << " W: " << I_WIDTH(layer_num) << " C:" << GROUP_C(0) << std::endl;

    // printf("starting resnet block 0:\n\t");
    resnet_block<0>(intermediate_dims[layer_num], REDUCTION_C(layer_num), // Input dimensions
                    REDUCTION_HW(layer_num),
                    STRIDE(layer_num), // Params for the first convolution
                    GROUP_C(layer_num),
                    PADDING(layer_num),
                    PADDING(layer_num + 1),
                    inter_0_dc,
                    filter_ptrs[layer_num],
                    filter_ptrs[layer_num + 1],
                    NULL,
                    inter_1_dc,
                    inter_0_dc);
    // printf("\n");

    layer_num += 2;
    auto resnet_blocks = 3;
    auto num_filters = layer_num_total - 1;
    for (int ds_layer = 1; ds_layer < resnet_blocks; ds_layer++)
    {
        // printf("starting resnet block %d:\n\t", ds_layer);

        dtype *O_intermediate = inter_2_dc;
        resnet_block<1>(intermediate_dims[layer_num], REDUCTION_C(layer_num), // Input dimensions
                        REDUCTION_HW(layer_num),
                        STRIDE(layer_num), // Params for the first convolution
                        GROUP_C(layer_num),
                        PADDING(layer_num), PADDING(layer_num + 1),
                        inter_0_dc,
                        filter_ptrs[layer_num],
                        filter_ptrs[layer_num + 1],
                        filter_ptrs[layer_num + 2],
                        inter_1_dc,
                        O_intermediate);
        layer_num += 3;

        // Since channels were scaled, switch the pointers between inter_2 and inter_0
        inter_2_dc = inter_0_dc;
        inter_0_dc = O_intermediate;
    }

    // printf("calling pool %d  \n", layer_num);
    Maxpool2D(0, REDUCTION_HW(layer_num), STRIDE(layer_num), PADDING(layer_num), GROUPS(layer_num), I_HEIGHT(layer_num), I_WIDTH(layer_num), inter_0_dc, inter_1_dc);
    // // Dense(1, num_classes, GROUP_C(layer_num - 1), inter_1_dc, filter_fc_dc, output_dc);
    Conv2D(0, 1, 1, 0, 0, 0, 0, GROUP_C(layer_num_total - 1), REDUCTION_C(layer_num_total - 1), 1, 1, inter_1_dc, filter_ptrs[num_filters - 1], inter_0_dc);

    return inter_0_dc;
}

//****************************************************************************
//****************************************************************************
int main(int argc, char **argv)
{
    int C_i = 3;

    uint32_t N = 32;
    uint32_t M = 32;

    int num_classes = 16;



    // uint32_t check_blocks = atol(argv[5]);
    if (num_classes % 16 != 0)
    {
        printf("Number of output classes must be a multiple of 16\n");
        exit(-1);
    }

    print_build_info_check();

    uint32_t input_dimensions = C_i * N * M;
    dtype *input_dc = alloc<dtype>(input_dimensions);
    init(input_dc, input_dimensions);
 
    // calculate total number of weight elements
    //uint32_t total_num_weights = 0;
    uint16_t layer_params[30][10] = {1};
    uint32_t intermediate_dims[30][2];

    uint8_t t_pad, b_pad, r_pad, l_pad;


    // Set up model parameters
    int layer_num = 0;
    uint32_t max_numel_inter_0 = 0, max_numel_inter_1 = 0;
    
    intermediate_dims[layer_num][0] = M;
    intermediate_dims[layer_num][1] = N;

    // conv
    REDUCTION_C(layer_num) = C_i; // input channels
    GROUP_C(layer_num) = 16;      // output channels
    GROUPS(layer_num) = 1;
    REDUCTION_HW(layer_num) = 3; // kernel size
    STRIDE(layer_num) = 1;       // stride
    CALC_PADDING(I_HEIGHT(layer_num), REDUCTION_HW(layer_num), STRIDE(layer_num), t_pad, b_pad);
    CALC_PADDING(I_WIDTH(layer_num), REDUCTION_HW(layer_num), STRIDE(layer_num), l_pad, r_pad);
    SET_PADDING(layer_num, t_pad, b_pad, l_pad, r_pad)
    layer_num++; // 1

    intermediate_dims[layer_num][0] = O_WIDTH(layer_num);
    intermediate_dims[layer_num][1] = O_HEIGHT(layer_num);
    auto inter_dim = INPUT_NUMEL(layer_num);
    max_numel_inter_0 = (inter_dim > max_numel_inter_0) ? inter_dim : max_numel_inter_0;

    // common set up for model architecture
    auto resnet_blocks = 3;
    int layer_strides[] = {1, 2, 2};
    // dwise 1
    for (int ds_layer = 0; ds_layer < resnet_blocks; ds_layer++)
    {
        int channel_multiplier = (ds_layer > 0) ? 2 : 1;

        uint32_t in_channels = GROUP_C(layer_num - 1); // output channels from the previous block

        REDUCTION_C(layer_num) = in_channels; // input channels
        GROUP_C(layer_num) = in_channels * channel_multiplier;
        GROUPS(layer_num) = 1;                       // output channels
        REDUCTION_HW(layer_num) = 3;                 // kernel size
        STRIDE(layer_num) = layer_strides[ds_layer]; // stride
        CALC_PADDING(I_HEIGHT(layer_num), REDUCTION_HW(layer_num), STRIDE(layer_num), t_pad, b_pad);
        CALC_PADDING(I_WIDTH(layer_num), REDUCTION_HW(layer_num), STRIDE(layer_num), l_pad, r_pad);
        SET_PADDING(layer_num, t_pad, b_pad, l_pad, r_pad);
        layer_num++; // 2,4,7
        intermediate_dims[layer_num][0] = O_WIDTH(layer_num);
        intermediate_dims[layer_num][1] = O_HEIGHT(layer_num);


        inter_dim = INPUT_NUMEL(layer_num);
        max_numel_inter_1 = (inter_dim > max_numel_inter_1) ? inter_dim : max_numel_inter_1;
        //std::cout << "conv 0 " << layer_num << "  " << I_HEIGHT(layer_num) << " " << I_WIDTH(layer_num) << " " << GROUP_C(layer_num - 1) << std::endl;

        REDUCTION_C(layer_num) = GROUP_C(layer_num - 1);
        GROUP_C(layer_num) = GROUP_C(layer_num - 1);
        GROUPS(layer_num) = 1;
        REDUCTION_HW(layer_num) = 3;
        STRIDE(layer_num) = 1;
        CALC_PADDING(I_HEIGHT(layer_num), REDUCTION_HW(layer_num), STRIDE(layer_num), t_pad, b_pad);
        CALC_PADDING(I_WIDTH(layer_num), REDUCTION_HW(layer_num), STRIDE(layer_num), l_pad, r_pad);
        SET_PADDING(layer_num, t_pad, b_pad, l_pad, r_pad);
        layer_num++; // 3,5,8
        inter_dim = INPUT_NUMEL(layer_num);
        max_numel_inter_0 = (inter_dim > max_numel_inter_0) ? inter_dim : max_numel_inter_0;
        if (channel_multiplier != 1)
        {

            intermediate_dims[layer_num][0] = O_WIDTH(layer_num-2);
            intermediate_dims[layer_num][1] = O_HEIGHT(layer_num-2);
            REDUCTION_C(layer_num) = in_channels; // input channels
            GROUP_C(layer_num) = in_channels * channel_multiplier;
            GROUPS(layer_num) = 1;       // output channels
            REDUCTION_HW(layer_num) = 1; // kernel size
            STRIDE(layer_num) = 2;       // stride
            SET_PADDING(layer_num, 0, 0, 0, 0);
            layer_num++; // 6,9
            inter_dim = INPUT_NUMEL(layer_num);
            max_numel_inter_0 = (inter_dim > max_numel_inter_0) ? inter_dim : max_numel_inter_0;
        }
        intermediate_dims[layer_num][0] = O_WIDTH(layer_num);
        intermediate_dims[layer_num][1] = O_HEIGHT(layer_num);
    }
    // pooling dims
    REDUCTION_C(layer_num) = 1;
    GROUP_C(layer_num) = 1;
    GROUPS(layer_num) = GROUP_C(layer_num - 1);
    REDUCTION_HW(layer_num) = I_HEIGHT(layer_num);
    STRIDE(layer_num) = 1;
    SET_PADDING(layer_num, 0, 0, 0, 0)
    layer_num++;

    intermediate_dims[layer_num][0] = O_WIDTH(layer_num);
    intermediate_dims[layer_num][1] = O_HEIGHT(layer_num);

    REDUCTION_C(layer_num) = GROUPS(layer_num - 1);
    GROUP_C(layer_num) = num_classes;
    GROUPS(layer_num) = 1;
    REDUCTION_HW(layer_num) = 1;
    STRIDE(layer_num) = 1;
    SET_PADDING(layer_num, 0, 0, 0, 0)
    layer_num++;

    // fc dims
    printf("size of intermediate buffers from configuration: %d %d\n", max_numel_inter_0, max_numel_inter_1);
    uint32_t layer_num_total = layer_num;
    auto num_filters = layer_num_total - 1;

    printf("total_layers : %d \n", layer_num_total);

    for (uint32_t i = 0; i < layer_num_total; i++)
    {
        printf("layer %d: ", i);
	printf(" input_dims: %d %d ", I_HEIGHT(i), I_WIDTH(i));
        for (auto j = 0; j < 10; j++)
        {
            printf("%d, ", layer_params[i][j]);
        }
	printf("\b\b");
	//printf("input dims: %d %d ", I_HEIGHT(i+1), I_WIDTH(i+1));
        printf("\n");
    }

    //  Copy layer weights to temporaries
    dtype *filter_fc_dc; //, *filter_conv_dc, *filter_1x1_1_dc, *filter_dw_1_dc;
    dtype *filter_ptrs[30];
    for (uint32_t l = 0; l < num_filters - 1; l++)
    {
        dtype *filter_ptr;
        uint32_t filter_dimensions = REDUCTION_HW(l) * REDUCTION_HW(l) * REDUCTION_C(l) * GROUP_C(l) * GROUPS(l);
        filter_ptr = alloc<dtype>(filter_dimensions);
        init(filter_ptr, filter_dimensions);
        filter_ptrs[l] = filter_ptr;
    }

    uint32_t filter_dimensions = GROUP_C(layer_num_total - 1) * REDUCTION_C(layer_num_total - 1);
    printf("Fc filter dims %d x %d\n", REDUCTION_C(layer_num_total - 1), GROUP_C(layer_num_total - 1));
    filter_fc_dc = alloc<dtype>(filter_dimensions);
    init(filter_fc_dc, filter_dimensions);
    filter_ptrs[num_filters - 1] = filter_fc_dc;

    // copy input
    // allocate space for intermediate outputs (use the max sizes calculated previously)
    printf("max_numel_inter 0 : %d 1: %d\n", max_numel_inter_0, max_numel_inter_1);
    dtype *inter_0_dc = alloc<dtype>(max_numel_inter_0);
    dtype *inter_1_dc = alloc<dtype>(max_numel_inter_1);
    dtype *inter_2_dc = alloc<dtype>(max_numel_inter_0/2);
    dtype *output_dc ; //= alloc<dtype>(num_classes);

    printf("total allocated elements %d\n", buf_offset);

    output_dc = model_inference(layer_num_total, layer_params, intermediate_dims, filter_ptrs, input_dc, inter_0_dc, inter_1_dc, inter_2_dc);

    // output_dc = inter_0_dc;
    for (int i = 0; i < num_classes; i++)
    {
        printf("%u \t", output_dc[i]);
    }
    printf("\n");

    printf("\n");
    free_all();
}
