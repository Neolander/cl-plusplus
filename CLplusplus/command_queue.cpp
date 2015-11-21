#include "command_queue.hpp"
#include "common.hpp"

namespace CLplusplus {

   CommandQueue::CommandQueue(const cl_command_queue identifier, const bool increment_reference_count) :
      internal_id{identifier}
   {
      // Handle invalid command queue IDs
      if(internal_id == NULL) throw InvalidArgument();

      // Unless asked not to do so, increment the command queue's reference count
      if(increment_reference_count) retain();
   }

   CommandQueue::CommandQueue(const CommandQueue & source) :
      internal_id{source.internal_id}
   {
      // Whenever a copy of a reference-counted command queue is made, its reference count should be incremented
      retain();
   }

   CommandQueue & CommandQueue::operator=(const CommandQueue & source) {
      // Reference count considerations also apply to copy assignment operator
      internal_id = source.internal_id;
      retain();
      return *this;
   }

   size_t CommandQueue::raw_query_output_size(cl_command_queue_info parameter_name) const {
      size_t result;
      raw_query(parameter_name, 0, nullptr, &result);
      return result;
   }

   void CommandQueue::raw_query(cl_command_queue_info parameter_name, size_t output_storage_size, void * output_storage, size_t * actual_output_size) const {
      throw_if_failed(clGetCommandQueueInfo(internal_id, parameter_name, output_storage_size, output_storage, actual_output_size));
   }

   Event CommandQueue::enqueued_read_buffer(const Buffer & source_buffer, const size_t offset, void * const destination, const size_t size, const EventWaitList & event_wait_list) const {
      cl_event event_id;
      raw_read_buffer(source_buffer, offset, destination, size, false, event_wait_list, &event_id);
      return Event{event_id, false};
   }

   void CommandQueue::enqueue_read_buffer(const Buffer & source_buffer, const size_t offset, void * const destination, const size_t size, const EventWaitList & event_wait_list) const {
      raw_read_buffer(source_buffer, offset, destination, size, false, event_wait_list, nullptr);
   }

   void CommandQueue::read_buffer(const Buffer & source_buffer, const size_t offset, void * const destination, const size_t size, const EventWaitList & event_wait_list) const {
      raw_read_buffer(source_buffer, offset, destination, size, true, event_wait_list, nullptr);
   }

   Event CommandQueue::enqueued_write_buffer(const void * const source, const bool wait_for_availability, const Buffer & dest_buffer, const size_t offset, const size_t size, const EventWaitList & event_wait_list) const {
      cl_event event_id;
      raw_write_buffer(source, wait_for_availability, dest_buffer, offset, size, event_wait_list, &event_id);
      return Event{event_id, false};
   }

   void CommandQueue::enqueue_write_buffer(const void * const source, const bool wait_for_availability, const Buffer & dest_buffer, const size_t offset, const size_t size, const EventWaitList & event_wait_list) const {
      raw_write_buffer(source, wait_for_availability, dest_buffer, offset, size, event_wait_list, nullptr);
   }

   Event CommandQueue::enqueued_copy_buffer(const Buffer & source_buffer, const size_t source_offset, const Buffer & dest_buffer, const size_t dest_offset, const size_t size, const EventWaitList & event_wait_list) const {
      cl_event event_id;
      raw_copy_buffer(source_buffer, source_offset, dest_buffer, dest_offset, size, event_wait_list, &event_id);
      return Event{event_id, false};
   }

   void CommandQueue::enqueue_copy_buffer(const Buffer & source_buffer, const size_t source_offset, const Buffer & dest_buffer, const size_t dest_offset, const size_t size, const EventWaitList & event_wait_list) const {
      raw_copy_buffer(source_buffer, source_offset, dest_buffer, dest_offset, size, event_wait_list, nullptr);
   }

   Event CommandQueue::raw_enqueued_fill_buffer(const void * const pattern, const size_t pattern_size, const Buffer & dest_buffer, const size_t offset, const size_t size, const EventWaitList & event_wait_list) const {
      cl_event event_id;
      raw_fill_buffer(pattern, pattern_size, dest_buffer, offset, size, event_wait_list, &event_id);
      return Event{event_id, false};
   }

   void CommandQueue::raw_enqueue_fill_buffer(const void * const pattern, const size_t pattern_size, const Buffer & dest_buffer, const size_t offset, const size_t size, const EventWaitList & event_wait_list) const {
      raw_fill_buffer(pattern, pattern_size, dest_buffer, offset, size, event_wait_list, nullptr);
   }

