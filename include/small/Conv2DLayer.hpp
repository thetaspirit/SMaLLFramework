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

#include <small.h>
#include <small/buffers.hpp>
#include <small/Layer.hpp>

namespace small
{

//****************************************************************************
template <typename BufferT>
class Conv2DLayer : public Layer<BufferT>
{
public:
    typedef typename BufferT::value_type value_type;
    typedef typename Tensor<BufferT>::shape_type shape_type;

    //Conv2DLayer () delete;

    /// @param[in] filters  Unpacked set of filters with dimensions packed
    ///                     in the following order:
    ///                     {in_chans, out_chans, kern_h, kern_w}
    ///
    Conv2DLayer(shape_type const &input_shape,    //predecessor.output_shape()
                uint32_t          kernel_height,
                uint32_t          kernel_width,
                uint32_t          stride,
                PaddingEnum       padding_type,
                uint32_t          num_output_channels,
                BufferT    const &filters,
                bool              filters_are_packed = true,
                ActivationType    activation_type = NONE,
                float             leaky_slope = 1.e-2)
        : Layer<BufferT>(),
          m_input_shape(input_shape),
          m_kernel_height(kernel_height),
          m_kernel_width(kernel_width),
          m_stride(stride),
          m_activation_type(activation_type),
          m_leaky_slope(leaky_slope),
          m_t_pad(0), m_b_pad(0), m_l_pad(0), m_r_pad(0)
    {
#if defined(DEBUG_LAYERS)
        std::cerr << "Conv2D(batches:" << m_input_shape[BATCH]
                  << ",k:" << kernel_height << "x" << kernel_width
                  << ",s:" << stride
                  << ",p:" << ((padding_type == PADDING_V) ? "'v'" : "'f'")
                  << ",ichans:" << m_input_shape[CHANNEL]
                  << ",ochans:" << num_output_channels
                  << ",img:" << m_input_shape[HEIGHT]
                  << "x" << m_input_shape[WIDTH]
                  << "), filters.size=" << filters.size() << std::endl;
#endif

        if (filters.size() <
            num_output_channels*m_input_shape[CHANNEL]*
            kernel_height*kernel_width)
        {
            throw std::invalid_argument(
                "Conv2DLayer::ctor ERROR: "
                "filters buffer too small.");
        }

        /// @todo is there a clean way to make these const members, or
        ///       will image size get moved to compute_output and all of
        ///       this moves to compute output?
        shape_type output_shape;
        output_shape[BATCH] = m_input_shape[BATCH];
        output_shape[CHANNEL] = num_output_channels;
        small::compute_padding_output_dim(m_input_shape[HEIGHT], kernel_height,
                                          stride, padding_type,
                                          m_t_pad, m_b_pad,
                                          output_shape[HEIGHT]);
        small::compute_padding_output_dim(m_input_shape[WIDTH], kernel_width,
                                          stride, padding_type,
                                          m_l_pad, m_r_pad,
                                          output_shape[WIDTH]);

#if defined(DEBUG_LAYERS)
        std::cerr << "Conv2D padding: " << (int)m_t_pad << "," << (int)m_b_pad
                  << "," << (int)m_l_pad << "," << (int)m_r_pad << std::endl;
        if (activation_type == RELU)
        {
            std::cerr << "ReLU(batches:" << output_shape[BATCH]
                      << ",chans:" << output_shape[CHANNEL]
                      << ",img:" << output_shape[HEIGHT]
                      << "x" << output_shape[WIDTH]
                      << ")" << std::endl;
        }
        else if (activation_type == LEAKY)
        {
            std::cerr << "LeakyReLU(batches:" << output_shape[BATCH]
                      << ",chans:" << output_shape[CHANNEL]
                      << ",slope:" << m_leaky_slope
                      << ",img:" << output_shape[HEIGHT]
                      << "x" << output_shape[WIDTH]
                      << ")" << std::endl;
        }
#endif

        Layer<BufferT>::set_output_shapes({output_shape});

        BufferT packed_filters(output_shape[CHANNEL]*m_input_shape[CHANNEL]*
                               kernel_height*kernel_width);
        if (!filters_are_packed)
        {
            // Pack the filter buffers for SMaLL use
            small::pack_buffer(filters,
                               FILTER_CONV,
                               output_shape[CHANNEL], m_input_shape[CHANNEL],
                               m_kernel_height, m_kernel_width,
                               C_ib, C_ob,
                               packed_filters);
        }
        else
        {
            std::copy(filters.data(),
                      filters.data() + packed_filters.size(),
                      packed_filters.data());
        }
        m_packed_filters = std::move(packed_filters);
    }

    virtual ~Conv2DLayer() {}

    virtual void compute_output(
        std::vector<Tensor<BufferT>*> const &input,
        std::vector<Tensor<BufferT>*>       &output) const
    {
        // assert input.size()==1, output.size()==1

        if ((input.size() != 1) || (input[0]->shape() != m_input_shape))
        {
            throw std::invalid_argument(
                "Conv2DLayer::compute_output() ERROR: "
                "incorrect input buffer shape.");
        }

        if ((output.size() != 1) || (output[0]->capacity() < this->output_size(0)))
        {
            throw std::invalid_argument(
                "Conv2DLayer::compute_output() ERROR: "
                "insufficient output buffer space.");
        }

        auto& output_shape(this->output_shape(0));

        if (m_kernel_width == m_kernel_height)
        {
            Conv2D(m_kernel_width, m_stride,
                   m_t_pad, m_b_pad, m_l_pad, m_r_pad,
                   output_shape[CHANNEL],
                   m_input_shape[CHANNEL],
                   m_input_shape[HEIGHT], m_input_shape[WIDTH],
                   input[0]->buffer(),
                   m_packed_filters,
                   output[0]->buffer());
        }
        else
        {
            Conv2D_rect(m_kernel_height, m_kernel_width, m_stride,
                        m_t_pad, m_b_pad, m_l_pad, m_r_pad,
                        output_shape[CHANNEL],
                        m_input_shape[CHANNEL],
                        m_input_shape[HEIGHT], m_input_shape[WIDTH],
                        input[0]->buffer(),
                        m_packed_filters,
                        output[0]->buffer());
        }

        output[0]->set_shape(output_shape);

        if (m_activation_type == RELU)
        {
            small::ReLUActivation(output_shape[CHANNEL],
                                  output_shape[HEIGHT], output_shape[WIDTH],
                                  output[0]->buffer(),
                                  output[0]->buffer());
        }
        else if (m_activation_type == LEAKY)
        {
            small::LeakyReLUActivation(output_shape[CHANNEL],
                                       output_shape[HEIGHT], output_shape[WIDTH],
                                       m_leaky_slope,
                                       output[0]->buffer(),
                                       output[0]->buffer());
        }
    }

private:
    shape_type const m_input_shape;

    uint32_t   const m_kernel_height, m_kernel_width;
    uint32_t   const m_stride;

    ActivationType const m_activation_type;
    float          const m_leaky_slope;

    /// @todo: how to make const?
    uint8_t          m_t_pad, m_b_pad, m_l_pad, m_r_pad;

    BufferT          m_packed_filters;
};

}
