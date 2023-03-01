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

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif /* WX_PRECOMP */
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>

#include <sstream>
#include <iostream>
#include <fstream>
#include <utility>
#include <complex>
#include <algorithm>
#include <cstdio>

using namespace std;

#include "SObject.h"
#include "stringformat.hpp"

/*
The formula for converting H-parameters to S-parameters for an arbitrary number
of ports is as follows:

S = (Z0/Y0) * (I + H) * (I - H)^-1

Where Z0 is the reference impedance, Y0 is the admittance of the reference
plane, I is the identity matrix, and H is the matrix of H-parameters.

It is important to note that this formula is only applicable to linear,
time-invariant systems and that the number of ports must be known and consistent
throughout the conversion.

#include <iostream>
#include <complex>
#include <Eigen/Dense>

using namespace std;
using namespace Eigen;

MatrixXcd h2s(const MatrixXcd& H, double Z0, double Y0) {
    int n = H.rows(); // get the number of ports
    // create the identity matrix
    MatrixXcd I = MatrixXcd::Identity(n, n);
    // calculate S-parameters
    MatrixXcd S = (Z0/Y0) * (I + H) * (I - H).inverse();
    return S;
}

int main() {
    // example usage
    MatrixXcd H(2,2);
    H << complex<double>(1,2), complex<double>(-3,-4),
         complex<double>(5,6), complex<double>(-7,-8);
    double Z0 = 50; // ohms
    double Y0 = 1/Z0;
    MatrixXcd S = h2s(H, Z0, Y0);
    cout << "S-parameters:" << endl << S << endl;
    return 0;
}

*/

SObject::SObject() {
  Clean();
  numPorts = 0;
  fUnits = 0;
  Z0 = 50;
  be_quiet = false;
  error = false;
}

SObject::~SObject() { Clean(); }

void SObject::Clean() {
  SData.clear();
  data_strings.clear();
  comment_strings.clear();
  data_saved = true;
}

bool SObject::openSFile(wxWindow* parent) {
#if defined(_WIN32) || defined(_WIN64)
  char const* WildcardStr = "S paramter (*.snp)|*.S?P;*.S??P|H paramter (*.hnp)|*.H?P;H??P|All files (*.*)|*.*";
#else
  // On non-Windows platforms we try to find mostly snp files but
  // they don't all allow ? as a wildcard
  char const* WildcardStr = "S paramter (*p)|*p;*P|All files (*)|*";
#endif
  if (!dataSaved()) {
    if (wxMessageBox(
            _("Current content has not been saved!\nDiscard current data?"),
            _("Please confirm"), wxICON_QUESTION | wxYES_NO, parent) == wxNO)
      return false;
  }
  wxFileDialog openFileDialog(parent, _("Open SnP file"), "", "", WildcardStr,
                              wxFD_OPEN | wxFD_FILE_MUST_EXIST);

  if (openFileDialog.ShowModal() == wxID_CANCEL)
    return false;  // the user changed idea...

  wxBusyCursor wait;
  wxFileName fileName = openFileDialog.GetPath();
  return readSFile(fileName);
}

