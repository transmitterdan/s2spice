/***************************************************************************
 *  s2spice  Copyright (C) 2023 by Dan Dickey                              *
 *                                                                         *
 * This program is free software: you can redistribute it and/or modify    *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation, either version 3 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.  *
 ***************************************************************************
 *
 * Project:  S2spice
 * Purpose:  S2spice wxWidgets Program converts S-parameter files to Spice
 *           subcircuit library file.
 * Author:   Dan Dickey
 *
 * Based on: s2spice.c
 * (https://groups.io/g/LTspice/files/z_yahoo/Tut/S-Parameter/s2spice.doc)
 *
 ***************************************************************************/
#if !defined(__SOBJECT)
#define __SOBJECT
#if defined(_MSC_VER)
#pragma once
#endif

#include <wx/wx.h>

using namespace std;

// std libraries we use
#include <vector>
#include <complex>
#include <iomanip>

#include "xqsmatrix.h"

#if !defined(NDEBUG)
#if !defined(DEBUG_MESSAGE_BOX)
#define DEBUG_MESSAGE_BOX(MESS)                                           \
  {                                                                       \
    ostringstream message;                                                \
    message << "[" << __FILE__ << ":" << __LINE__ << "]" << endl << MESS; \
    (wxMessageBox(message.str(), _("Debug s2spice"),                      \
                  wxOK | wxICON_INFORMATION));                            \
  }
#endif
#else
#define DEBUG_MESSAGE_BOX(MESS) {}
#endif

class Sparam {
public:
  Sparam() { Freq = 0.0; };
  Sparam(std::size_t n);
  double Freq;
  XQSMatrix<double> dB;
  XQSMatrix<double> Phase;
};

class SObject {
public:
  SObject();
  ~SObject();
  int nPorts(void) { return numPorts; }
  int nFreq(void) { return SData.size(); }
  bool dataSaved(void) { return (SData.empty() || data_saved); }
  bool openSFile(wxWindow* parent);
  bool readSFile(wxFileName& fileName);
  bool writeLibFile(wxWindow* parent);
  bool writeSymFile(wxWindow* parent);
  wxFileName getSNPfile() { return snp_file; }
  wxFileName getASYfile() { return asy_file; }
  wxFileName getLIBfile() { return lib_file; }
  bool WriteASY();
  bool WriteLIB();
  bool SetQuiet(bool flag) {
    bool res = be_quiet;
    be_quiet = flag;
    return res;
  };
  bool GetQuiet() { return be_quiet; };
  bool SetForce(bool flag) {
    bool res = force;
    force = flag;
    return res;
  };
  bool GetForce() { return force; };
  void Clean();

private:
  vector<Sparam> SData;
  string data_strings;            // String array of data from SnP file
  wxArrayString comment_strings;  // String array of comments from SnP file
  bool data_saved;                // have we saved in imported S-parameter file
  bool be_quiet;
  bool force;           // force overwrite of files without complaining
  int numPorts;         // number of ports in this file (comes from file name)
  wxFileName snp_file;  // file that is currently loaded
  wxFileName asy_file;
  wxFileName lib_file;
  double fUnits;           // frequency units
  double Z0;               // reference Z
  wxString format;         // data format (DB, MA or RI)
  wxString parameterType;      // type of parameter (S is the only allowed type)
  wxString option_string;  // meta data strings
  // This function reads the contents of the file into a vector of points
  vector<pair<double, double>> ReadFile(const string& fileName);

  // This function creates a string vector describing a LTspice symbol
  vector<string> Symbol(const string& symname) const;
  vector<string> Symbol1port(const string& symname) const;
  vector<string> Symbol2port(const string& symname) const;

  // Read in .snp file
  bool ReadSNP(const wxFileName& file);
  // Convert text to S-parameters
  bool Convert2S();
};

inline void SObject::Clean() {
  SData.clear();
  data_strings.clear();
  comment_strings.clear();
  data_saved = true;
}

#endif