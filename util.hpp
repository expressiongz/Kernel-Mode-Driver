#pragma once
#include "includes.hpp"

extern "C" NTSTATUS __stdcall MmCopyVirtualMemory( PEPROCESS, PVOID, PEPROCESS,
                                                   PVOID, SIZE_T,
                                                   KPROCESSOR_MODE, PSIZE_T );

constexpr auto debug = true;

namespace structs
{
    struct vmem_arg_t
    {
        uint32_t target_process_id;
        uint32_t rw_length;
        void* va_write_to;
        void* va_read_from;
    };
    struct um_process_t
    {
        PEPROCESS um_process;
        uint32_t um_process_id;
    };
} // namespace structs

namespace driver_globals
{
    namespace driver_data
    {
        inline DEVICE_OBJECT* device_object{ nullptr };
        inline UNICODE_STRING symbolic_link_name{ };
        inline UNICODE_STRING driver_name{ };
    } // namespace driver_data

    inline bool connection_initialized{ false };
    inline structs::um_process_t um_process{ };
} // namespace driver_globals

namespace util
{

    template < typename... args_t >
    inline void log( const char* format, args_t&&... args )
    {
        if constexpr ( debug )
            DbgPrintEx( 0, 0, format, std::forward< args_t >( args )... );
    }

    namespace memory
    {
        /// <summary>
        /// Function responsible for writing memory to other processes.
        /// </summary>
        /// <param name="process"></param>
        /// <param name="write_buffer"></param>
        /// <param name="write_to"></param>
        /// <param name="num_bytes"></param>
        /// <returns></returns>
        NTSTATUS KM_WRITE( PEPROCESS process, void* write_buffer,
                           void* write_to, unsigned long num_bytes )
        {
            const auto mmcopyvmem_res = MmCopyVirtualMemory(
                driver_globals::um_process.um_process, write_buffer, process,
                write_to, num_bytes, KernelMode, nullptr );
            ObfDereferenceObject( process );
            if ( mmcopyvmem_res != STATUS_SUCCESS )
            {
                util::log( "%s %i", "MmCopyVirtualMemory failed. Error code: ",
                           mmcopyvmem_res );
                return mmcopyvmem_res;
            }
            util::log( "%s", "KM_WRITE succeeded. Returning STATUS_SUCCESS" );
            return STATUS_SUCCESS;
        }
        /// <summary>
        /// Function responsible for reading memory from processes.
        /// </summary>
        /// <param name="process"></param>
        /// <param name="read_from"></param>
        /// <param name="write_to"></param>
        /// <param name="num_bytes"></param>
        /// <returns></returns>
        NTSTATUS KM_READ( PEPROCESS process, void* read_from, void* write_to,
                          unsigned long num_bytes )
        {
            const auto mmcopyvmem_res = MmCopyVirtualMemory(
                process, read_from, driver_globals::um_process.um_process,
                write_to, num_bytes, KernelMode, nullptr );
            ObfDereferenceObject( process );
            if ( mmcopyvmem_res != STATUS_SUCCESS )
            {
                util::log( "%s %i", "MmCopyVirtualMemory failed. Error code: ",
                           mmcopyvmem_res );
                return mmcopyvmem_res;
            }
            util::log( "%s", "KM_READ succeeded. Returning STATUS_SUCCESS" );
            return STATUS_SUCCESS;
        }

    } // namespace memory
} // namespace util
