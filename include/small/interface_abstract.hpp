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

#pragma once

#include <math.h>
#include <stdint.h>
#include <stdexcept>

#if defined(QUANTIZED)
#include <small/q_abstract_layer.hpp>
#else
#include <small/abstract_layer.hpp>
#endif

//#define RECORD_CALLS

#if defined(RECORD_CALLS)
#include <iostream>
#endif

namespace small
{

//****************************************************************************
template <class BufferT>
void Conv2D(int kernel_size, int stride,  /// @todo dim_t?
            uint8_t t_pad, uint8_t b_pad, uint8_t l_pad, uint8_t r_pad,
            int output_channels, int input_channels,
            int input_height, int input_width,
            BufferT const &input_buf,
            BufferT const &filter_buf,
            BufferT       &output_buf)
{
#if defined(RECORD_CALLS)
    std::cout << "Conv2D(k:" << kernel_size << ",s:" << stride
              << ",pad:[" << (int)t_pad << "," << (int)b_pad
              << "," << (int)l_pad << "," << (int)r_pad
              << "],ochans:" << output_channels
              << ",ichans:" << input_channels
              << ",img:" << input_height << "x" << input_width
              << ",I,F,O)\n";
#endif

    /// @todo We need another specific case for input_channels==1 (maybe more)

    // Specific case for the first layer
    if (input_channels == 3)
    {
        if (stride == 1)
        {
            detail::abstract_layer<BufferT,
                                   1, C_ob, 3, W_ob, 1, 1, 'c', 2, 1>(
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size, kernel_size,
                t_pad, l_pad, r_pad, b_pad,
                &input_buf, &filter_buf, &output_buf);
        }
        else if (stride == 2)
        {
            detail::abstract_layer<BufferT,
                                   1, C_ob, 3, W_ob, 2, 1, 'c', 2, 1>(
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size, kernel_size,
                t_pad, l_pad, r_pad, b_pad,
                &input_buf, &filter_buf, &output_buf);
        }
        else
        {
            // printf("This stride is unsupported, please change the interface.cpp file\n");
            throw std::invalid_argument("Conv2D ERROR: stride unsupported.");
        }
    }
    else
    {
        if (stride == 1)
        {
            detail::abstract_layer<BufferT,
                                   1, C_ob, C_ob, W_ob, 1, UNROLL, 'c', 2, 1>(
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size, kernel_size,
                t_pad, l_pad, r_pad, b_pad,
                &input_buf, &filter_buf, &output_buf);
        }
        else if (stride == 2)
        {
            detail::abstract_layer<BufferT,
                                   1, C_ob, C_ob, W_ob, 2, UNROLL, 'c', 2, 1>(
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size, kernel_size,
                t_pad, l_pad, r_pad, b_pad,
                &input_buf, &filter_buf, &output_buf);
        }
        else
        {
            // printf("This stride is unsupported, please change the interface.cpp file\n");
            throw std::invalid_argument("Conv2D ERROR: stride unsupported.");
        }
    }
}

//****************************************************************************
template <class BufferT>
void PartialConv2D(int kernel_size, int stride,
                   uint8_t t_pad, uint8_t b_pad, uint8_t l_pad, uint8_t r_pad,
                   int output_channels, int input_channels,
                   int input_height, int input_width,
                   BufferT const &input_buf,
                   BufferT const &filter_buf,
                   BufferT       &output_buf)
{
#if defined(RECORD_CALLS)
    std::cout << "PartialConv2D(k:" << kernel_size << ",s:" << stride
              << ",pad:[" << (int)t_pad << "," << (int)b_pad
              << "," << (int)l_pad << "," << (int)r_pad
              << "],ochans:" << output_channels
              << ",ichans:" << input_channels
              << ",img:" << input_height << "x" << input_width
              << ",I,F,O)\n";
#endif
    /// @todo We need another specific case for input_channels==1 (maybe more)

    // Specific case for the first layer
    if (input_channels == 3)
    {
        if (stride == 1)
        {
            detail::abstract_layer<BufferT,
                                   1, C_ob, 3, W_ob, 1, 1, 'c', 2, 0>(
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size, kernel_size,
                t_pad, l_pad, r_pad, b_pad,
                &input_buf, &filter_buf, &output_buf);
        }
        else if (stride == 2)
        {

            detail::abstract_layer<BufferT,
                                   1, C_ob, 3, W_ob, 2, 1, 'c', 2, 0>(
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size, kernel_size,
                t_pad, l_pad, r_pad, b_pad,
                &input_buf, &filter_buf, &output_buf);
        }
        else
        {
            // printf("This stride is unsupported, please change the interface.cpp file\n");
        }
    }
    else
    {
        if (stride == 1)
        {
            detail::abstract_layer<BufferT,
                                   1, C_ob, C_ob, W_ob, 1, UNROLL, 'c', 2, 0>(
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size, kernel_size,
                t_pad, l_pad, r_pad, b_pad,
                &input_buf, &filter_buf, &output_buf);
        }
        else if (stride == 2)
        {

            detail::abstract_layer<BufferT,
                                   1, C_ob, C_ob, W_ob, 2, UNROLL, 'c', 2, 0>(
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size, kernel_size,
                t_pad, l_pad, r_pad, b_pad,
                &input_buf, &filter_buf, &output_buf);
        }
        else
        {
            // printf("This stride is unsupported, please change the interface.cpp file\n");
        }
    }
}

//****************************************************************************
template <class BufferT>
void MaxPool2D(int kernel_size, int stride,
               uint8_t t_pad, uint8_t b_pad, uint8_t l_pad, uint8_t r_pad,
               int input_channels,
               int input_height, int input_width,
               BufferT const &input_buf,
               BufferT       &output_buf)
{
#if defined(RECORD_CALLS)
    std::cout << "MaxPool2D(k:" << kernel_size << ",s:" << stride
              << ",pad:[" << (int)t_pad << "," << (int)b_pad
              << "," << (int)l_pad << "," << (int)r_pad
              << "],chans:" << input_channels
              << ",img:" << input_height << "x" << input_width
              << ",I,O)\n";
#endif

    if (stride == 1)
    {
        detail::abstract_layer<BufferT,
                               C_ob, 1, 1, W_ob, 1, 1, 'p', 1, 1>(
            input_channels, // Output Channel Grouping
            1,              // Output Channels per group
            1,
            input_height, input_width,
            kernel_size, kernel_size,
            t_pad, l_pad, r_pad, b_pad,
            &input_buf, (BufferT*)nullptr, &output_buf);
    }
    else if (stride == 2)
    {

        detail::abstract_layer<BufferT,
                               C_ob, 1, 1, W_ob, 2, 1, 'p', 1, 1>(
            input_channels, // Output Channel Grouping
            1,              // Output Channels per group
            1,
            input_height, input_width,
            kernel_size, kernel_size,
            t_pad, l_pad, r_pad, b_pad,
            &input_buf, (BufferT*)nullptr, &output_buf);
    }
    else
    {
        // printf("This stride is unsupported, please change the interface.cpp file\n");
        throw std::invalid_argument("Maxpool2D ERROR: stride unsupported.");
    }
}

//****************************************************************************
template <class BufferT>
void DepthwiseConv2D(int kernel_size, int stride,
                     uint8_t t_pad, uint8_t b_pad, uint8_t l_pad, uint8_t r_pad,
                     int input_channels,
                     int input_height, int input_width,
                     BufferT const &input_buf,
                     BufferT const &filter_buf,
                     BufferT       &output_buf)
{
#if defined(RECORD_CALLS)
    std::cout << "DepthwiseConv2D(k:" << kernel_size << ",s:" << stride
              << ",pad:[" << (int)t_pad << "," << (int)b_pad
              << "," << (int)l_pad << "," << (int)r_pad
              << "],chans:" << input_channels
              << ",img:" << input_height << "x" << input_width
              << ",I,F,O)\n";
#endif
    if (stride == 1)
    {

        detail::abstract_layer<BufferT,
                               C_ob, 1, 1, W_ob, 1, 1, 'c', 1, 1>(
            input_channels, // Output Channel Grouping
            1,              // Output Channels per group
            1,
            input_height, input_width,
            kernel_size, kernel_size,
            t_pad, l_pad, r_pad, b_pad,
            &input_buf, &filter_buf, &output_buf);
    }
    else if (stride == 2)
    {

        detail::abstract_layer<BufferT,
                               C_ob, 1, 1, W_ob, 2, 1, 'c', 1, 1>(
            input_channels, // Output Channel Grouping
            1,              // Output Channels per group
            1,
            input_height, input_width,
            kernel_size, kernel_size,
            t_pad, l_pad, r_pad, b_pad,
            &input_buf, &filter_buf, &output_buf);
    }
    else
    {
        // printf("This stride is unsupported, please change the interface.cpp file\n");
        throw std::invalid_argument("DepthwiseConv2D ERROR: stride unsupported.");
    }
}

//****************************************************************************
/// @todo In q_interface_abstract, why was this the only layer with 'zero' param?
template <class BufferT>
void ReLUActivation(int input_channels,
                    int input_height, int input_width,
                    BufferT const &input_buf,
                    BufferT       &output_buf) //, int zero = 0)
{
#if defined(RECORD_CALLS)
    std::cout << "ReLUActivation(chans:" << input_channels
              << ",img:" << input_height << "x" << input_width
              << ",I,O)\n";
#endif
    detail::abstract_layer<BufferT,
                           C_ob, 1, 1, W_ob, 1, 1, 'a', 0, 1>(
        input_channels, // Output Channel Grouping
        1,              // Output Channels per group
        1,
        input_height, input_width,
        1, 1,
        0, 0, 0, 0,
        &input_buf, (BufferT*)nullptr, &output_buf); //, zero);
}

//****************************************************************************
template <class BufferT>
void LeakyReLUActivation(int input_channels,
                         int input_height, int input_width,
                         float negative_slope,
                         BufferT const &input_buf,
                         BufferT       &output_buf)
{
#if defined(RECORD_CALLS)
    std::cout << "LeakyReLUActivation(chans:" << input_channels
              << ",img:" << input_height << "x" << input_width
              << ",slope:" << negative_slope
              << ",I,O)\n";
#endif

    /// @todo Create an abstract_layer call for this layer type.
    for (auto ix = 0; ix < input_channels*input_height*input_width; ++ix)
    {
        output_buf[ix] = ((input_buf[ix] < 0) ?
                          (negative_slope*input_buf[ix]) :
                          input_buf[ix]);
    }
}

//****************************************************************************
// nearest neighbor
template <typename BufferT>
void UpSample2D(int scale_factor,
                int input_channels,
                int input_height, int input_width,
                BufferT const &input_buf,
                BufferT &output_buf)
{
    if (scale_factor == 1)
    {
        output_buf = input_buf;
    }
    else if (scale_factor == 2)
    {
        detail::abstract_layer<BufferT, C_ob, 1, 1, W_ob, 2, 1, 'u', 0, 1>(
            input_channels, // Output Channel Grouping
            1,              // Output Channels per group
            1,
            input_height, input_width,
            1, 1,
            0, 0, 0, 0,
            &input_buf, (BufferT *)NULL, &output_buf);
    }
    else
    {
        throw std::invalid_argument(
            "Upsample ERROR: scale factor unsupported (only 1 or 2).");
    }
}

//****************************************************************************
template <class BufferT>
void Accum(int input_channels,
           int input_height, int input_width,
           BufferT const &input_buf,
           BufferT       &output_buf)
{
#if defined(RECORD_CALLS)
    std::cout << "Accum(chans:" << input_channels
              << ",img:" << input_height << "x" << input_width
              << ",I,O)\n";
#endif

    /// @todo Create an abstract_layer call for this layer type.
    for (auto ix = 0; ix < input_channels*input_height*input_width; ++ix)
    {
        output_buf[ix] += input_buf[ix];
    }
}




//****************************************************************************
template <class BufferT>
void Dense(int output_elements, int input_elements,
           BufferT const &input_buf,
           BufferT const &filter_buf,
           BufferT       &output_buf)
{
#if defined(RECORD_CALLS)
    std::cout << "Dense(I,F,O)\n";
#endif
    detail::abstract_layer<BufferT,
                           C_ob, 1, 1, W_ob, 1, 1, 'c', 1, 1>(
        output_elements, // Output Channel Grouping
        1,               // Output Channels per group
        1,
        1, input_elements,
        1, 1,
        0, 0, 0, 0,
        &input_buf, &filter_buf, &output_buf);
}

//****************************************************************************
template <class BufferT>
void Conv2D_rect(int kernel_size_h, int kernel_size_w, int stride,
                 uint8_t t_pad, uint8_t b_pad, uint8_t l_pad, uint8_t r_pad,
                 int output_channels, int input_channels,
                 int input_height, int input_width,
                 BufferT const &input_buf,
                 BufferT const &filter_buf,
                 BufferT       &output_buf)
{
#if defined(RECORD_CALLS)
    std::cout << "Conv2D_rect(k:" << kernel_size_h << "x" << kernel_size_w
              << ",s:" << stride
              << ",pad:[" << (int)t_pad << "," << (int)b_pad
              << "," << (int)l_pad << "," << (int)r_pad
              << "],ochans:" << output_channels
              << ",ichans:" << input_channels
              << ",img:" << input_height << "x" << input_width
              << ",I,F,O)\n";
#endif

    // Specific case for the first layer
    if (input_channels == 3)
    {
        if (stride == 1)
        {
            detail::abstract_layer<BufferT,
                                   1, C_ob, 3, W_ob, 1, 1, 'c', 2, 1>(
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size_h, kernel_size_w,
                t_pad, l_pad, r_pad, b_pad,
                &input_buf, &filter_buf, &output_buf);
        }
        else if (stride == 2)
        {
            detail::abstract_layer<BufferT,
                                   1, C_ob, 3, W_ob, 2, 1, 'c', 2, 1>( // unroll?
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size_h, kernel_size_w,
                t_pad, l_pad, r_pad, b_pad,
                &input_buf, &filter_buf, &output_buf);
        }
        else
        {
            // printf("This stride is unsupported, please change the interface.cpp file\n");
            throw std::invalid_argument("Conv2D_rect ERROR: stride unsupported.");
        }
    }
    else
    {
        if (stride == 1)
        {
            detail::abstract_layer<BufferT,
                                   1, C_ob, C_ob, W_ob, 1, UNROLL, 'c', 2, 1>(
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size_h, kernel_size_w,
                t_pad, l_pad, r_pad, b_pad,
                &input_buf, &filter_buf, &output_buf);
        }
        else if (stride == 2)
        {

            detail::abstract_layer<BufferT,
                                   1, C_ob, C_ob, W_ob, 2, UNROLL, 'c', 2, 1>(
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size_h, kernel_size_w,
                t_pad, l_pad, r_pad, b_pad,
                &input_buf, &filter_buf, &output_buf);
        }
        else
        {
            // printf("This stride is unsupported, please change the interface.cpp file\n");
            throw std::invalid_argument("Conv2D_rect ERROR: stride unsupported.");
        }
    }
}

//****************************************************************************
template <class BufferT>
void MaxPool2D_rect(int kernel_size_h, int kernel_size_w, int stride,
                    uint8_t t_pad, uint8_t b_pad, uint8_t l_pad, uint8_t r_pad,
                    int input_channels,
                    int input_height, int input_width,
                    BufferT const &input_buf,
                    BufferT       &output_buf)
{
#if defined(RECORD_CALLS)
    std::cout << "MaxPool2D_rect(k:" << kernel_size_h << "x" << kernel_size_w
              << ",s:" << stride
              << ",pad:[" << (int)t_pad << "," << (int)b_pad
              << "," << (int)l_pad << "," << (int)r_pad
              << "],chans:" << input_channels
              << ",img:" << input_height << "x" << input_width
              << ",I,O)\n";
#endif
    if (stride == 1)
    {
        detail::abstract_layer<BufferT,
                               C_ob, 1, 1, W_ob, 1, 1, 'p', 1, 1>(
            input_channels, // Output Channel Grouping
            1,              // Output Channels per group
            1,
            input_height, input_width,
            kernel_size_h, kernel_size_w,
            t_pad, l_pad, r_pad, b_pad,
            &input_buf, (BufferT*)nullptr, &output_buf);
    }
    else if (stride == 2)
    {
        detail::abstract_layer<BufferT,
                               C_ob, 1, 1, W_ob, 2, 1, 'p', 1, 1>(
            input_channels, // Output Channel Grouping
            1,              // Output Channels per group
            1,
            input_height, input_width,
            kernel_size_h, kernel_size_w,
            t_pad, l_pad, r_pad, b_pad,
            &input_buf, (BufferT*)nullptr, &output_buf);
    }
    else
    {
        // printf("This stride is unsupported, please change the interface.cpp file\n");
        throw std::invalid_argument("MaxPool2D_rect ERROR: stride unsupported.");
    }
}

} // small
