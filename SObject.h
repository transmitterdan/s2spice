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

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif /* WX_PRECOMP */
#include <wx/filename.h>

#include <sstream>
#include <iostream>
#include <utility>

#if defined(__WINDOWS__)
// Enable leak detection under windows
// For Linux use valgrind or other leak detection tool
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#if defined(NDEBUG)
#define DBG_NEW new
#else
#define DBG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
// allocations to be of _CLIENT_BLOCK type
#endif
#else
#define DBG_NEW new
#endif

#include <wx/wx.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>

// std libraries we use
#include <vector>
#include <complex>
#include <iomanip>
#include <list>
#include <string>
#include <Eigen/Dense>

using namespace std;
using namespace Eigen;

#if !defined(NDEBUG)
#if !defined(DEBUG_MESSAGE_BOX)
#define DEBUG_MESSAGE_BOX(MESS)                                           \
  std::{                                                                  \
    ostringstream message;                                                \
    message << "[" << __FILE__ << ":" << __LINE__ << "]" << endl << MESS; \
    (wxMessageBox(message.str(), _("Debug s2spice"),                      \
                  wxOK | wxICON_INFORMATION));                            \
  }
#endif
#else
#define DEBUG_MESSAGE_BOX(MESS) { }
#endif

inline bool HandleMessage(const wxString& mess, bool be_quiet) {
  if (be_quiet) {
    std::cout << mess << std::endl;
  } else {
    wxLogError(mess);
  }
  return false;
}

class Sparam {
public:
  Sparam() {
    Freq = 0;
    dB = ArrayXXd::Zero(2, 2);
    Phase = ArrayXXd::Zero(2, 2);
  }
  Sparam(int _n) {
    Freq = 0;
    dB = ArrayXXd::Zero(_n, _n);
    Phase = ArrayXXd::Zero(_n, _n);
  }
  Sparam(size_t _n) {
    Freq = 0;
    dB = ArrayXXd::Zero(_n, _n);
    Phase = ArrayXXd::Zero(_n, _n);
  }
  Sparam(double _f, size_t _n = 2) {
    Freq = _f;
    dB = ArrayXXd::Zero(_n, _n);
    Phase = ArrayXXd::Zero(_n, _n);
  }
  Sparam(double _f, const MatrixXd& _dB, const MatrixXd& _Phase) {
    Freq = _f;
    dB = _dB;
    Phase = _Phase;
  }
  Sparam(const Sparam& _s) : Freq(_s.Freq), dB(_s.dB), Phase(_s.Phase) {}
  Sparam& operator=(const Sparam& _s) {
    if (&_s == this) return *this;
    Freq = _s.Freq;
    dB = _s.dB;
    Phase = _s.Phase;
    return *this;
  }
  MatrixXd phaseRad() const { return (Phase * M_PI / 180.0); }
  MatrixXd phaseDeg() const { return (Phase); }
  MatrixXd mag() const {
    MatrixXd x = (dB / 20);
    MatrixXd res = pow(10, x.array());
    return res;
  }
  MatrixXcd Scplx() const {
    auto m = mag();
    auto p = phaseRad();
    MatrixXcd res(dB.rows(), dB.cols());
    res.real() = m.array() * cos(p.array());
    res.imag() = m.array() * sin(p.array());
    return res;
  }
  void cplxStore(const MatrixXcd& cp) {
    dB = 20.0 * log10(cp.cwiseAbs().array());
    Phase = (180.0 / M_PI) * cp.cwiseArg();
  }
  double Freq;     // Freq is stored as Hz
  MatrixXd dB;     // dB is stored as 20*log10(magnitude)
  MatrixXd Phase;  // Phase is stored as degrees
};

class SObject {
public:
  // Create-Destroy
  SObject();
  ~SObject();
  // Don't try to use operator=()
  SObject& operator=(const SObject& _s) = delete;
  // Accessors
  int nPorts(void) { return numPorts; }
  int nFreq(void) { return SData.size(); }
  double fBegin(void) { return SData.begin()->Freq; }
  double fEnd(void) { return (SData.end() - 1)->Freq; }
  bool dataSaved(void) { return (SData.empty() || data_saved); }
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
  wxFileName getSNPfile() { return snp_file; }
  wxFileName getASYfile() { return asy_file; }
  wxFileName getLIBfile() { return lib_file; }

  // Data processors
  bool openSFile(wxWindow* parent);
  bool readSFile(wxFileName& fileName);
  bool writeLibFile(wxWindow* parent);
  bool writeSymFile(wxWindow* parent);
  bool WriteASY();
  bool WriteLIB();

  // Clean out the object and prep to import another
  void Clean();

private:
  // Step 0: file targets + defaults
  void InitTargetsAndDefaults(const wxFileName& SFile);

  // Step 1: decide V2 and (if not V2) numPorts from extension
  bool DeterminePortsAndVersionFromExt();

  // Step 2: scan lines and collect metadata + raw data strings
  bool ParseTouchstone(wxTextInputStream& in);

  // Step 3: parse the "# ..." header options for units/format/type/Z0
  bool ParseOptionsFromHeader();

  // Step 4: final validation before conversion
  bool ValidateAfterParse() const;

  vector<Sparam> SData;
  string data_strings;            // String array of data from SnP file
  wxArrayString comment_strings;  // String array of comments from SnP file
  bool data_saved;                // have we saved in imported S-parameter file
  bool be_quiet;
  bool force;  // force overwrite of files without complaining
  bool error;
  int numPorts;  // number of ports in this file (comes from file name)
  bool Swap;
  wxFileName snp_file;  // file that is currently loaded
  wxFileName asy_file;
  wxFileName lib_file;
  double fUnits;           // frequency units
  double Z0;               // reference Z
  vector<double> Ref;      // reference impedance for each port
  int numFreq;             // number of frequency points
  double Ver;              // S-parameter file version
  string inputFormat;      // data format (DB, MA or RI)
  string parameterType;    // type of parameter (S is the only allowed type)
  wxString option_string;  // meta data strings

  // These functions create a string list describing a LTspice symbol
  list<string> Symbol(const string& symname) const;
  list<string> Symbol1port(const string& symname) const;
  list<string> Symbol2port(const string& symname) const;

  // Convert text to S-parameters
  bool Convert2S();

  // Convert H to S-parameters
  MatrixXcd h2s(const MatrixXcd& H, double Z0, double Y0) const;

  // Convert the saved value back to original source format type
  // The data is always stored internally as dB/Phase_degrees
  // So we use this to convert it back to the original S-parameter
  // file data type
  void Convert2Input(double& A, double& B);
};

#endif