bool SObject::readSFile(wxFileName& SFile) {
  Clean();
  snp_file = SFile;
  lib_file = SFile;
  asy_file = SFile;
  string lib_name = SFile.GetName().ToStdString();
  replace_if(lib_name.begin(), lib_name.end(), ::isspace, '_');
  lib_file.SetName(lib_name);
  lib_file.SetExt("lib");
  asy_file.SetName(lib_name);
  asy_file.SetExt("asy");
#if !defined(NDEBUG)
  wxString cwd = wxGetCwd();
#endif
  if (!snp_file.FileExists()) {
    wxString mess = wxString::Format(_("[%s:%d]\nFile '%s' does not exist.\n"
                                       "Current working directory: '%s'"),
                                     __FILE__, __LINE__, snp_file.GetFullPath(),
                                     wxGetCwd());
    // DEBUG_MESSAGE_BOX(wxString::Format(_("Flag be_quiet = %d."), be_quiet))
    if (be_quiet) {
      cout << mess << endl;
    } else {
      wxLogError(mess);
    }
    return false;
  }

  {
    wxFileInputStream input_stream(snp_file.GetFullPath());
    if (!input_stream.IsOk()) {
      wxString mess =
          wxString::Format(_("%s:%d Cannot open file '%s'."), __FILE__,
                           __LINE__, snp_file.GetFullPath());
      // DEBUG_MESSAGE_BOX(wxString::Format(_("Flag be_quiet = %d."), be_quiet))
      if (be_quiet) {
        cout << mess << endl;
      } else {
        wxLogError(mess);
      }
      return false;
    }
    // the numbers in the extension tell us the number of ports
    string ext(snp_file.GetExt().c_str());
    string strPorts;
    for (auto i : ext) {
      if (isdigit(i)) strPorts += i;
      if ((strPorts.length() > 0) && !(isdigit(i))) break;
    }
    numPorts = stoi(strPorts);
    if (numPorts < 1 || numPorts > 90) {
      wxString mess =
          wxString::Format(_("%s:%d SOjbect::readSFile:Cannot read file '%s'."),
                           __FILE__, __LINE__, snp_file.GetFullPath());
      if (be_quiet) {
        cout << mess << endl;
      } else {
        wxLogError(mess);
      }
      return false;
    }
    wxTextInputStream text_input(input_stream);
    wxString line;
    comment_strings.Empty();
    option_string.Clear();
    data_strings.clear();
    while (!text_input.GetInputStream().Eof()) {
      line.Empty();
      line = text_input.ReadLine();
      line.Trim();
      line.Trim(wxFalse);
      if (line.StartsWith("!"))
        comment_strings.push_back(line);
      else if (line.StartsWith(";"))
        comment_strings.push_back(line);
      else if (line.StartsWith("*"))
        comment_strings.push_back(line);
      else if (line.StartsWith("#"))
        option_string = line.MakeUpper();
      else {
        data_strings.append(line.ToStdString());
        data_strings.append(" ");
      }
    }
    inputFormat = "MAG";  // default mag/angle
    fUnits = 1e9;
    parameterType = "S";
    Z0 = 50;
    wxArrayString options(wxStringTokenize(
        option_string, wxDEFAULT_DELIMITERS, wxTOKEN_DEFAULT));
    for (size_t i = 0; i < options.GetCount(); i++) {
      if (options[i].Matches("GHZ"))
        fUnits = 1e9;
      else if (options[i].Matches("MHZ"))
        fUnits = 1e6;
      else if (options[i].Matches("KHZ"))
        fUnits = 1e3;
      else if (options[i].Matches("HZ"))
        fUnits = 1;
      else if (options[i].Matches("S"))
        parameterType = options[i].ToStdString();
      else if (options[i].Matches("Y"))
        parameterType = options[i].ToStdString();
      else if (options[i].Matches("Z"))
        parameterType = options[i].ToStdString();
      else if (options[i].Matches("H"))
        parameterType = options[i].ToStdString();
      else if (options[i].Matches("G"))
        parameterType = options[i].ToStdString();
      else if (options[i].StartsWith("DB"))
        inputFormat = "DB";
      else if (options[i].StartsWith("MA"))
        inputFormat = "MAG";
      else if (options[i].StartsWith("RI"))
        inputFormat = "R_I";
      else if (options[i].Matches("R"))
        options[i + 1].ToDouble(&Z0);
    }
  }
  if (!Convert2S()) {
    data_saved = true;
    data_strings.clear();
    return false;
  }
  data_saved = false;
  return !error;
}

bool SObject::writeLibFile(wxWindow* parent) {
  if (lib_file.FileExists() && !GetForce()) {
    wxString mess = wxString::Format(_("Library file '%s' exists. Overwrite?"),
                                     lib_file.GetFullPath());
    if (wxMessageBox(mess, _("Please confirm"), wxICON_QUESTION | wxYES_NO,
                     parent) == wxNO)
      return false;
  }
  return WriteLIB();
}

