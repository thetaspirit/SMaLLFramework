/*
 * SMaLL Framework
 *
 * Copyright 2022 Carnegie Mellon University and Authors.
 *
 * THIS MATERIAL WAS PREPARED AS AN ACCOUNT OF WORK SPONSORED BY AN AGENCY OF
 * THE UNITED STATES GOVERNMENT.  NEITHER THE UNITED STATES GOVERNMENT NOR THE
 * UNITED STATES DEPARTMENT OF ENERGY, NOR THE UNITED STATES DEPARTMENT OF
 * DEFENSE, NOR CARNEGIE MELLON UNIVERSITY, NOR ANY OF THEIR
 * EMPLOYEES, NOR ANY JURISDICTION OR ORGANIZATION THAT HAS COOPERATED IN THE
 * DEVELOPMENT OF THESE MATERIALS, MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR
 * ASSUMES ANY LEGAL LIABILITY OR RESPONSIBILITY FOR THE ACCURACY, COMPLETENESS,
 * OR USEFULNESS OR ANY INFORMATION, APPARATUS, PRODUCT, SOFTWARE, OR PROCESS
 * DISCLOSED, OR REPRESENTS THAT ITS USE WOULD NOT INFRINGE PRIVATELY OWNED
 * RIGHTS.
 *
 * Released under a BSD-style license, please see LICENSE file or contact
 * permission@sei.cmu.edu for full terms.
 *
 * [DISTRIBUTION STATEMENT A] This material has been approved for public release
 * and unlimited distribution.  Please see Copyright notice for non-US
 * Government use and distribution.
 *
 * DMxx-xxxx
 */

#pragma once

#include <math.h>
#include <stdint.h>
#include <stdexcept>

#include <small/abstract_layer.hpp>

