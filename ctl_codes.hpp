#pragma once
#include "includes.hpp"

// ioctl control codes, for communication with user mode processes.
namespace ioctl_codes
{
	inline constexpr auto read_code			   = CTL_CODE( FILE_DEVICE_UNKNOWN, 0x234a8e, METHOD_BUFFERED, FILE_SPECIAL_ACCESS );
	inline constexpr auto write_code		   = CTL_CODE( FILE_DEVICE_UNKNOWN, 0x3d5809, METHOD_BUFFERED, FILE_SPECIAL_ACCESS );
	inline constexpr auto init_connection_code = CTL_CODE( FILE_DEVICE_UNKNOWN, 0x2d8b24, METHOD_BUFFERED, FILE_SPECIAL_ACCESS );
}
