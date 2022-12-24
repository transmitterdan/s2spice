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
#include <string>
#include <utility>

#include "SObject.h"

using namespace std;

Sparam::Sparam(std::size_t _n) {
  Freq = 0.0;
  dB.set_row_count(_n);
  dB.set_col_count(_n);
  Phase.set_row_count(_n);
  Phase.set_col_count(_n);
}

SObject::SObject() {
  data_saved = true;
  numPorts = 0;
  fUnits = 0;
  Z0 = 50;
}

SObject::~SObject() {
  SData.clear();
  comment_strings.clear();

  wxObject::~wxObject();
}

bool SObject::openSFile(wxWindow* parent) {
#if defined(_WIN32) || defined(_WIN64)
  char const* WildcardStr = "S paramter (*.snp)|*.s?p|All files (*.*)|*.*";
#else
  // On non-Windows platforms we try to find mostly snp files but
  // they don't all allow ? as a wildcard
  char const* WildcardStr = "S paramter (*p)|*p;*P|All files (*)|*";
#endif
  if (!dataSaved()) {
    if (wxMessageBox(_("Current content has not been saved! Proceed?"),
                     _("Please confirm"), wxICON_QUESTION | wxYES_NO,
                     parent) == wxNO)
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

bool SObject::readSFile(wxFileName& fileName) {

  SData.clear();
  wxString cwd = wxGetCwd();
  snp_file.Assign(fileName);
  wxFileInputStream input_stream(snp_file.GetFullPath());
  if (!input_stream.IsOk()) {
    wxLogError(_("%s:%d Cannot open file '%s'."), __FILE__, __LINE__, snp_file.GetFullPath());
    return false;
  }
  wxString N(snp_file.GetExt().Mid(1, 1));
  if (N.IsNumber()) {
    numPorts = atoi(N.ToAscii());
    if (numPorts < 0) numPorts = 0;
  } else
    numPorts = 0;
  if (numPorts < 1) {
    wxLogError(_("%s:%d SOjbect::readSFile:Cannot read file '%s'."), __FILE__, __LINE__, snp_file.GetFullPath());
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
  if (option_string.IsEmpty()) {
    format = "MA";  // default mag/angle
    fUnits = 1e9;
    parameter = "S";
    Z0 = 50;
  } else {
    wxArrayString options(
        wxStringTokenize(option_string, wxDEFAULT_DELIMITERS, wxTOKEN_DEFAULT));
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
        parameter = options[i];
      else if (options[i].Matches("Y"))
        parameter = options[i];
      else if (options[i].Matches("Z"))
        parameter = options[i];
      else if (options[i].Matches("H"))
        parameter = options[i];
      else if (options[i].Matches("G"))
        parameter = options[i];
      else if (options[i].Matches("DB"))
        format = options[i];
      else if (options[i].Matches("MA"))
        format = options[i];
      else if (options[i].Matches("RI"))
        format = options[i];
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
  return true;
}

bool SObject::writeLibFile(wxWindow* parent) {
  wxFileName libFile = snp_file;
  libFile.SetExt("lib");
  if (libFile.Exists()) {
    wxString mess = wxString::Format(_("Library file '%s' exists. Overwrite?"),
                                     libFile.GetFullPath());
    if (wxMessageBox(mess, _("Please confirm"), wxICON_QUESTION | wxYES_NO,
                     parent) == wxNO)
      return false;
  }
  return WriteLIB(libFile);
}

bool SObject::WriteLIB(const wxFileName& libFile) {
  lib_file = libFile;
  string libName(lib_file.GetFullPath().ToStdString());

  ofstream output_stream(libName);
  if (!output_stream) {
    wxLogError(_("%s:%d SOjbect::WriteLIB:Cannot create file '%s'."), __FILE__, __LINE__ , libName);
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
    output_stream << (wxString::Format("R%dN %d %d %e\n", i + 1, i + 1,
                                       10 * (i + 1), -Z0)
                          .c_str());
    output_stream << (wxString::Format("R%dP %d %d %e\n", i + 1, 10 * (i + 1),
                                       10 * (i + 1) + 1, 2 * Z0));
  }

  const int charMAX = 2048;
  output_stream << "\n";
  char* out = new char[charMAX + 1];
  for (int i = 0; i < numPorts; i++) {
    for (int j = 0; j < numPorts; j++) {
      snprintf(out, charMAX, "*S%d%d FREQ DB PHASE\n", i + 1, j + 1);
      output_stream << out;
      if (j + 1 == numPorts) {
        snprintf(out, charMAX, "E%d%d %d%d %d FREQ {V(%d,%d)}= DB\n", i + 1,
                 j + 1, i + 1, j + 1, numPorts + 1, 10 * (j + 1), numPorts + 1);
        output_stream << out;
      } else {
        snprintf(out, charMAX, "E%d%d %d%d %d%d FREQ {V(%d,%d)}= DB\n", i + 1,
                 j + 1, i + 1, j + 1, i + 1, j + 2, 10 * (j + 1), numPorts + 1);
        output_stream << out;
      }
      double offset = 0;
      double prevph = 0;
      for (auto& sparam : SData) {
        double phs = sparam.Phase(i, j);
        double dB = sparam.dB(i, j);
        if ((abs(phs - prevph)) > 180.0) {
          offset = offset - 360.0 * (double)signbit(prevph - phs);
        }
        prevph = phs;
        snprintf(out, charMAX, "+(%14eHz,%14e,%14e)\n", sparam.Freq, dB,
                 phs + offset);
        output_stream << out;
      }
      output_stream << "\n";
    }
  }
  delete[] out;
  output_stream << ".ENDS * " << lib_file.GetName() << "\n";
  data_saved = true;
  return true;
}

bool SObject::writeSymFile(wxWindow* parent) {
  wxFileName asyFile = snp_file;
  asyFile.SetExt("asy");
  if (asyFile.Exists()) {
    wxString mess = wxString::Format(_("Symbol file '%s' exists. Overwrite?"),
                                     asyFile.GetFullPath());
    if (wxMessageBox(mess, _("Please confirm"), wxICON_QUESTION | wxYES_NO,
                     parent) == wxNO)
      return false;
  }
  return WriteASY(asyFile);
}

bool SObject::WriteASY(const wxFileName& asyFile) {
  asy_file = asyFile;
  if (numPorts < 1) {
    wxString mess = wxString::Format(
        _("%s:%d No data. Please open SnP file and make LIB first."),__FILE__, __LINE__);
    wxLogError(mess);
    return false;
  }

  vector<string> sym;

  sym = Symbol(asyFile.GetName().ToStdString());

  if (sym.empty()) {
    wxLogError(_("%s:%d Error creating symbol '%s'."), __FILE__, __LINE__, asyFile.GetName());
    return false;
  }

  string symName(asyFile.GetFullPath().ToStdString());
  ofstream output_stream(symName);
  if (!output_stream) {
    wxLogError(_("%s:%d Cannot create file '%s'."), __FILE__, __LINE__, symName);
    return false;
  }
  for (auto i = sym.begin(); i != sym.end(); i++) {
    output_stream << *i << "\n";
  }
  return true;
}

bool SObject::Convert2S() {
  Sparam S(numPorts);
  // Since we know the number of ports we can know the amount
  // of data that should be in the data section.  So we tokenize
  // the numbers and then make sure we get exactly the right #.
  istringstream iss(data_strings);
  vector<string> tokens{istream_iterator<string>{iss},
                        istream_iterator<string>{}};
  data_strings.clear();
  int nFreqs = tokens.size() / (numPorts * numPorts * 2 + 1);
  if (nFreqs * (numPorts * numPorts * 2 + 1) != tokens.size()) return false;
  SData.clear();

  auto i = tokens.begin();
  while (i != tokens.end()) {
    S.Freq = fUnits * atof(i++->c_str());
    if (format == "MA") {
      for (auto j = 0; j < numPorts; j++) {
        for (auto k = 0; k < numPorts; k++) {
          double mag, angle;
          mag = stod(*i++);
          angle = stod(*i++);
          S.dB(j, k) = 20.0 * log10(mag);
          S.Phase(j, k) = angle;
        }
      }
    } else if (format == "DB") {
      for (auto j = 0; j < numPorts; j++) {
        for (auto k = 0; k < numPorts; k++) {
          double dB, angle;
          dB = stod(*i++);
          angle = stod(*i++);
          S.dB(j, k) = dB;
          S.Phase(j, k) = angle;
        }
      }
    } else if (format == "RI") {
      for (auto j = 0; j < numPorts; j++) {
        for (auto k = 0; k < numPorts; k++) {
          double re, im;
          re = stod(*i++);
          im = stod(*i++);
          S.dB(j, k) = 20 * log10(sqrt(re * re + im * im));
          S.Phase(j, k) = atan2(im, re);
        }
      }
    } else {
      wxLogError(wxString::Format("%s:%d Cannot read file '%s'."),__FILE__,__LINE__,
                 snp_file.GetFullPath());
      return false;
    }
    if (numPorts == 2) {
      swap(S.dB(0, 1), S.dB(1, 0));
      swap(S.Phase(0, 1), S.Phase(1, 0));
    }
    SData.push_back(S);
  }
  tokens.clear();
  return true;
}

vector<string> SObject::Symbol2port(const string& symname) const {
  vector<string> symbol;
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

vector<string> SObject::Symbol1port(const string& symname) const {
  vector<string> symbol;
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

vector<string> SObject::Symbol(const string& symname) const {
  vector<string> symbol;
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