// Convert the stored S-parameter data (dB/phase) format
// into the original input file format so long as
// LTspice supports that format. Otherwize, don't convert.
void SObject::Convert2Input(double& A, double& B) {
  if (inputFormat.compare("DB") == 0) {
    return;
  } else if (inputFormat.compare("R_I") == 0) {
    double mag = pow(10.0, A / 20.0);
    double ph = B * M_PI / 180.0;
    A = mag * cos(ph);
    B = mag * sin(ph);
  } else if (inputFormat.compare("MAG") == 0) {
    double mag = pow(10.0, A / 20.0);
    double phase = B;
    A = mag;
    B = phase;
  } else {
    wxString mess = wxString::Format(
        _("%s:%d SOjbect::WriteLIB:Cannot handle %s format data file."),
        __FILE__, __LINE__, wxString(parameterType));
    if (be_quiet) {
      cout << mess << endl;
    } else {
      wxLogError(mess);
    }
    error = true;
  }
}

bool SObject::WriteLIB() {
  string libName(lib_file.GetFullPath().ToStdString());
  int npMult = 100;
  if (parameterType.compare("S") != 0) {
    wxString mess =
      wxString::Format(_("%s:%d SOjbect::WriteLIB:Cannot handle %s format data file."),
                       __FILE__, __LINE__, wxString(parameterType));
    if (be_quiet) {
      cout << mess << endl;
    } else {
      wxLogError(mess);
    }
    return false;
  }
	  
  ofstream output_stream(libName);
  if (!output_stream) {
    wxString mess =
        wxString::Format(_("%s:%d SOjbect::WriteLIB:Cannot create file '%s'."),
                         __FILE__, __LINE__, libName);
    if (be_quiet) {
      cout << mess << endl;
    } else {
      wxLogError(mess);
    }
    return false;
  }
  output_stream << ".SUBCKT " << lib_file.GetName() << " ";
  for (int i = 0; i < numPorts + 1; i++) output_stream << " " << i + 1;
  output_stream << "\n";
  output_stream
      << "* Pin " << numPorts + 1
      << " is the reference plane (usually it should be connected to GND)\n";

  for (int i = 0; i < comment_strings.Count(); i++) {
    output_stream << "*" << comment_strings[i].Mid(1) << "\n";
  }
  output_stream << "*" << option_string.Mid(1) << "\n";
  output_stream << "*";

  for (int i = 0; i < numPorts; i++) {
    output_stream << " Z" << i + 1 << " = " << Z0;
  }
  output_stream << "\n";

  /* define resistances for Spice model */

  for (int i = 0; i < numPorts; i++) {
    output_stream << stringFormat("R%dN %d %d %e\n", i + 1, i + 1,
                                  npMult * (i + 1), -Z0);
    output_stream << stringFormat("R%dP %d %d %f\n", i + 1, npMult * (i + 1),
                                  npMult * (i + 1) + 1, 2 * Z0);
  }

  output_stream << "\n";
  for (int i = 0; i < numPorts; i++) {
    for (int j = 0; j < numPorts; j++) {
      output_stream << stringFormat("* S%d%d FREQ %s\n ", i + 1, j + 1, inputFormat);
      if (j + 1 == numPorts) {
        output_stream << stringFormat(
            "E%02d%02d %d %d FREQ {V(%d,%d)}= %s\n", i + 1, j + 1,
            npMult * (i + 1) + j + 1, numPorts + 1, npMult * (j + 1),
            numPorts + 1, inputFormat);
      } else {
        output_stream << stringFormat(
            "E%02d%02d %d %d FREQ {V(%d,%d)}= %s\n", i + 1, j + 1,
            npMult * (i + 1) + j + 1, npMult * (i + 1) + j + 2,
            npMult * (j + 1), numPorts + 1, inputFormat);
      }
      for (auto s = SData.begin(); s != SData.end(); s++) {
        double A = s->dB(i, j);
        double B = s->Phase(i, j);
        Convert2Input(A, B);
        output_stream << stringFormat("+(%14eHz,%14e,%14e)\n", s->Freq, A, B);
      }
    }
    output_stream << "\n";
  }

  output_stream << ".ENDS * " << lib_file.GetName() << "\n";
  output_stream.close();
  data_saved = true;
  return !error;
}

