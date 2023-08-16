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

namespace detail
{
    //************************************************************************
    template <class BufferT>
    void initialize_dw_conv2d_buffers(
        uint32_t          num_channels,
        uint32_t          m_kernel_size,
        BufferT    const &filters,
        BufferT    const &bias,
        BufferT    const &bn_weight,            // gamma
        BufferT    const &bn_bias,              // beta
        BufferT    const &bn_running_mean,      // mu_hat
        BufferT    const &bn_running_variance,  // sigma_hat^2
        float      const &bn_eps,               // float?
        bool              buffers_are_packed,
        BufferT          &m_packed_filters,
        BufferT          &m_packed_bias)
    {
        // ============ Filter weights ===========
        if (filters.size() !=   /// @todo consider allowing larger filter buffers??
            num_channels*m_kernel_size*m_kernel_size)
        {
            throw std::invalid_argument(
                "DepthwiseConv2DLayer::ctor ERROR: "
                "filters buffer is incorrect size.");
        }

        if (!buffers_are_packed)
        {
            // Pack the filter buffers for SMaLL use
            small::pack_buffer(filters,
                               FILTER_DW,
                               num_channels, 1U,
                               m_kernel_size, m_kernel_size,
                               BufferT::C_ib, BufferT::C_ob,
                               m_packed_filters);
        }
        else
        {
            std::copy(filters.data(),
                      filters.data() + m_packed_filters.size(),
                      m_packed_filters.data());
        }

        // ============ Bias term ===========
        if (bias.size() > 0)
        {
            if (bias.size() != num_channels)
            {
                throw std::invalid_argument(
                    "Conv2DLayer::ctor ERROR: "
                    "bias buffer incorrect size.");
            }

            // if (!buffers_are_packed)
            BufferT packed_bias(num_channels);
            std::copy(bias.data(),
                      bias.data() + num_channels,
                      packed_bias.data());
            m_packed_bias = std::move(packed_bias);
        }

        // ============ BN terms ===========
        // Note: it is all or nothing for 4 buffers
        if ((bn_weight.size() > 0) ||
            (bn_bias.size() > 0) ||
            (bn_running_mean.size() > 0) ||
            (bn_running_variance.size() > 0))
        {
            if ((bn_weight.size() != num_channels) ||
                (bn_bias.size() != num_channels) ||
                (bn_running_mean.size() != num_channels) ||
                (bn_running_variance.size() != num_channels))
            {
                throw std::invalid_argument(
                    "DepthwiseConv2DLayer::ctor ERROR: "
                    "BN buffers incorrect size.");
            }

            // Fuse the BN parameters with packed filters and bias
            /* ----------------------------------------------------------------
             * From: https://nenadmarkus.com/p/fusing-batchnorm-and-conv/
             *
             * # prepare filters
             * w_conv = conv.weight.clone().view(conv.out_channels, -1)
             * w_bn = torch.diag(bn.weight.div(torch.sqrt(bn.eps+bn.running_var)))
             *
             * fusedconv.weight.copy_(
             *     torch.mm(w_bn, w_conv).view(fusedconv.weight.size()) )
             *
             * # prepare spatial bias
             * if conv.bias is not None:
             *     b_conv = conv.bias
             * else:
             *     b_conv = torch.zeros( conv.weight.size(0) )
             *
             * b_bn = bn.bias -
             *        bn.weight.mul(bn.running_mean).div(
             *            torch.sqrt(bn.running_var + bn.eps))
             *
             * fusedconv.bias.copy_( torch.matmul(w_bn, b_conv) + b_bn )
             */
            bool no_bias = false;
            if (m_packed_bias.size() == 0)
            {
                m_packed_bias = std::move(BufferT(num_channels));
                no_bias = true;
            }

            for (size_t ochan = 0; ochan < num_channels; ++ochan)
            {
                // compute scaling factor for filters of this output channel
                float filter_scale =
                    bn_weight[ochan]/std::sqrt(bn_running_variance[ochan] + bn_eps);

                /// @todo REVISIT: this does not look like python code above
                if (no_bias)
                {
                    m_packed_bias[ochan] =
                        bn_bias[ochan] - bn_running_mean[ochan]*filter_scale;
                }
                else
                {
                    m_packed_bias[ochan] = filter_scale*m_packed_bias[ochan] +
                        bn_bias[ochan] - bn_running_mean[ochan]*filter_scale;
                }
                //std::cerr << ": packed_bias(" << ochan << ") = "
                //          << m_packed_bias[ochan]
                //          << std::endl;

                for (size_t fh = 0; fh < m_kernel_size; ++fh)
                {
                    for (size_t fw = 0; fw < m_kernel_size; ++fw)
                    {
                        size_t packed_index =
                            packed_weight_index(num_channels,  // # output chans
                                                1U,            // # input chans
                                                m_kernel_size,
                                                m_kernel_size,
                                                BufferT::C_ob,
                                                1U,            //BufferT::C_ib,
                                                ochan, 0U, fh, fw);
                        //std::cerr << "packed_index = " << packed_index << std::endl;
                        m_packed_filters[packed_index] *= filter_scale;
                    }
                }
            }
        }
    }

} // detail

