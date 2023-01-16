#include "includes.hpp"

/// <summary>
/// Sets up all Major Functions
/// </summary>
/// <param name="pdriver_object"></param>

void mj_function_setup( PDRIVER_OBJECT pdriver_object )
{

    util::log( "%s", "Starting Major Function setup." );
    pdriver_object->DriverUnload = &major_functions::DriverUnload;
    pdriver_object->MajorFunction[ IRP_MJ_CREATE ] =
        &major_functions::mj_create;
    pdriver_object->MajorFunction[ IRP_MJ_CLOSE ] = &major_functions::mj_close;
    pdriver_object->MajorFunction[ IRP_MJ_DEVICE_CONTROL ] =
        &major_functions::mj_ioctl_dispatcher;
    util::log(
        "%s",
        "Major Function setup complete for KM READWRITE Driver finished." );
}

/// <summary>
/// Driver Entry to set up the driver
/// </summary>
/// <param name="pdriver_object"></param>
/// <param name=""></param>
/// <returns></returns>
extern "C" NTSTATUS DriverEntry( PDRIVER_OBJECT pdriver_object,
                                 PUNICODE_STRING )
{
    util::log(
        "%s",
        "DriverEntry callback called. Starting KM READWRITE Driver setup." );

    // set up major functions
    

    // set up device object and symbolic link
    RtlInitUnicodeString( &driver_globals::driver_data::driver_name,
                          L"\\Device\\km_readwrite" );
    RtlInitUnicodeString( &driver_globals::driver_data::symbolic_link_name,
                          L"\\DosDevices\\km_readwrite" );

    const auto device_obj_creation_status = IoCreateDevice(
        pdriver_object, 0, &driver_globals::driver_data::driver_name,
        FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, false,
        &driver_globals::driver_data::device_object );

    if ( device_obj_creation_status != STATUS_SUCCESS )
    {
        util::log( "%s %i", "IoCreateDevice failed. Returning: ",
                   device_obj_creation_status );
        IoDeleteDevice( pdriver_object->DeviceObject );
        return device_obj_creation_status;
    }

    util::log( "%s", "Created Driver Device for KM READWRITE Driver" );
    driver_globals::driver_data::device_object->Flags |= DO_DIRECT_IO;
    driver_globals::driver_data::device_object->Flags &=
        ~DO_DEVICE_INITIALIZING;

    const auto symbolic_link_creation_status =
        IoCreateSymbolicLink( &driver_globals::driver_data::symbolic_link_name,
                              &driver_globals::driver_data::driver_name );
    if ( symbolic_link_creation_status != STATUS_SUCCESS )
    {
        util::log( "%s %i", "IoCreateSymbolicLink failed. Returning: ",
                   symbolic_link_creation_status );
        IoDeleteDevice( pdriver_object->DeviceObject );
        return symbolic_link_creation_status;
    }
    util::log( "%s", "Created Symbolic Link." );
    mj_function_setup( pdriver_object );
    util::log( "%s", "KM READWRITE Driver ready for use." );
    return STATUS_SUCCESS;
}
