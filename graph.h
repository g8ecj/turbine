//---------------------------------------------------------------------------
// Copyright (C) 2011 Robin Gilks
//
//
//  control.c   -   This program measures the output from and controls the load on a wind turbine
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

#ifndef _GRAPH_H
#define _GRAPH_H


#define MINGRAPH 1
#define HOURGRAPH 2
#define DAYGRAPH 3

#define DEGREE  1
#define SDCARD  2
#define BOTQUAR 3
#define BOTHALF 4
#define BOTTHRE 5
#define TOPTHRE 6
#define TOPHALF 7
#define TOPQUAR 8


void graph_init (void);
void run_graph (void);
void display_graph (KFile *stream, uint8_t type);

#endif