   Event CommandQueue::enqueued_map_buffer(const Buffer & buffer, const size_t offset, const size_t size, const cl_map_flags map_flags, const EventWaitList & event_wait_list, void * & future_result) const {
      cl_event event_id;
      future_result = raw_map_buffer(buffer, offset, size, false, map_flags, event_wait_list, &event_id);
      return Event{event_id, false};
   }

   void CommandQueue::enqueue_map_buffer(const Buffer & buffer, const size_t offset, const size_t size, const cl_map_flags map_flags, const EventWaitList & event_wait_list, void * & future_result) const {
      future_result = raw_map_buffer(buffer, offset, size, false, map_flags, event_wait_list, nullptr);
   }

   void * CommandQueue::map_buffer(const Buffer & buffer, const size_t offset, const size_t size, const cl_map_flags map_flags, const EventWaitList & event_wait_list) const {
      return raw_map_buffer(buffer, offset, size, true, map_flags, event_wait_list, nullptr);
   }

   Event CommandQueue::enqueued_unmap_mem_object(const MemoryObject & memobj, void * const mapped_ptr, const EventWaitList & event_wait_list) const {
      cl_event event_id;
      raw_unmap_mem_object(memobj, mapped_ptr, event_wait_list, &event_id);
      return Event{event_id, false};
   }

   void CommandQueue::enqueue_unmap_mem_object(const MemoryObject & memobj, void * const mapped_ptr, const EventWaitList & event_wait_list) const {
      raw_unmap_mem_object(memobj, mapped_ptr, event_wait_list, nullptr);
   }

   Event CommandQueue::enqueued_migrate_mem_objects(const ConstMemoryObjectRefVector & mem_objects, const cl_mem_migration_flags flags, const EventWaitList & event_wait_list) const {
      cl_event event_id;
      raw_migrate_mem_objects(mem_objects, flags, event_wait_list, &event_id);
      return Event{event_id, false};
   }

   void CommandQueue::enqueue_migrate_mem_objects(const ConstMemoryObjectRefVector & mem_objects, const cl_mem_migration_flags flags, const EventWaitList & event_wait_list) const {
      raw_migrate_mem_objects(mem_objects, flags, event_wait_list, nullptr);
   }

   void CommandQueue::flush() const {
      throw_if_failed(clFlush(internal_id));
   }

   void CommandQueue::finish() const {
      throw_if_failed(clFinish(internal_id));
   }

   void CommandQueue::raw_read_buffer(const Buffer & source_buffer, const size_t offset, void * const destination, const size_t size, const bool synchronous_read, const EventWaitList & event_wait_list, cl_event * event) const {
      const auto num_events = event_wait_list.size();
      if(num_events == 0) {
         throw_if_failed(clEnqueueReadBuffer(internal_id, source_buffer.raw_identifier(), synchronous_read, offset, size, destination, 0, nullptr, event));
      } else {
         cl_event raw_event_ids[num_events];
         for(size_t i = 0; i < num_events; ++i) raw_event_ids[i] = event_wait_list[i].raw_identifier();
         throw_if_failed(clEnqueueReadBuffer(internal_id, source_buffer.raw_identifier(), synchronous_read, offset, size, destination, num_events, raw_event_ids, event));
      }
   }

   void CommandQueue::raw_write_buffer(const void * const source, const bool wait_for_availability, const Buffer & dest_buffer, const size_t offset, const size_t size, const EventWaitList & event_wait_list, cl_event * event) const {
      const auto num_events = event_wait_list.size();
      if(num_events == 0) {
         throw_if_failed(clEnqueueWriteBuffer(internal_id, dest_buffer.raw_identifier(), wait_for_availability, offset, size, source, 0, nullptr, event));
      } else {
         cl_event raw_event_ids[num_events];
         for(size_t i = 0; i < num_events; ++i) raw_event_ids[i] = event_wait_list[i].raw_identifier();
         throw_if_failed(clEnqueueWriteBuffer(internal_id, dest_buffer.raw_identifier(), wait_for_availability, offset, size, source, num_events, raw_event_ids, event));
      }
   }

   void CommandQueue::raw_copy_buffer(const Buffer & source_buffer, const size_t source_offset, const Buffer & dest_buffer, const size_t dest_offset, const size_t size, const EventWaitList & event_wait_list, cl_event * event) const {
      const auto num_events = event_wait_list.size();
      if(num_events == 0) {
         throw_if_failed(clEnqueueCopyBuffer(internal_id, source_buffer.raw_identifier(), dest_buffer.raw_identifier(), source_offset, dest_offset, size, 0, nullptr, event));
      } else {
         cl_event raw_event_ids[num_events];
         for(size_t i = 0; i < num_events; ++i) raw_event_ids[i] = event_wait_list[i].raw_identifier();
         throw_if_failed(clEnqueueCopyBuffer(internal_id, source_buffer.raw_identifier(), dest_buffer.raw_identifier(), source_offset, dest_offset, size, num_events, raw_event_ids, event));
      }
   }