bool SObject::writeSymFile(wxWindow* parent) {
  if (asy_file.FileExists() && !GetForce()) {
    wxString mess = wxString::Format(_("Symbol file '%s' exists. Overwrite?"),
                                     asy_file.GetFullPath());
    if (wxMessageBox(mess, _("Please confirm"), wxICON_QUESTION | wxYES_NO,
                     parent) == wxNO)
      return false;
  }
  return WriteASY();
}

bool SObject::WriteASY() {
  if (numPorts < 1) {
    wxString mess = wxString::Format(
        _("%s:%d No data. Please open SnP file and make LIB first."), __FILE__,
        __LINE__);
    if (be_quiet) {
      cout << mess << endl;
    } else {
      wxLogError(mess);
    }
    return false;
  }

  list<string> sym;

  sym = Symbol(asy_file.GetName().ToStdString());

  if (sym.empty()) {
    wxString mess = wxString::Format(_("%s:%d Error creating symbol '%s'."),
                                     __FILE__, __LINE__, asy_file.GetName());
    if (be_quiet) {
      cout << mess << endl;
    } else {
      wxLogError(mess);
    }
    return false;
  }

  string symName(asy_file.GetFullPath().ToStdString());
  ofstream output_stream(symName);
  if (!output_stream) {
    wxString mess = wxString::Format(_("%s:%d Cannot create file '%s'."),
                                     __FILE__, __LINE__, symName);
    if (be_quiet) {
      cout << mess << endl;
    } else {
      wxLogError(mess);
    }
    return false;
  }
  for (auto i = sym.begin(); i != sym.end(); i++) {
    output_stream << *i << "\n";
  }
  return !error;
}

