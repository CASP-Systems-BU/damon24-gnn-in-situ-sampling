

#include <ap_int.h>
#include <hls_stream.h>

// takes in a list of target nodes and fanout and
// example
// input => target_nodes, n_targetnodes, sample_result_array, fanout
//[1,3,4]                   3               2
// sample result
//[2,100],[6,7] etc

// LFSR-based pseudo-random number generator with external seed
ap_uint<32> lfsr_random(ap_uint<32> *external_seed = nullptr) {
  static ap_uint<32> lfsr = 0xACE1u; // Default initialization

  if (external_seed != nullptr) {
    lfsr = *external_seed; // Reinitialize with external seed if provided
  }

  unsigned lsb = lfsr[0]; // Get LSB (i.e., the output bit)
  lfsr >>= 1;             // Shift register
  if (lsb)                // Apply toggle mask
    lfsr ^= 0xB400u;

  return lfsr;
}

// Function to generate a random number within a range [min, max)
ap_uint<32> random_number_in_range(ap_uint<32> min, ap_uint<32> max,
                                   ap_uint<32> *external_seed = nullptr) {
  ap_uint<32> random_num = lfsr_random(external_seed);
  return (random_num % (max - min)) + min;
}

// Function to generate a random number within a range [0, max)
ap_uint<32> random_number_in_range(ap_uint<32> max,
                                   ap_uint<32> *external_seed = nullptr) {
  ap_uint<32> random_num = lfsr_random(external_seed);
  return random_num % max;
}

extern "C" {
/*
    Vector Addition Kernel Implementation using dataflow
    Arguments:
        in   (input)  --> Input Vector
        out  (output) --> Output Integer
        size (input)  --> Size of Vector in Integer
   */
void seqkernel(unsigned int *chunk, unsigned int *sample_result,
               unsigned int *target_nodes, unsigned int n_target,
               unsigned int fanout) {

// first 3 lines tells fpga
#pragma HLS INTERFACE m_axi port = chunk offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = sample_result offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = target_nodes offset = slave bundle = gmem
#pragma HLS INTERFACE s_axilite port = n_target
#pragma HLS INTERFACE s_axilite port = fanout
#pragma HLS ARRAY_PARTITION variable = chunk complete
#pragma HLS ARRAY_PARTITION variable = sample_result complete
#pragma HLS ARRAY_PARTITION variable = target_nodes complete

  // Auto-pipeline is going to apply pipeline to this loop

  // takes total and start_id
  unsigned int n_nodes = chunk[0];
  unsigned int current_node = chunk[1];
  int pos = 2;
  int target_ptr = 0;

  while (n_nodes > 0) {
    int degree = chunk[pos];
    if (current_node == target_nodes[target_ptr]) {
      if (fanout > degree) {
        // move all nodes
        for (int k = 0; k < degree; k++) {
          sample_result[(target_ptr * fanout) + k] += chunk[pos + k + 1];
        }
        for (int k = degree; k < fanout; k++) {
          sample_result[(target_ptr * fanout) + k] =
              static_cast<unsigned int>(-1);
        }
      }

      else {
        // sample
        for (int k = 0; k < fanout; k++) {
          unsigned int random_neighbor =
              random_number_in_range(pos + 1, pos + degree + 1);
          sample_result[(target_ptr * fanout) + k] = chunk[random_neighbor];
        }
      }

      if (target_ptr == n_target - 1) {
        // we have reached the last target node
        break;
      } else {
        // move the pointer ahead
        target_ptr += 1;
      }
    }

    pos = pos + degree + 1;
    n_nodes -= 1;
    current_node += 1;
  }
}
}