//****************************************************************************
template <typename BufferT>
class DepthwiseConv2DLayer : public Layer<BufferT>
{
public:
    typedef typename BufferT::value_type value_type;

    //DepthwiseConv2DLayer () delete;

    /// @param[in] filters  Unpacked set of filters with dimensions packed
    ///                     in the following order:
    ///                     {in_chans, out_chans, kern_h, kern_w}
    ///
    DepthwiseConv2DLayer(shape_type const &input_shape,
                         uint32_t          kernel_size,
                         uint32_t          stride,
                         PaddingEnum       padding_type,
                         BufferT    const &filters,      /// @todo support move
                         bool              buffers_are_packed = true,
                         ActivationType    activation_type = NONE,
                         float             leaky_slope = 1.e-2)
        : Layer<BufferT>(),
          m_input_shape(input_shape),
          m_kernel_size(kernel_size),
          m_stride(stride),
          m_activation_type(activation_type),
          m_t_pad(0), m_b_pad(0), m_l_pad(0), m_r_pad(0),
          m_leaky_slope(1),  /// @note Allocating 1-element buffer
          m_packed_filters(input_shape[CHANNEL]*kernel_size*kernel_size)
    {
#if defined(DEBUG_LAYERS)
        std::cerr << "DWConv(batches:" << m_input_shape[BATCH]
                  << ",k:" << kernel_size
                  << ",s:" << stride
                  << ",p:" << ((padding_type == PADDING_V) ? "'v'" : "'f'")
                  << ",chans:" << m_input_shape[CHANNEL]
                  << ",img:" << m_input_shape[HEIGHT]
                  << "x" << m_input_shape[WIDTH]
                  << "), filters.size=" << filters.size() << std::endl;
#endif

        m_leaky_slope[0] = leaky_slope;
        compute_padding_output_shape(input_shape,
                                     kernel_size, stride, padding_type);

        detail::initialize_dw_conv2d_buffers(
            input_shape[CHANNEL],
            m_kernel_size,
            filters,
            BufferT(),  // empty bias
            BufferT(), BufferT(), BufferT(), BufferT(), 0.f, // no BN
            buffers_are_packed,
            m_packed_filters,
            m_packed_bias);

#if defined(DEBUG_LAYERS)
        auto &output_shape = this->output_shape();
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
                      << ",slope:" << leaky_slope
                      << ",img:" << output_shape[HEIGHT]
                      << "x" << output_shape[WIDTH]
                      << ")" << std::endl;
        }
#endif
    }

    DepthwiseConv2DLayer(shape_type const &input_shape,
                         uint32_t          kernel_size,
                         uint32_t          stride,
                         PaddingEnum       padding_type,
                         BufferT    const &filters,
                         BufferT    const &bias,
                         bool              buffers_are_packed = true,
                         ActivationType    activation_type = NONE,
                         float             leaky_slope = 1.e-2)
        : Layer<BufferT>(),
          m_input_shape(input_shape),
          m_kernel_size(kernel_size),
          m_stride(stride),
          m_activation_type(activation_type),
          m_t_pad(0), m_b_pad(0), m_l_pad(0), m_r_pad(0),
          m_leaky_slope(1),  /// @note Allocating 1-element buffer
          m_packed_filters(input_shape[CHANNEL]*kernel_size*kernel_size)
    {
#if defined(DEBUG_LAYERS)
        std::cerr << "DWConv(batches:" << m_input_shape[BATCH]
                  << ",k:" << kernel_size
                  << ",s:" << stride
                  << ",p:" << ((padding_type == PADDING_V) ? "'v'" : "'f'")
                  << ",chans:" << m_input_shape[CHANNEL]
                  << ",img:" << m_input_shape[HEIGHT]
                  << "x" << m_input_shape[WIDTH]
                  << "), filters.size=" << filters.size()
                  << ",bias.size=" << bias.size() << std::endl;
#endif

        m_leaky_slope[0] = leaky_slope;
        compute_padding_output_shape(input_shape,
                                     kernel_size, stride, padding_type);

        detail::initialize_dw_conv2d_buffers(
            input_shape[CHANNEL],
            m_kernel_size,
            filters,
            bias,
            BufferT(), BufferT(), BufferT(), BufferT(), 0.f, // no BN
            buffers_are_packed,
            m_packed_filters,
            m_packed_bias);

#if defined(DEBUG_LAYERS)
        auto &output_shape = this->output_shape();
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
                      << ",slope:" << leaky_slope
                      << ",img:" << output_shape[HEIGHT]
                      << "x" << output_shape[WIDTH]
                      << ")" << std::endl;
        }