   void CommandQueue::raw_fill_buffer(const void * const pattern, const size_t pattern_size, const Buffer & dest_buffer, const size_t offset, const size_t size, const EventWaitList & event_wait_list, cl_event * event) const {
      const auto num_events = event_wait_list.size();
      if(num_events == 0) {
         throw_if_failed(clEnqueueFillBuffer(internal_id, dest_buffer.raw_identifier(), pattern, pattern_size, offset, size, 0, nullptr, event));
      } else {
         cl_event raw_event_ids[num_events];
         for(size_t i = 0; i < num_events; ++i) raw_event_ids[i] = event_wait_list[i].raw_identifier();
         throw_if_failed(clEnqueueFillBuffer(internal_id, dest_buffer.raw_identifier(), pattern, pattern_size, offset, size, num_events, raw_event_ids, event));
      }
   }

   void * CommandQueue::raw_map_buffer(const Buffer & buffer, const size_t offset, const size_t size, const bool synchronous_map, const cl_map_flags map_flags, const EventWaitList & event_wait_list, cl_event * event) const {
      // Determine if we are waiting for events, and prepare error code and result storage
      const auto num_events = event_wait_list.size();
      cl_int error_code;
      void * result;

      // Send the raw OpenCL command, with a wait list if needed
      if(num_events == 0) {
         result = clEnqueueMapBuffer(internal_id, buffer.raw_identifier(), synchronous_map, map_flags, offset, size, 0, nullptr, event, &error_code);
      } else {
         cl_event raw_event_ids[num_events];
         for(size_t i = 0; i < num_events; ++i) raw_event_ids[i] = event_wait_list[i].raw_identifier();
         result = clEnqueueMapBuffer(internal_id, buffer.raw_identifier(), synchronous_map, map_flags, offset, size, num_events, raw_event_ids, event, &error_code);
      }

      // Check if an error occured, and throw exceptions accordingly
      throw_if_failed(error_code);
      return result;
   }

   void CommandQueue::raw_unmap_mem_object(const MemoryObject & memobj, void * const mapped_ptr, const EventWaitList & event_wait_list, cl_event * event) const {
      const auto num_events = event_wait_list.size();
      if(num_events == 0) {
         throw_if_failed(clEnqueueUnmapMemObject(internal_id, memobj.raw_identifier(), mapped_ptr, 0, nullptr, event));
      } else {
         cl_event raw_event_ids[num_events];
         for(size_t i = 0; i < num_events; ++i) raw_event_ids[i] = event_wait_list[i].raw_identifier();
         throw_if_failed(clEnqueueUnmapMemObject(internal_id, memobj.raw_identifier(), mapped_ptr, num_events, raw_event_ids, event));
      }
   }

   void CommandQueue::raw_migrate_mem_objects(const ConstMemoryObjectRefVector & mem_objects, const cl_mem_migration_flags flags, const EventWaitList & event_wait_list, cl_event * event) const {
      // Convert the memory object list to its OpenCL representation
      const auto num_objects = mem_objects.size();
      cl_mem raw_object_ids[num_objects];
      for(size_t i = 0; i < num_objects; ++i) raw_object_ids[i] = mem_objects[i].get().raw_identifier();

      // Call for the migration of memory objects
      const auto num_events = event_wait_list.size();
      if(num_events == 0) {
         // If we are waiting for no event, null out the event wait list in final call
         throw_if_failed(clEnqueueMigrateMemObjects(internal_id, num_objects, raw_object_ids, flags, 0, nullptr, event));
      } else {
         // Convert the event wait list to its OpenCL representation before calling
         cl_event raw_event_ids[num_events];
         for(size_t i = 0; i < num_events; ++i) raw_event_ids[i] = event_wait_list[i].raw_identifier();
         throw_if_failed(clEnqueueMigrateMemObjects(internal_id, num_objects, raw_object_ids, flags, num_events, raw_event_ids, event));
      }
   }

   void CommandQueue::retain() const {
      throw_if_failed(clRetainCommandQueue(internal_id));
   }

   void CommandQueue::release() {
      throw_if_failed(clReleaseCommandQueue(internal_id));
   }

}