bool SObject::Convert2S() {
  vector<double> raw_data;
  // Since we know the number of ports we can know the amount
  // of data that should be in the data section.  So we tokenize
  // the numbers and then make sure we get exactly the right #.
  {
    int warning = 0;
    istringstream iss(data_strings);
    vector<string> tokens{istream_iterator<string>{iss},
                          istream_iterator<string>{}};
    data_strings.clear();
    for (auto i = tokens.begin(); i != tokens.end(); i++) {
      try {
        raw_data.push_back(stod(*i));
      } catch (std::invalid_argument const& ex) {
        warning++;
      }
    }
    if (warning > 0) {
      wxString mess = wxString::Format(
          "%s:%d WARNING: %s contains invalid non-numeric characters", __FILE__,
          __LINE__, snp_file.GetFullPath());
      if (be_quiet) {
        cout << mess << endl;
      } else {
        wxLogWarning(mess);
      }
    }
  }

  int nFreqs = raw_data.size() / (numPorts * numPorts * 2 + 1);
  if (nFreqs * (numPorts * numPorts * 2 + 1) != raw_data.size()) {
    wxString mess =
        wxString::Format(_("%s:%d ERROR: %s contains wrong number of values"),
                         __FILE__, __LINE__, snp_file.GetFullPath());
    if (be_quiet) {
      cout << mess << endl;
    } else {
      wxLogError(mess);
    }
    return false;
  }

  Sparam S(numPorts);
  double prevFreq = 0;
  for (auto rd = raw_data.begin(); rd != raw_data.end();) {
    size_t numPortsSqd = numPorts * numPorts;
    S.Freq = fUnits * *rd++;
    // frequencies must be monotonically increasing
    if (S.Freq < prevFreq) {
      wxString mess = wxString::Format(
          _("%s:%d ERROR: %s contains decreasing frequency values"), __FILE__,
          __LINE__, snp_file.GetFullPath());
      if (be_quiet) {
        cout << mess << endl;
      } else {
        wxLogError(mess);
      }
      return false;
    }
    prevFreq = S.Freq;

    // Step 1: Convert data from input specified type to internal dB/phase deg
    if (inputFormat.compare("MAG") == 0) {
      for (size_t i = 0; i < numPorts; i++) {
        for (size_t j = 0; j < numPorts; j++) {
          // convert raw mag to dB
          S.dB(i, j) = (20.0 * log10(*rd++));
          // copy the phase in degrees
          S.Phase(i, j) = *rd++;
        }
      }
    } else if (inputFormat.compare("R_I") == 0) {
      // Move data into complex matrix ri
      MatrixXcd ri(numPorts, numPorts);
      for (size_t i = 0; i < numPorts; i++) {
        for (size_t j = 0; j < numPorts; j++) {
          double a = *rd++;
          double b = *rd++;
          ri(i, j) = dcomplex(a, b);
        }
      }
      // Extract dB from matrix ri
      S.dB = 20.0 * log10(abs(ri.array()));
      // Extract phase in degrees from ri
      S.Phase = (180 / M_PI ) * ri.cwiseArg();
    } else if (inputFormat.compare("DB") == 0) {
      // input == internal form so just copy each value
      for (size_t i = 0; i < numPorts; i++) {
        for (size_t j = 0; j < numPorts; j++) {
          S.dB(i, j) = *rd++;
          S.Phase(i, j) = *rd++;
        }
      }
    } else {
      wxString mess =
          wxString::Format(_("%s:%d Data format '%s' unsporrted in file '%s'."), __FILE__,
                           __LINE__, wxString(inputFormat), snp_file.GetFullPath());
      if (be_quiet) {
        cout << mess << endl;
      } else {
        wxLogError(mess);
      }
      return false;
    }

    // Step 2: Convert from input parameter type H to S
    if (parameterType.compare("H") == 0) {
      auto H = S.Scplx();
      // convert H to S-parameters
      MatrixXcd Slocal = h2s(H, Z0, 1 / Z0);
      S.cplxStore(Slocal);
      parameterType = "S";
    }
    // Step 3: Fixup 2-port data locations
    //         Touchstone treats 2-ports uniquely
    if (numPorts == 2) {
      std::swap(S.dB(0, 1), S.dB(1, 0));
      std::swap(S.Phase(0, 1), S.Phase(1, 0));
    }
    SData.push_back(S);
  }
  return !error;
}

MatrixXcd SObject::h2s(const MatrixXcd& H, double Z0, double Y0) const {
  int n = H.rows();  // get the number of ports
  // create the identity matrix
  MatrixXcd I(H.Identity(n, n));
  // calculate S-parameters
  MatrixXcd S = (Z0 / Y0) * (I + H) * (I - H).inverse();
  return S;
}

list<string> SObject::Symbol2port(const string& symname) const {
  list<string> symbol;
  symbol.push_back("Version 4");
  symbol.push_back("SymbolType BLOCK");
  symbol.push_back("RECTANGLE Normal 48 -32 -48 32");
  symbol.push_back("TEXT 0 -48 Center 2 " + symname);
  symbol.push_back("SYMATTR Prefix X");
  symbol.push_back("SYMATTR SpiceModel " + symname);
  symbol.push_back("SYMATTR ModelFile " + symname + ".lib");
  symbol.push_back("PIN -48 0 LEFT 8");
  symbol.push_back("PINATTR PinName 1");
  symbol.push_back("PINATTR SpiceOrder 1");
  symbol.push_back("PIN 48 0 RIGHT 8");
  symbol.push_back("PINATTR PinName 2");
  symbol.push_back("PINATTR SpiceOrder 2");
  symbol.push_back("PIN 0 32 BOTTOM 8");
  symbol.push_back("PINATTR PinName 3");
  symbol.push_back("PINATTR SpiceOrder 3");
  return symbol;
}

