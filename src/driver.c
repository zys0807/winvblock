/**
 * Copyright (C) 2009, Shao Miller <shao.miller@yrdsb.edu.on.ca>.
 * Copyright 2006-2008, V.
 * For WinAoE contact information, see http://winaoe.org/
 *
 * This file is part of WinVBlock, derived from WinAoE.
 *
 * WinVBlock is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * WinVBlock is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WinVBlock.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 *
 * Driver specifics
 *
 */

#include <stdio.h>
#include <ntddk.h>

#include "winvblock.h"
#include "portable.h"
#include "driver.h"
#include "debug.h"
#include "registry.h"
#include "disk.h"
#include "bus.h"
#include "protocol.h"
#include "aoe.h"
#include "debug.h"
#include "probe.h"

/* in this file */
static NTSTATUS STDCALL Driver_DispatchNotSupported (
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
 );

static VOID STDCALL Driver_Unload (
	IN PDRIVER_OBJECT DriverObject
 );

static PVOID Driver_Globals_StateHandle;
static winvblock__bool Driver_Globals_Started = FALSE;

/*
 * Note the exception to the function naming convention.
 * TODO: See if a Makefile change is good enough
 */
NTSTATUS STDCALL
DriverEntry (
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
 )
{
	NTSTATUS Status;
	PDEVICE_OBJECT PDODeviceObject = NULL;
	int i;

		/**
     * TODO: Remove this fun test
     *
     * LARGE_INTEGER NapTime;
     * PWCHAR DisplayStringInternal;
     * UNICODE_STRING DisplayString;
     */

	DBG ( "Entry\n" );
	if ( Driver_Globals_Started )
		return STATUS_SUCCESS;
	Debug_Initialize (  );
	if ( !NT_SUCCESS ( Status = Registry_Check (  ) ) )
		return Error ( "Registry_Check", Status );
	if ( !NT_SUCCESS ( Status = Bus_Start (  ) ) )
		return Error ( "Bus_Start", Status );

		/**
     * TODO: Remove this fun test
     *
     * NapTime.QuadPart = -10000000;
     * DisplayStringInternal = L"\nHello!\n";
     * DisplayString.Buffer = DisplayStringInternal;
     * DisplayString.Length = wcslen ( DisplayStringInternal ) * sizeof ( WCHAR );
     * DisplayString.MaximumLength = DisplayString.Length + sizeof ( WCHAR );
     * while ( 1 ) {
     *     i = 0;
     *     while ( i < 65535 ) i++;
     *     ZwDisplayString ( &DisplayString );
     * }
     */

	Driver_Globals_StateHandle = NULL;

	if ( ( Driver_Globals_StateHandle =
				 PoRegisterSystemState ( NULL, ES_CONTINUOUS ) ) == NULL )
		{
			DBG ( "Could not set system state to ES_CONTINUOUS!!\n" );
		}

	/*
	 * Set up IRP MajorFunction function table for devices
	 * this driver handles
	 */
	for ( i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++ )
		DriverObject->MajorFunction[i] = Driver_DispatchNotSupported;
	DriverObject->MajorFunction[IRP_MJ_PNP] = Driver_Dispatch;
	DriverObject->MajorFunction[IRP_MJ_POWER] = Driver_Dispatch;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = Driver_Dispatch;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = Driver_Dispatch;
	DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = Driver_Dispatch;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Driver_Dispatch;
	DriverObject->MajorFunction[IRP_MJ_SCSI] = Driver_Dispatch;
	/*
	 * Other functions this driver handles
	 */
	DriverObject->DriverExtension->AddDevice = Bus_AddDevice;
	DriverObject->DriverUnload = Driver_Unload;
	IoReportDetectedDevice ( DriverObject, InterfaceTypeUndefined, -1, -1, NULL,
													 NULL, FALSE, &PDODeviceObject );
	if ( !NT_SUCCESS
			 ( Status = Bus_AddDevice ( DriverObject, PDODeviceObject ) ) )
		{
			Protocol_Stop (  );
			AoE_Stop (  );
			return Error ( "Bus_AddDevice", Status );
		}
	probe__disks (  );
	Driver_Globals_Started = TRUE;
	return Status;
}

static NTSTATUS STDCALL
Driver_DispatchNotSupported (
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
 )
{
	Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	IoCompleteRequest ( Irp, IO_NO_INCREMENT );
	return Irp->IoStatus.Status;
}

NTSTATUS STDCALL
Driver_Dispatch (
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
 )
{
	NTSTATUS Status;
	PIO_STACK_LOCATION Stack;
	driver__dev_ext_ptr DeviceExtension;

#ifdef DEBUGIRPS
	Debug_IrpStart ( DeviceObject, Irp );
#endif
	Stack = IoGetCurrentIrpStackLocation ( Irp );
	DeviceExtension = ( driver__dev_ext_ptr ) DeviceObject->DeviceExtension;
	if ( DeviceExtension->State == Deleted )
		{
			if ( Stack->MajorFunction == IRP_MJ_POWER )
				PoStartNextPowerIrp ( Irp );
			Irp->IoStatus.Information = 0;
			Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
			IoCompleteRequest ( Irp, IO_NO_INCREMENT );
#ifdef DEBUGIRPS
			Debug_IrpEnd ( Irp, STATUS_NO_SUCH_DEVICE );
#endif
			return STATUS_NO_SUCH_DEVICE;
		}
	switch ( Stack->MajorFunction )
		{
			case IRP_MJ_CREATE:
			case IRP_MJ_CLOSE:
				Status = STATUS_SUCCESS;
				Irp->IoStatus.Status = Status;
				IoCompleteRequest ( Irp, IO_NO_INCREMENT );
				break;
			default:
				Status =
					DeviceExtension->Dispatch ( DeviceObject, Irp, Stack,
																			DeviceExtension );
		}
#ifdef DEBUGIRPS
	if ( Status != STATUS_PENDING )
		Debug_IrpEnd ( Irp, Status );
#endif
	return Status;
}

static VOID STDCALL
Driver_Unload (
	IN PDRIVER_OBJECT DriverObject
 )
{
	if ( Driver_Globals_StateHandle != NULL )
		PoUnregisterSystemState ( Driver_Globals_StateHandle );
	Protocol_Stop (  );
	AoE_Stop (  );
	Bus_Stop (  );
	Driver_Globals_Started = FALSE;
	DBG ( "Done\n" );
}

VOID STDCALL
Driver_CompletePendingIrp (
	IN PIRP Irp
 )
{
#ifdef DEBUGIRPS
	Debug_IrpEnd ( Irp, Irp->IoStatus.Status );
#endif
	IoCompleteRequest ( Irp, IO_NO_INCREMENT );
}

/*
 * Note the exception to the function naming convention
 */
NTSTATUS STDCALL
Error (
	IN PCHAR Message,
	IN NTSTATUS Status
 )
{
	DBG ( "%s: 0x%08x\n", Message, Status );
	return Status;
}
