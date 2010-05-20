/**
 * Copyright (C) 2009-2010, Shao Miller <shao.miller@yrdsb.edu.on.ca>.
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
#ifndef _BUS_H
#  define _BUS_H

/**
 * @file
 *
 * Bus specifics
 *
 */

winvblock__def_struct ( bus__type )
{
  device__type device;
  PDEVICE_OBJECT LowerDeviceObject;
  PDEVICE_OBJECT PhysicalDeviceObject;
  winvblock__uint32 Children;
  winvblock__uint8_ptr first_child_ptr;
  KSPIN_LOCK SpinLock;
  LIST_ENTRY tracking;
};

extern NTSTATUS STDCALL Bus_GetDeviceCapabilities (
  IN PDEVICE_OBJECT DeviceObject,
  IN PDEVICE_CAPABILITIES DeviceCapabilities
 );

/**
 * Initialize the global, bus-common environment
 *
 * @ret ntstatus        STATUS_SUCCESS or the NTSTATUS for a failure
 */
extern STDCALL NTSTATUS bus__init (
  void
 );

/**
 * Tear down the global, bus-common environment
 */
extern void bus__finalize (
  void
 );

/**
 * Create a new bus
 *
 * @ret bus_ptr         The address of a new bus, or NULL for failure
 *
 * This function should not be confused with a PDO creation routine, which is
 * actually implemented for each device type.  This routine will allocate a
 * bus__type, track it in a global list, as well as populate the bus
 * with default values.
 */
extern winvblock__lib_func STDCALL bus__type_ptr bus__create (
  void
 );

/**
 * Add a child node to the bus
 *
 * @v dev_ptr           The details for the child device to add
 *
 * Returns TRUE for success, FALSE for failure
 */
extern winvblock__lib_func winvblock__bool STDCALL bus__add_child (
  IN device__type_ptr dev_ptr
 );

/**
 * Get a pointer to the bus device's extension space
 *
 * @ret         A pointer to the bus device's extension space, or NULL
 */
extern winvblock__lib_func bus__type_ptr STDCALL bus__dev (
  void
 );

/*
 * Establish a pointer to the bus.
 * Since the device extension is the first member of a bus
 * structure, a simple cast will suffice
 */
#  define get_bus_ptr( dev_ptr ) \
  ( ( bus__type_ptr ) dev_ptr )

#endif				/* _BUS_H */
