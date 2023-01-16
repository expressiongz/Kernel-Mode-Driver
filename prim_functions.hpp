#pragma once
#include "includes.hpp"

namespace major_functions
{
    /// <summary>
    /// IRP_MJ_CREATE Major Function
    /// </summary>
    /// <param name=""></param>
    /// <param name="irp_ptr"></param>
    /// <returns></returns>
    inline NTSTATUS mj_create( PDEVICE_OBJECT, PIRP irp_ptr )
    {
        util::log( "%s", "MJ_CREATE Major Function called." );

        // denying handle if connection with user-mode process already
        // established
        if ( driver_globals::connection_initialized )
        {
            util::log( "%s",
                       "Cannot establish connection with 2 user-mode processes "
                       "at once." );
            return STATUS_ACCESS_DENIED;
        }
        irp_ptr->IoStatus.Information = 0;
        irp_ptr->IoStatus.Status = STATUS_SUCCESS;

        IofCompleteRequest( irp_ptr, IO_NO_INCREMENT );
        return STATUS_SUCCESS;
    }
    /// <summary>
    /// IRP_MJ_CLOSE Major Function
    /// </summary>
    /// <param name=""></param>
    /// <param name="irp_ptr"></param>
    /// <returns></returns>
    inline NTSTATUS mj_close( PDEVICE_OBJECT, PIRP irp_ptr )
    {
        util::log( "%s", "MJ_CLOSE Major Function called." );
        // resetting for preparation for a new user-mode process to establish a
        // connection to.
        driver_globals::connection_initialized = false;
        driver_globals::um_process.um_process = nullptr;
        driver_globals::um_process.um_process_id = 0;

        irp_ptr->IoStatus.Information = 0;
        irp_ptr->IoStatus.Status = STATUS_SUCCESS;

        IofCompleteRequest( irp_ptr, IO_NO_INCREMENT );
        return STATUS_SUCCESS;
    }
    /// <summary>
    /// This function is responsible for dispatching and initializing
    /// connections with user-mode processes.
    /// </summary>
    /// <param name=""></param>
    /// <param name="irp_ptr"></param>
    /// <returns></returns>
    NTSTATUS mj_ioctl_dispatcher( PDEVICE_OBJECT, PIRP irp_ptr )
    {
        util::log( "%s", "MJ_IOCTL_HANDLER Major Function called." );

        const auto irp_stack_location = IoGetCurrentIrpStackLocation( irp_ptr );
        const auto ctl_code =
            irp_stack_location->Parameters.DeviceIoControl.IoControlCode;

        // check for valid control codes.
        if ( ( ctl_code != ioctl_codes::read_code ) &&
             ( ctl_code != ioctl_codes::write_code ) &&
             ( ctl_code != ioctl_codes::init_connection_code ) )
        {
            util::log(
                "%s %i",
                "Invalid control code passed as DeviceIoControl parameter. "
                "Returning STATUS_INVALID_PARAMETER. Control Code: ",
                ctl_code );
            return STATUS_INVALID_PARAMETER;
        }

        util::log( "%s %i", "Io Control Code Passed: ", ctl_code );
        // initializing/establishing connection with user-mode process.
        if ( ctl_code == ioctl_codes::init_connection_code )
        {
            if ( driver_globals::connection_initialized )
            {
                util::log(
                    "%s %i",
                    "Connection with user-mode process already established." );
                return STATUS_ACCESS_DENIED;
            }
            util::log( "%s",
                       "INITIALIZE_CONNECTION control code passed. "
                       "Initializing connection with user-mode process." );

            const auto um_process_id = *static_cast< uint64_t* >(
                irp_ptr->AssociatedIrp.SystemBuffer );
            const auto um_process_lookup_res = PsLookupProcessByProcessId(
                reinterpret_cast< HANDLE >( um_process_id ),
                &driver_globals::um_process.um_process );

            if ( um_process_lookup_res != STATUS_SUCCESS )
            {
                util::log( "%s %i",
                           "PsLookupProcessByProcessId failed. Error code: ",
                           um_process_lookup_res );
                return um_process_lookup_res;
            }

            driver_globals::um_process.um_process_id = um_process_id;
            driver_globals::connection_initialized = true;
            util::log( "%s",
                       "Connection with user-mode process established. KM_READ "
                       "and KM_WRITE now available." );
            return STATUS_SUCCESS;
        }

        if ( !driver_globals::connection_initialized )
        {
            util::log( "%s %i",
                       "Cannot use KM_READ or KM_WRITE until connection with "
                       "user-mode process is established." );
            return STATUS_ACCESS_DENIED;
        }

        const auto& [ target_process_id, rw_length, write_to, read_from ] =
            *static_cast< structs::vmem_arg_t* >(
                irp_ptr->AssociatedIrp.SystemBuffer );

        PEPROCESS target_process{ nullptr };
        const auto target_proc_lookup_res = PsLookupProcessByProcessId(
            reinterpret_cast< HANDLE >( target_process_id ), &target_process );

        if ( target_proc_lookup_res != STATUS_SUCCESS )
        {
            util::log( "%s %i",
                       "PsLookupProcessByProcessId call failed. Error code: ",
                       ctl_code );
            return ctl_code;
        }

        if ( ctl_code == ioctl_codes::read_code )
        {
            util::log( "%s", "KM_READ Called.\n" );
            return util::memory::KM_READ( target_process, read_from, write_to,
                                          rw_length );
        }

        util::log( "%s", "KM_WRITE Called.\n" );
        return util::memory::KM_WRITE( target_process, read_from, write_to,
                                       rw_length );
    }
    /// <summary>
    /// This function is responsible for cleaning up.
    /// </summary>
    /// <param name="driver_object_ptr"></param>
    inline void DriverUnload( PDRIVER_OBJECT pdriver_object )
    {
        util::log( "%s",
                   "DriverUnload callback called. Cleaning up the driver." );
        IoDeleteSymbolicLink( &driver_globals::driver_data::symbolic_link_name );
        IoDeleteDevice( pdriver_object->DeviceObject );
        ObfDereferenceObject( &driver_globals::um_process.um_process );
        util::log( "%s", "DriverUnload call complete." );
    }

} // namespace major_functions