namespace small
{

//****************************************************************************
template <class ScalarT>
void Conv2D(int kernel_size, int stride,  /// @todo dim_t?
            uint8_t t_pad, uint8_t b_pad, uint8_t l_pad, uint8_t r_pad,
            int output_channels, int input_channels,
            int input_height, int input_width,
            Buffer<ScalarT> const &input_buf,
            Buffer<ScalarT> const &filter_buf,
            Buffer<ScalarT>       &output_buf)
{
    /// @todo do we need another specific case for input_channels==1?

    //Specific case for the first layer
    if (input_channels == 3)
    {
        if (stride == 1)
        {
            detail::abstract_layer<typename Buffer<ScalarT>::value_type,
                                   1, C_ob, 3, W_ob, 1, 1, 'c', 2, 1>(
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size, kernel_size,
                t_pad, l_pad, r_pad, b_pad,
                input_buf, filter_buf, output_buf);
        }
        else if (stride == 2)
        {
            detail::abstract_layer<typename Buffer<ScalarT>::value_type,
                                   1, C_ob, 3, W_ob, 2, 1, 'c', 2, 1>(
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size, kernel_size,
                t_pad, l_pad, r_pad, b_pad,
                input_buf, filter_buf, output_buf);
        }
        else
        {
            //printf("This stride is unsupported, please change the interface.cpp file\n");
            throw std::invalid_argument("Conv2D ERROR: stride unsupported.");
        }
    }
    else
    {
        if (stride == 1)
        {
            detail::abstract_layer<typename Buffer<ScalarT>::value_type,
                                   1, C_ob, C_ob, W_ob, 1, UNROLL, 'c', 2, 1>(
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size, kernel_size,
                t_pad, l_pad, r_pad, b_pad,
                input_buf, filter_buf, output_buf);
        }
        else if (stride == 2)
        {
            detail::abstract_layer<typename Buffer<ScalarT>::value_type,
                                   1, C_ob, C_ob, W_ob, 2, UNROLL, 'c', 2, 1>(
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size, kernel_size,
                t_pad, l_pad, r_pad, b_pad,
                input_buf, filter_buf, output_buf);
        }
        else
        {
            //printf("This stride is unsupported, please change the interface.cpp file\n");
            throw std::invalid_argument("Conv2D ERROR: stride unsupported.");
        }
    }
}

//****************************************************************************
template <class ScalarT>
void PartialConv2D(int kernel_size, int stride,
                   uint8_t t_pad, uint8_t b_pad, uint8_t l_pad, uint8_t r_pad,
                   int output_channels, int input_channels,
                   int input_height, int input_width,
                   Buffer<ScalarT> const &input_buf,
                   Buffer<ScalarT> const &filter_buf,
                   Buffer<ScalarT>       &output_buf)
{
    // Specific case for the first layer
    if (input_channels == 3)
    {
        if (stride == 1)
        {
            detail::abstract_layer<typename Buffer<ScalarT>::value_type,
                                   1, C_ob, 3, W_ob, 1, 1, 'c', 2, 0>(
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size, kernel_size,
                t_pad, l_pad, r_pad, b_pad,
                input_buf, filter_buf, output_buf);
        }
        else if (stride == 2)
        {

            detail::abstract_layer<typename Buffer<ScalarT>::value_type,
                                   1, C_ob, 3, W_ob, 2, 1, 'c', 2, 0>(
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size, kernel_size,
                t_pad, l_pad, r_pad, b_pad,
                input_buf, filter_buf, output_buf);
        }
        else
        {
            //printf("This stride is unsupported, please change the interface.cpp file\n");
        }
    }
    else
    {
        if (stride == 1)
        {
            detail::abstract_layer<typename Buffer<ScalarT>::value_type,
                                   1, C_ob, C_ob, W_ob, 1, UNROLL, 'c', 2, 0>(
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size, kernel_size,
                t_pad, l_pad, r_pad, b_pad,
                input_buf, filter_buf, output_buf);
        }
        else if (stride == 2)
        {

            detail::abstract_layer<typename Buffer<ScalarT>::value_type,
                                   1, C_ob, C_ob, W_ob, 2, UNROLL, 'c', 2, 0>(
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size, kernel_size,
                t_pad, l_pad, r_pad, b_pad,
                input_buf, filter_buf, output_buf);
        }
        else
        {
            //printf("This stride is unsupported, please change the interface.cpp file\n");
        }
    }
}

//****************************************************************************
template <class ScalarT>
void Maxpool2D(int kernel_size, int stride,
               uint8_t t_pad, uint8_t b_pad, uint8_t l_pad, uint8_t r_pad,
               int input_channels,
               int input_height, int input_width,
               Buffer<ScalarT> const &input_buf,
               Buffer<ScalarT>       &output_buf)
{
    if (stride == 1)
    {
        detail::abstract_layer<typename Buffer<ScalarT>::value_type,
                               C_ob, 1, 1, W_ob, 1, 1, 'p', 1, 1>(
            input_channels, // Output Channel Grouping
            1,              // Output Channels per group
            1,
            input_height, input_width,
            kernel_size, kernel_size,
            t_pad, l_pad, r_pad, b_pad,
            input_buf, Buffer<ScalarT>(0), output_buf);  /// @todo HACK for no filters
    }
    else if (stride == 2)
    {

        detail::abstract_layer<typename Buffer<ScalarT>::value_type,
                               C_ob, 1, 1, W_ob, 2, 1, 'p', 1, 1>(
            input_channels, // Output Channel Grouping
            1,              // Output Channels per group
            1,
            input_height, input_width,
            kernel_size, kernel_size,
            t_pad, l_pad, r_pad, b_pad,
            input_buf, Buffer<ScalarT>(0), output_buf);  /// @todo HACK for no filters
    }
    else
    {
        //printf("This stride is unsupported, please change the interface.cpp file\n");
        throw std::invalid_argument("Maxpool2D ERROR: stride unsupported.");
    }
}

//****************************************************************************
template <class ScalarT>
void DepthwiseConv2D(int kernel_size, int stride,
                     uint8_t t_pad, uint8_t b_pad, uint8_t l_pad, uint8_t r_pad,
                     int input_channels,
                     int input_height, int input_width,
                     Buffer<ScalarT> const &input_buf,
                     Buffer<ScalarT> const &filter_buf,
                     Buffer<ScalarT>       &output_buf)
{
    if (stride == 1)
    {

        detail::abstract_layer<typename Buffer<ScalarT>::value_type,
                               C_ob, 1, 1, W_ob, 1, 1, 'c', 1, 1>(
            input_channels, // Output Channel Grouping
            1,              // Output Channels per group
            1,
            input_height, input_width,
            kernel_size, kernel_size,
            t_pad, l_pad, r_pad, b_pad,
            input_buf, filter_buf, output_buf);
    }
    else if (stride == 2)
    {

        detail::abstract_layer<typename Buffer<ScalarT>::value_type,
                               C_ob, 1, 1, W_ob, 2, 1, 'c', 1, 1>(
            input_channels, // Output Channel Grouping
            1,              // Output Channels per group
            1,
            input_height, input_width,
            kernel_size, kernel_size,
            t_pad, l_pad, r_pad, b_pad,
            input_buf, filter_buf, output_buf);
    }
    else
    {
        //printf("This stride is unsupported, please change the interface.cpp file\n");
        throw std::invalid_argument("DepthwiseConv2D ERROR: stride unsupported.");
    }
}

//****************************************************************************
template <class ScalarT>
void ReLUActivation(int input_channels,
                    int input_height, int input_width,
                    Buffer<ScalarT> const &input_buf,
                    Buffer<ScalarT>       &output_buf)
{
    detail::abstract_layer<typename Buffer<ScalarT>::value_type,
                           C_ob, 1, 1, W_ob, 1, 1, 'a', 0, 1>(
        input_channels, // Output Channel Grouping
        1,              // Output Channels per group
        1,
        input_height, input_width,
        1, 1,
        0, 0, 0, 0,
        input_buf, Buffer<ScalarT>(0), output_buf);  /// @todo HACK for no filters
}

//****************************************************************************
template <class ScalarT>
void Dense(int output_elements, int input_elements,
           Buffer<ScalarT> const &input_buf,
           Buffer<ScalarT> const &filter_buf,
           Buffer<ScalarT>       &output_buf)
{
    detail::abstract_layer<typename Buffer<ScalarT>::value_type,
                           C_ob, 1, 1, W_ob, 1, 1, 'c', 1, 1>(
        output_elements, // Output Channel Grouping
        1,              // Output Channels per group
        1,
        1, input_elements,
        1, 1,
        0, 0, 0, 0,
        input_buf, filter_buf, output_buf);
}

//****************************************************************************
template <class ScalarT>
void Conv2D_rect(int kernel_size_h, int kernel_size_w, int stride,
                 uint8_t t_pad, uint8_t b_pad, uint8_t l_pad, uint8_t r_pad,
                 int output_channels, int input_channels,
                 int input_height, int input_width,
                 Buffer<ScalarT> const &input_buf,
                 Buffer<ScalarT> const &filter_buf,
                 Buffer<ScalarT>       &output_buf)
{
    // Specific case for the first layer
    if (input_channels == 3)
    {
        if (stride == 1)
        {
            detail::abstract_layer<typename Buffer<ScalarT>::value_type,
                                   1, C_ob, 3, W_ob, 1, 1, 'c', 2, 1>(
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size_h, kernel_size_w,
                t_pad, l_pad, r_pad, b_pad,
                input_buf, filter_buf, output_buf);
        }
        else if (stride == 2)
        {
            detail::abstract_layer<typename Buffer<ScalarT>::value_type,
                                   1, C_ob, 3, W_ob, 2, 1, 'c', 2, 1>(  // unroll?
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size_h, kernel_size_w,
                t_pad, l_pad, r_pad, b_pad,
                input_buf, filter_buf, output_buf);
        }
        else
        {
            //printf("This stride is unsupported, please change the interface.cpp file\n");
            throw std::invalid_argument("Conv2D_rect ERROR: stride unsupported.");
        }
    }
    else
    {
        if (stride == 1)
        {
            detail::abstract_layer<typename Buffer<ScalarT>::value_type,
                                   1, C_ob, C_ob, W_ob, 1, UNROLL, 'c', 2, 1>(
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size_h, kernel_size_w,
                t_pad, l_pad, r_pad, b_pad,
                input_buf, filter_buf, output_buf);
        }
        else if (stride == 2)
        {

            detail::abstract_layer<typename Buffer<ScalarT>::value_type,
                                   1, C_ob, C_ob, W_ob, 2, UNROLL, 'c', 2, 1>(
                1,               // Output Channel Grouping
                output_channels, // Output Channels per group
                input_channels,
                input_height, input_width,
                kernel_size_h, kernel_size_w,
                t_pad, l_pad, r_pad, b_pad,
                input_buf, filter_buf, output_buf);
        }
        else
        {
            //printf("This stride is unsupported, please change the interface.cpp file\n");
            throw std::invalid_argument("Conv2D_rect ERROR: stride unsupported.");
        }
    }
}

//****************************************************************************
template <class ScalarT>
void MaxPool2D_rect(int kernel_size_h, int kernel_size_w, int stride,
                    uint8_t t_pad, uint8_t b_pad, uint8_t l_pad, uint8_t r_pad,
                    int input_channels,
                    int input_height, int input_width,
                    Buffer<ScalarT> const &input_buf,
                    Buffer<ScalarT>       &output_buf)
{
    if (stride == 1)
    {
        detail::abstract_layer<typename Buffer<ScalarT>::value_type,
                               C_ob, 1, 1, W_ob, 1, 1, 'p', 1, 1>(
            input_channels, // Output Channel Grouping
            1,              // Output Channels per group
            1,
            input_height, input_width,
            kernel_size_h, kernel_size_w,
            t_pad, l_pad, r_pad, b_pad,
            input_buf, Buffer<ScalarT>(0), output_buf);  /// @todo HACK for no filters
    }
    else if (stride == 2)
    {
        detail::abstract_layer<typename Buffer<ScalarT>::value_type,
                               C_ob, 1, 1, W_ob, 2, 1, 'p', 1, 1>(
            input_channels, // Output Channel Grouping
            1,              // Output Channels per group
            1,
            input_height, input_width,
            kernel_size_h, kernel_size_w,
            t_pad, l_pad, r_pad, b_pad,
            input_buf, Buffer<ScalarT>(0), output_buf);  /// @todo HACK for no filters
    }
    else
    {
        //printf("This stride is unsupported, please change the interface.cpp file\n");
        throw std::invalid_argument("MaxPool2D_rect ERROR: stride unsupported.");
    }
}

} // small
