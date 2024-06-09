/**
 * This sampler will take already sampled target and copy only the neighbors to
 * the output
 */

#include <ap_int.h>
#include <hls_stream.h>

extern "C" {
void random_read_sampler(unsigned int *in, unsigned int *out,
                         unsigned int *offsets, unsigned int n_total) {
#pragma HLS INTERFACE m_axi port = in offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = offsets offset = slave bundle = gmem
#pragma HLS INTERFACE s_axilite port = n_total
#pragma HLS ARRAY_PARTITION variable = in complete
#pragma HLS ARRAY_PARTITION variable = out complete
#pragma HLS ARRAY_PARTITION variable = offsets complete

  const int UNROLL_FACTOR = 64;
  for (unsigned int i = 0; i < n_total / UNROLL_FACTOR; i++) {
    for (unsigned int j = 0; j < UNROLL_FACTOR; j++) {
#pragma HLS PIPELINE
#pragma HLS UNROLL factor = UNROLL_FACTOR

      unsigned int pos = i * UNROLL_FACTOR + j;

      if (offsets[pos] == -1) {
        out[pos] = static_cast<unsigned int>(-1);
      } else {
        out[pos] = in[offsets[pos] + 128 * pos];
      }
    }
  }

  // handle the last few target nodes
  for (unsigned int i = n_total / UNROLL_FACTOR * UNROLL_FACTOR; i < n_total;
       i++) {
    if (offsets[i] == -1) {
      out[i] = static_cast<unsigned int>(-1);
    } else {
      out[i] = in[offsets[i] + 128 * i];
    }
  }
}
}
