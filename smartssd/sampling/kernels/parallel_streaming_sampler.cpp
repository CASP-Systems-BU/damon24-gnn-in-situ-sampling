/**
 * This sampler will sample target the nodes in the chunk, give a sample of
 * n_sample nodes for each node. The chunk format reqruied is: [n_nodes]
 * [start_node_id] [real_offset_0, real_offset_1, ..., real_offset_n_nodes]
 * [neightbor_0, neighbor_1, ..., neighbor_k] ... [neightbor_0, neighbor_1, ...,
 * neighbor_k] For example: [3] [1234] [6, 8, 11, 16] [2000, 2002] [3001, 5678,
 * 88] [66, 77, 88, 99, 10] indicate there is 3 nodes in the chunk, the start
 * node id is 1234 So nodes 1234 offset 6 and 8, so arr[6] = 2001, arr[7] = 2002
 * is the neighbors The second node is 1235, and offset 8 and 11, so arr[8] =
 * 3001, arr[9] = 5678, arr[10] = 88 is the neighbors The third node is 1236,
 * and offset 11 and 16, so arr[11] = 66, arr[12] = 77, arr[13] = 88, arr[14] =
 * 99, arr[15] = 10 is the neighbors
 */

#include <hls_stream.h>



extern "C" {

inline unsigned int lfsr_random(unsigned int *lfsr) {
  unsigned lsb = *lfsr & 1; // Get LSB (i.e., the output bit)
  *lfsr >>= 1;             // Shift register
  if (lsb)                // Apply toggle mask
    *lfsr ^= 0xB400u;
  return *lfsr;
}

/*
    Vector Addition Kernel Implementation using dataflow
    Arguments:
        in   (input)  --> Input Vector
        out  (output) --> Output Integer
        size (input)  --> Size of Vector in Integer
   */
void parallel_streaming_sampler(unsigned int *in, unsigned int *out,
                         unsigned int *target, unsigned int n_target,
                         unsigned int n_sample, unsigned int external_seed = 0xACE1u) {
#pragma HLS INTERFACE m_axi port = in offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = target offset = slave bundle = gmem
#pragma HLS INTERFACE s_axilite port = n_target
#pragma HLS INTERFACE s_axilite port = n_sample
#pragma HLS INTERFACE s_axilite port = external_seed
#pragma HLS ARRAY_PARTITION variable = in complete
#pragma HLS ARRAY_PARTITION variable = out complete
#pragma HLS ARRAY_PARTITION variable = target complete

  const int UNROLL_FACTOR = 16;

  unsigned int lfsr[UNROLL_FACTOR];
#pragma HLS ARRAY_PARTITION variable = lfsr complete

  for (unsigned int i = 0; i < UNROLL_FACTOR; i++) {
    lfsr[i] = external_seed * (i+1);
  }

  // Auto-pipeline is going to apply pipeline to this loop
  unsigned int n_nodes = in[0];
  unsigned int start_node = in[1];
  
  for (unsigned int i = 0; i < n_target / UNROLL_FACTOR; i++) {
    for (unsigned int j = 0; j < UNROLL_FACTOR; j++) {
#pragma HLS PIPELINE
#pragma HLS unroll factor=UNROLL_FACTOR skip_exit_check
      // find the current target node we are working on
      unsigned int target_node = target[i * UNROLL_FACTOR + j];
      // find the index of the current target node in the chunk
      unsigned int target_node_index = target_node - start_node;
      // real position of current target node's neighbors exist in in_array
      unsigned int offset_l = in[target_node_index + 2];
      unsigned int offset_r = in[target_node_index + 3];
      unsigned int degree = offset_r - offset_l;
      // if current target node has less than n_sample neighbors, fill the
      // output with all its neighbors
      if (degree <= n_sample) {
        for (unsigned int k = 0; k < degree; k++) {
#pragma HLS PIPELINE
          out[i * UNROLL_FACTOR * n_sample + j * n_sample + k] =
              in[offset_l + k];
        }
        for (unsigned int k = degree; k < n_sample; k++) {
#pragma HLS PIPELINE
          out[i * UNROLL_FACTOR * n_sample + j * n_sample + k] =
              static_cast<unsigned int>(-1);
        }
      }
      // if current target node has more than n_sample neighbors, randomly
      // sample n_sample neighbors
      else {
        for (unsigned int k = 0; k < n_sample; k++) {
#pragma HLS PIPELINE
          out[i * UNROLL_FACTOR * n_sample + j * n_sample + k] =
              in[offset_l + (lfsr_random(&lfsr[j]) % degree)];
        }
      }
    }
  }

  // handle the last few target nodes
  for (unsigned int i = n_target / UNROLL_FACTOR * UNROLL_FACTOR; i < n_target;
       i++) {
    // find the current target node we are working on
    unsigned int target_node = target[i];
    // find the index of the current target node in the chunk
    unsigned int target_node_index = target_node - start_node;
    // real position of current target node's neighbors exist in in_array
    unsigned int offset_l = in[target_node_index + 2];
    unsigned int offset_r = in[target_node_index + 3];
    unsigned int degree = offset_r - offset_l;
    // if current target node has less than n_sample neighbors, fill the output
    // with all its neighbors
    if (degree <= n_sample) {
      for (unsigned int k = 0; k < degree; k++) {
#pragma HLS PIPELINE
        out[i * n_sample + k] = in[offset_l + k];
      }
      for (unsigned int k = degree; k < n_sample; k++) {
#pragma HLS PIPELINE
        out[i * n_sample + k] = static_cast<unsigned int>(-1);
      }
    }
    // if current target node has more than n_sample neighbors, randomly sample
    // n_sample neighbors
    else {
      for (unsigned int k = 0; k < n_sample; k++) {
#pragma HLS PIPELINE
        out[i * n_sample + k] = in[offset_l + (lfsr_random(&lfsr[0]) % degree)];
      }
    }
  }
}
}