list<string> SObject::Symbol1port(const string& symname) const {
  list<string> symbol;
  symbol.push_back("Version 4");
  symbol.push_back("SymbolType BLOCK");
  symbol.push_back("RECTANGLE Normal 48 -32 -48 32");
  symbol.push_back("TEXT 0 -48 Center 2 " + symname);
  symbol.push_back("SYMATTR Prefix X");
  symbol.push_back("SYMATTR SpiceModel " + symname);
  symbol.push_back("SYMATTR ModelFile " + symname + ".lib");
  symbol.push_back("PIN -48 0 LEFT 8");
  symbol.push_back("PINATTR PinName 1");
  symbol.push_back("PINATTR SpiceOrder 1");
  symbol.push_back("PIN 0 32 BOTTOM 8");
  symbol.push_back("PINATTR PinName 2");
  symbol.push_back("PINATTR SpiceOrder 2");
  return symbol;
}

list<string> SObject::Symbol(const string& symname) const {
  list<string> symbol;
  switch (numPorts) {
    case 1:
      symbol = Symbol1port(symname);
      break;
    case 2:
      symbol = Symbol2port(symname);
      break;
    default:
      vector<int> pinsLeft, pinsRight;
      symbol.push_back("Version 4");
      symbol.push_back("SymbolType BLOCK");
      for (int i = 0; i < numPorts; i++) {
        if (!(i % 2) && pinsLeft.size() < numPorts / 2)
          pinsLeft.push_back(i + 1);
        else
          pinsRight.push_back(i + 1);
      }
      int symWidth = 96;
      if (numPorts > 10) symWidth = 128;
      int symHeight = max(pinsLeft.size() * 32, pinsRight.size() * 32);
      int xur = symWidth / 2;
      int yur = -32;
      int yll = yur + symHeight;
      int xll = xur - symWidth;
      stringstream ss;
      ss << "RECTANGLE Normal " << xll << " " << yll << " " << xur << " "
         << yur;
      symbol.push_back(ss.str());
      ss.str(std::string());
      symbol.push_back("TEXT 0 -48 Center 2 " + symname);
      symbol.push_back("SYMATTR Prefix X");
      symbol.push_back("SYMATTR SpiceModel " + symname);
      symbol.push_back("SYMATTR ModelFile " + symname + ".lib");
      // Do the left pins
      int yPin;
      int pinName = -1;
      if (pinsLeft.size() % 2) {
        yPin = 0;
      } else {
        yPin = -16;
      }
      for (auto i : pinsLeft) {
        ss << "PIN " << xll << " " << yPin << " LEFT 8";
        symbol.push_back(ss.str());
        ss.str(std::string());
        yPin += 32;
        ss << "PINATTR PinName " << i;
        symbol.push_back(ss.str());
        ss.str(std::string());
        ss << "PINATTR SpiceOrder " << i;
        symbol.push_back(ss.str());
        ss.str(std::string());
      }
      if (pinsRight.size() % 2) {
        yPin = 0;
      } else {
        yPin = -16;
      }
      pinName = 0;
      for (auto i : pinsRight) {
        ss << "PIN " << xur << " " << yPin << " RIGHT 8";
        symbol.push_back(ss.str());
        ss.str(std::string());
        yPin += 32;
        ss << "PINATTR PinName " << i;
        symbol.push_back(ss.str());
        ss.str(std::string());
        ss << "PINATTR SpiceOrder " << i;
        symbol.push_back(ss.str());
        ss.str(std::string());
      }
      ss << "PIN 0 " << yll << " Bottom 8";
      symbol.push_back(ss.str());
      ss.str(std::string());
      ss << "PINATTR PinName " << numPorts + 1;
      symbol.push_back(ss.str());
      ss.str(std::string());
      ss << "PINATTR SpiceOrder " << numPorts + 1;
      symbol.push_back(ss.str());
      ss.str(std::string());
      break;
  }
  return symbol;
}
