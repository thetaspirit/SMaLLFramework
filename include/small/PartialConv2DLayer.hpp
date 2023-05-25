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
class PartialConv2DLayer : public Layer<BufferT>
{
public:
    typedef typename BufferT::value_type value_type;
    typedef typename Tensor<BufferT>::shape_type shape_type;

    //PartialConv2DLayer () delete;

    /// @param[in] filters  Unpacked set of filters with dimensions packed
    ///                     in the following order:
    ///                     {in_chans, out_chans, kern_h, kern_w}
    ///
    PartialConv2DLayer(Layer<BufferT> const &predecessor,
                       uint32_t       kernel_size,
                       uint32_t       stride,
                       PaddingEnum    padding_type,
                       uint32_t       num_output_channels,
                       BufferT const &filters,
                       bool           filters_are_packed = true)
        : Layer<BufferT>(),
          m_input_shape(predecessor.output_shape()),
          m_kernel_size(kernel_size),
          m_stride(stride),
          m_t_pad(0), m_b_pad(0), m_l_pad(0), m_r_pad(0)
    {
#if defined(DEBUG_LAYERS)
        std::cerr << "PartialConv2D(batches:" << m_input_shape[BATCH]
                  << ",k:" << kernel_size
                  << ",s:" << stride
                  << ",p:" << ((padding_type == PADDING_V) ? "'v'" : "'f'")
                  << ",ichans:" << m_input_shape[CHANNEL]
                  << ",ochans:" << num_output_channels
                  << ",img:" << m_input_shape[HEIGHT] << "x" << m_input_shape[WIDTH]
                  << "), filter.size=" << filters.size() << std::endl;
#endif

        if (filters.size() <
            num_output_channels*m_input_shape[CHANNEL]*kernel_size*kernel_size)
        {
            throw std::invalid_argument(
                "PartialConv2DLayer::ctor ERROR: "
                "filters buffer too small.");
        }

        /// @todo is there a clean way to make these const members, or
        ///       will image size get moved to compute_output and all of
        ///       this moves to compute output?
        shape_type output_shape;
        output_shape[BATCH] = m_input_shape[BATCH];
        output_shape[CHANNEL] = num_output_channels;
        small::compute_padding_output_dim(m_input_shape[HEIGHT], kernel_size,
                                          stride, padding_type,
                                          m_t_pad, m_b_pad,
                                          output_shape[HEIGHT]);
        small::compute_padding_output_dim(m_input_shape[WIDTH], kernel_size,
                                          stride, padding_type,
                                          m_l_pad, m_r_pad,
                                          output_shape[WIDTH]);
        // std::cerr << "Conv2D padding: " << (int)m_t_pad << "," << (int)m_b_pad
        //           << "," << (int)m_l_pad << "," << (int)m_r_pad << std::endl;

        Layer<BufferT>::set_output_shape(output_shape);

        BufferT packed_filters(output_shape[CHANNEL]*m_input_shape[CHANNEL]*
                               kernel_size*kernel_size);
        if (!filters_are_packed)
        {
            // Pack the filter buffers for SMaLL use
            small::pack_buffer(filters,
                               FILTER_CONV,
                               output_shape[CHANNEL], m_input_shape[CHANNEL],
                               m_kernel_size, m_kernel_size,
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

    virtual ~PartialConv2DLayer() {}

    virtual void compute_output(Tensor<BufferT> const &input,
                                Tensor<BufferT>       &output) const
    {
        if (input.shape() != m_input_shape)
        {
            throw std::invalid_argument(
                "PartialConv2DLayer::compute_output() ERROR: "
                "incorrect input buffer shape.");
        }

        if (output.capacity() < Layer<BufferT>::output_size())
        {
            throw std::invalid_argument(
                "PartialConv2DLayer::compute_output() ERROR: "
                "insufficient output buffer space.");
        }

        auto output_channels(Layer<BufferT>::output_shape()[CHANNEL]);

        PartialConv2D(m_kernel_size,
                      m_stride,
                      m_t_pad, m_b_pad, m_l_pad, m_r_pad,
                      output_channels,
                      m_input_shape[CHANNEL],
                      m_input_shape[HEIGHT], m_input_shape[WIDTH],
                      input.buffer(),
                      m_packed_filters,
                      output.buffer());
        output.set_shape(Layer<BufferT>::output_shape());
    }

private:
    shape_type const m_input_shape;

    uint32_t   const m_kernel_size;
    uint32_t   const m_stride;

    /// @todo: how to make const?
    uint8_t          m_t_pad, m_b_pad, m_l_pad, m_r_pad;

    BufferT          m_packed_filters;
};

}