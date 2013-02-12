//---------------------------------------------------------------------------
// Copyright (C) 2011 Robin Gilks
//
//
//  rpm.h   -   This program interfaces with timer0 an AVR CPU to implement a techometer
//
//  History:   1.0 - First release. 
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


extern int16_t gRPM, gMaxRPM;
extern int16_t gPoles;

void rpm_init (void);
void rpm_count (void);
void run_rpm (void);
