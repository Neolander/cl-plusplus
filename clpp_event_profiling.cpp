// Copyright 2015 Hadrien Grasland
//
// This file is part of CLplusplus.
//
// CLplusplus is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// CLplusplus is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with CLplusplus.  If not, see <http://www.gnu.org/licenses/>.

#include <array>
#include <iostream>

#include <CL/cl.h>

#include "CLplusplus/device.hpp"
#include "CLplusplus/kernel.hpp"
#include "CLplusplus/platform.hpp"
#include "CLplusplus/program.hpp"
#include "CLplusplus/version.hpp"

#include "shared.hpp"

// This program compares two approaches to matrix transposition using event profiling
int main() {
   // === PROGRAM PARAMETERS ===

   // In this example, we will be transposing a square float matrix of a certain size
   const unsigned int matrix_side_length = 8192;
   const std::array<size_t, 2> global_work_size = {matrix_side_length, matrix_side_length};
   const size_t matrix_size = matrix_side_length * matrix_side_length * sizeof(cl_float);
   const std::array<size_t, 2> local_work_size = {16, 16};
   const size_t local_buf_size = local_work_size[0] * local_work_size[1] * sizeof(cl_float);

   // Minimal platform and device parameters are specified here
   const CLplusplus::Version target_version = CLplusplus::version_1p2;
   const cl_ulong min_mem_alloc_size = matrix_size;
   const cl_ulong min_global_mem_size = 3 * matrix_size;
   const cl_ulong min_local_mem_size = local_buf_size;

   // === INITIALIZATION ===

   // Have the user select a suitable device, according to some criteria (see shared.hpp for more details)
   // TODO : Transfer some of this complexity to more advanced CLplusplus examples
   const auto selected_platform_and_device = Shared::select_device(
      [&](const CLplusplus::Platform & platform) -> bool {
         return (platform.version() >= target_version);                       // Platform OpenCL version is recent enough
      },
      [&](const CLplusplus::Device & device) -> bool {
         if(device.version() < target_version) return false;                  // OpenCL platforms may support older-generation devices, which we need to eliminate
         const auto queue_properties = device.queue_properties();
         const bool device_supports_ooe_execution = queue_properties & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;
         const bool device_supports_profiling = queue_properties & CL_QUEUE_PROFILING_ENABLE;
         const auto device_double_config = device.double_fp_config();
         return device.available() &&                                         // Device is available for compute purposes
                device.endian_little() &&                                     // Device is little-endian
                (device.execution_capabilities() & CL_EXEC_KERNEL) &&         // Device can execute OpenCL kernels
                device_supports_ooe_execution &&                              // Device can execute OpenCL commands out of order
                device_supports_profiling &&                                  // Device supports OpenCL command profiling
                device.compiler_available() && device.linker_available() &&   // Implementation has an OpenCL C compiler and linker for this device
                (device.max_mem_alloc_size() >= min_mem_alloc_size) &&        // Device accepts large enough global memory allocations
                (device.global_mem_size() >= min_global_mem_size) &&          // Device has enough global memory
                (device.local_mem_type() == CL_LOCAL) &&                      // Device has local memory support, with dedicated storage
                (device.local_mem_size() >= min_local_mem_size) &&            // Device has a large enough local memory
                (device_double_config != 0) &&                                // Doubles are supported
                ((device_double_config & CL_FP_SOFT_FLOAT) == 0);             // Doubles are not emulated in software
      }
   );

   // Create an OpenCL context on the device with some default parameters (see shared.hpp for more details)
   const auto context = Shared::build_default_context(selected_platform_and_device);

   // Allocate our input and output buffers
   std::cout << "Creating buffers..." << std::endl;
   const auto input_matrix_buffer = context.create_buffer(CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, matrix_size);
   const auto output_matrix_1_buffer = context.create_buffer(CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY, matrix_size);
   const auto output_matrix_2_buffer = context.create_buffer(CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY, matrix_size);

   // Create a program object from the matrix transpose example
   std::cout << "Loading program..." << std::endl;
   auto program = context.create_program_with_source_file("matrix_transpose.cl");

   // Start an asynchronous program build
   std::cout << "Starting to build program..." << std::endl;
   const auto build_event = program.build_with_event("-cl-mad-enable -cl-no-signed-zeros -cl-std=CL1.2 -cl-kernel-arg-info");

   // Create an out-of-order command queue for the device
   const auto command_queue = context.create_command_queue(CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE);

   // Generate our input data and send it to the device
   std::cout << "Generating and sending data..." << std::endl;
   const auto matrix_length = matrix_side_length * matrix_side_length;
   std::vector<cl_float> input_matrix(matrix_length);
   for(size_t i = 0; i < matrix_length; ++i) input_matrix[i] = i + 1;
   const auto write_event = command_queue.enqueued_write_buffer(static_cast<const void *>(&(input_matrix[0])), false, input_matrix_buffer, 0, matrix_size, {});

   // === NAIVE MATRIX TRANSPOSE ===

   // Once the program is built, create a kernel object associated to the naive matrix transpose routine
   std::cout << std::endl;
   std::cout << "Creating a kernel for naive matrix transposition..." << std::endl;
   const auto kernel_1 = program.create_kernel("float_transpose_naive", build_event);

   // Set its arguments as appropriate
   kernel_1.set_buffer_argument(0, &input_matrix_buffer);
   kernel_1.set_buffer_argument(1, &output_matrix_1_buffer);

   // Begin naive kernel execution
   std::cout << "Starting the kernel..." << std::endl;
   const auto exec_event_1 = command_queue.enqueued_2d_range_kernel(kernel_1, global_work_size, local_work_size, {write_event});

   // Once the kernel is done, synchronously read device output back into host memory
   std::cout << "Waiting for output..." << std::endl;
   std::vector<cl_float> output_matrix_1(matrix_length);
   command_queue.read_buffer(output_matrix_1_buffer, 0, static_cast<void *>(&(output_matrix_1[0])), matrix_size, {exec_event_1});

   // Tell the profiled performance of this first output
   std::cout << "The naive kernel executed in " << (exec_event_1.end_time_ns() - exec_event_1.start_time_ns()) / 1000 << " microseconds" << std::endl;

   // === LOCAL MEMORY TRANSPOSE ===

   // Create a kernel object associated to the local memory matrix transpose routine
   std::cout << std::endl;
   std::cout << "Creating a kernel for local memory matrix transposition..." << std::endl;
   const auto kernel_2 = program.create_kernel("float_transpose_local", build_event);

   // Set its arguments as appropriate
   kernel_2.set_buffer_argument(0, &input_matrix_buffer);
   kernel_2.set_local_argument(1, local_buf_size);   
   kernel_2.set_buffer_argument(2, &output_matrix_2_buffer);

   // Begin local mem kernel execution
   std::cout << "Starting the kernel..." << std::endl;
   const auto exec_event_2 = command_queue.enqueued_2d_range_kernel(kernel_2, global_work_size, local_work_size, {write_event});

   // Once the kernel is done, synchronously read device output back into host memory
   std::cout << "Waiting for output..." << std::endl;
   std::vector<cl_float> output_matrix_2(matrix_length);
   command_queue.read_buffer(output_matrix_2_buffer, 0, static_cast<void *>(&(output_matrix_2[0])), matrix_size, {exec_event_2});

   // Tell the profiled performance of this first output
   std::cout << "The local memory based kernel executed in " << (exec_event_2.end_time_ns() - exec_event_2.start_time_ns()) / 1000 << " microseconds" << std::endl;

   // === RESULT COMPARISON ===

   // Check that both routines produce the same result (we will assume the second one is right)
   std::cout << std::endl;
   for(size_t i = 0; i < matrix_length; ++i) {
      if(output_matrix_1[i] != output_matrix_2[i]) {
         std::cout << "Transpose output mismatch !" << std::endl;
         std::abort();
      }
   }
   std::cout << "Naive and optimized matrix transposition agree !" << std::endl;

   return 0;
}