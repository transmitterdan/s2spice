/***************************************************************************
 *
 * Project:  S2spice
 * Purpose:  S2spice wxWidgets Program converts S-parameter files to Spice
 *           subcircuit library file.
 * Author:   Dan Dickey
 *
 * Based on: s2spice.c
 * (https://groups.io/g/LTspice/files/z_yahoo/Tut/S-Parameter/s2spice.doc)
 *
 ***************************************************************************
 *   Copyright (C) 2023 by Dan Dickey                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
 **************************************************************************/
#if !defined(__SOBJECT)
#define __SOBJECT
#if defined(_MSC_VER)
#pragma once
#endif

using namespace std;

// std libraries we use
#include <vector>
#include <complex>

#include "xqsmatrix.h"

class Sparam {
public:
  Sparam(){};
  Sparam(std::size_t n);
  double Freq;
  XQSMatrix<double> dB;
  XQSMatrix<double> Phase;
};

class SObject : public wxObject {
public:
  SObject();
  int nPorts(void) { return numPorts; }
  int nFreq(void) { return SData.size(); }
  bool dataSaved(void) { return (SData.empty() || data_saved); }
  bool readSfile(wxWindow* parent);
  bool writeLibFile(wxWindow* parent);
  bool writeSymFile(wxWindow* parent);
  wxFileName getSNPfile() { return snp_file; }
  wxFileName getASYfile() { return asy_file; }
  wxFileName getLIBfile() { return lib_file; }

private:
  vector<Sparam> SData;
  string data_strings;            // String array of data from SnP file
  wxArrayString comment_strings;  // String array of comments from SnP file
  bool data_saved;                // have we saved in imported S-parameter file
  int numPorts;         // number of ports in this file (comes from file name)
  wxFileName snp_file;  // file that is currently loaded
  wxFileName asy_file;
  wxFileName lib_file;
  double fUnits;           // frequency units
  double Z0;               // reference Z
  wxString format;         // data format (DB, MA or RI)
  wxString parameter;      // type of parameter (S is the only allowed type)
  wxString option_string;  // meta data strings

  // This function reads the contents of the file into a vector of points
  vector<pair<double, double>> ReadFile(const string& fileName);

  // This function creates a string vector describing a LTspice symbol
  vector<string> Symbol(const string& symname) const;
  vector<string> Symbol1port(const string& symname) const;
  vector<string> Symbol2port(const string& symname) const;

  // Read in .snp file
  bool ReadSNP(const wxFileName& file);
  // Write .asy file
  bool WriteASY(const wxFileName& file);
  // Write .lib file
  bool WriteLIB(const wxFileName& file);

  // Convert text to S-parameters
  bool Convert2S();
};

#endif