#endif
    }

    DepthwiseConv2DLayer(
        shape_type const &input_shape,
        uint32_t          kernel_size,
        uint32_t          stride,
        PaddingEnum       padding_type,
        BufferT    const &filters,
        BufferT    const &bn_weight,            // gamma
        BufferT    const &bn_bias,              // beta
        BufferT    const &bn_running_mean,      // mu_hat
        BufferT    const &bn_running_variance,  // sigma_hat^2
        float      const &bn_eps = 1.e-5,       // float?
        bool              buffers_are_packed = true,
        ActivationType    activation_type = NONE,
        float             leaky_slope = 1.e-2)
        : Layer<BufferT>(),
          m_input_shape(input_shape),
          m_kernel_size(kernel_size),
          m_stride(stride),
          m_activation_type(activation_type),
          m_t_pad(0), m_b_pad(0), m_l_pad(0), m_r_pad(0),
          m_leaky_slope(1),  /// @note Allocating 1-element buffer
          m_packed_filters(input_shape[CHANNEL]*kernel_size*kernel_size)
    {
#if defined(DEBUG_LAYERS)
        std::cerr << "DWConv(batches:" << m_input_shape[BATCH]
                  << ",k:" << kernel_size
                  << ",s:" << stride
                  << ",p:" << ((padding_type == PADDING_V) ? "'v'" : "'f'")
                  << ",chans:" << m_input_shape[CHANNEL]
                  << ",img:" << m_input_shape[HEIGHT]
                  << "x" << m_input_shape[WIDTH]
                  << "), filters.size=" << filters.size()
                  << ",bn.sizes(weight,bias,run_var,run_avg)=("
                  << bn_weight.size()
                  << "," << bn_bias.size()
                  << "," << bn_running_variance.size()
                  << "," << bn_running_mean.size()
                  << "),bn_eps:" << bn_eps << std::endl;
#endif

        m_leaky_slope[0] = leaky_slope;
        compute_padding_output_shape(input_shape,
                                     kernel_size, stride, padding_type);

        detail::initialize_dw_conv2d_buffers(
            input_shape[CHANNEL],
            m_kernel_size,
            filters,
            BufferT(), // no bias
            bn_weight, bn_bias,
            bn_running_mean, bn_running_variance, bn_eps,
            buffers_are_packed,
            m_packed_filters,
            m_packed_bias);

#if defined(DEBUG_LAYERS)
        auto &output_shape = this->output_shape();
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
                      << ",slope:" << leaky_slope
                      << ",img:" << output_shape[HEIGHT]
                      << "x" << output_shape[WIDTH]
                      << ")" << std::endl;
        }
