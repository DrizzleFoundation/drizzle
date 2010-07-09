/* Copyright (c) 2010 PrimeBase Technologies GmbH, Germany
 *
 * PrimeBase Media Stream Daemon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Author: Barry Leslie
 *
 */
#ifndef __PBMS_VERSION_H__
#define __PBMS_VERSION_H__

// In the MySQL build the version number is set in configure.in AM_INIT_AUTOMAKE()
// In the drizzle build it is set in plugin.in

#ifndef PBMS_VERSION
#define PBMS_VERSION 0.5.14-betaxx
#endif

#define _PBMSSTR(x) #x
#define PBMSSTR(x) _PBMSSTR(x)

class PBMSVersion
{
public:
	static const char *getCString() { return PBMSSTR(PBMS_VERSION); }
};

#endif // __PBMS_VERSION_H__