#endif
    }

    virtual ~DepthwiseConv2DLayer() {}

    virtual void compute_output(
        std::vector<Tensor<BufferT> const *> input,
        Tensor<BufferT>*                     output) const
    {
        if ((input.size() != 1) || (input[0]->shape() != m_input_shape))
        {
            throw std::invalid_argument(
                "DepthwiseConv2DLayer::compute_output() ERROR: "
                "incorrect input buffer shape.");
        }

        if (output->capacity() < this->output_size())
        {
            throw std::invalid_argument(
                "DepthwiseConv2DLayer::compute_output() ERROR: "
                "insufficient output buffer space.");
        }

        auto& output_shape(this->output_shape());

        DepthwiseConv2D(m_kernel_size, m_stride,
                        m_t_pad, m_b_pad, m_l_pad, m_r_pad,
                        m_input_shape[CHANNEL],
                        m_input_shape[HEIGHT], m_input_shape[WIDTH],
                        input[0]->buffer(),
                        m_packed_filters,
                        output->buffer());


        // HACK: placeholder for bias term
        if (m_packed_bias.size() == output_shape[CHANNEL])
        {
            for (size_t Co = 0; Co < output_shape[CHANNEL]; ++Co)
            {
                for (size_t h = 0; h < output_shape[HEIGHT]; ++h)
                {
                    for (size_t w = 0; w < output_shape[WIDTH]; ++w)
                    {
                        size_t idx = packed_buffer_index(output_shape[CHANNEL],
                                                         output_shape[HEIGHT],
                                                         output_shape[WIDTH],
                                                         BufferT::C_ob,
                                                         Co, h, w);
                        output->buffer()[idx] += m_packed_bias[Co];
                    }
                }
            }
        }

        output->set_shape(output_shape);

        if (m_activation_type == RELU)
        {
            small::ReLUActivation(output_shape[CHANNEL],
                                  output_shape[HEIGHT], output_shape[WIDTH],
                                  output->buffer(),
                                  output->buffer());
        }
        else if (m_activation_type == LEAKY)
        {
            small::LeakyReLUActivation(output_shape[CHANNEL],
                                       output_shape[HEIGHT], output_shape[WIDTH],
                                       output->buffer(),
                                       m_leaky_slope,
                                       output->buffer());
        }
    }

private:

    //************************************************************************
    void compute_padding_output_shape(shape_type const &input_shape,
                                      uint32_t          kernel_size,
                                      uint32_t          stride,
                                      PaddingEnum       padding_type)
    {
        shape_type output_shape;

        /// @todo is there a clean way to make these const members, or
        ///       will image size get moved to compute_output and all of
        ///       this moves to compute output?
        output_shape[BATCH] = input_shape[BATCH];
        output_shape[CHANNEL] = input_shape[CHANNEL];
        small::compute_padding_output_dim(input_shape[HEIGHT], kernel_size,
                                          stride, padding_type,
                                          m_t_pad, m_b_pad,
                                          output_shape[HEIGHT]);
        small::compute_padding_output_dim(input_shape[WIDTH], kernel_size,
                                          stride, padding_type,
                                          m_l_pad, m_r_pad,
                                          output_shape[WIDTH]);

#if defined(DEBUG_LAYERS)
        std::cerr << "DepthwiseConv2D padding: "
                  << (int)m_t_pad << "," << (int)m_b_pad
                  << "," << (int)m_l_pad << "," << (int)m_r_pad << std::endl;
#endif

        this->set_output_shape(output_shape);
    }

private:
    shape_type const m_input_shape;

    uint32_t   const m_kernel_size;
    uint32_t   const m_stride;

    ActivationType const m_activation_type;

    /// @todo: how to make const?
    uint8_t          m_t_pad, m_b_pad, m_l_pad, m_r_pad;

    BufferT          m_leaky_slope;
    BufferT          m_packed_filters;
    BufferT          m_packed_bias;
};

}
