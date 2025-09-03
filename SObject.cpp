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

#include "SObject.h"
#include "stringformat.hpp"
#include <wx/tokenzr.h>
#include <fstream>
#include <complex>
#include <algorithm>
#include <cctype>

SObject::SObject() {
  Clean();
  numPorts = 0;
  fUnits = 0;
  Z0 = 50;
  be_quiet = false;
  error = false;
  // Assume V1.0 until we see otherwise
  Swap = true;
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
  char const* WildcardStr =
      "Touchstone S|*.S?P;*.S??P;*.TS|H paramter (*.hnp)|*.H?P;H??P|All files "
      "(*.*)|*.*";
#else
  // On non-Windows platforms we try to find mostly snp files but
  // they don't all allow ? as a wildcard
  char const* WildcardStr = "Touchstone S|*p;*P;*ts;*TS|All files (*)|*";
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

  InitTargetsAndDefaults(SFile);

  if (!snp_file.FileExists()) {
    wxString mess = wxString::Format(_("[%s:%d]\nFile '%s' does not exist.\n"
                                       "Current working directory: '%s'"),
                                     __FILE__, __LINE__, snp_file.GetFullPath(),
                                     wxGetCwd());
    return HandleMessage(mess, be_quiet);
  }

  bool v2 = false;
  if (!DeterminePortsAndVersionFromExt(v2)) {
    return false;
  }

  {
    wxFileInputStream input_stream(snp_file.GetFullPath());
    if (!input_stream.IsOk()) {
      wxString mess =
          wxString::Format(_("%s:%d Cannot open file '%s'."), __FILE__,
                           __LINE__, snp_file.GetFullPath());
      return HandleMessage(mess, be_quiet);
    }
    wxTextInputStream text_input(input_stream);
    if (!ParseTouchstone(text_input, v2)) {
      return false;
    }
  }

  if (!ParseOptionsFromHeader()) {
    return false;
  }
  if (!ValidateAfterParse()) {
    return false;
  }

  if (Convert2S()) {
    data_saved = true;
    data_strings.clear();
    return true;
  } else {
    data_saved = false;
    return false;
  }
}

void SObject::InitTargetsAndDefaults(const wxFileName& SFile) {
  lib_file = SFile;
  asy_file = SFile;
  string lib_name = SFile.GetName().ToStdString();
  replace_if(lib_name.begin(), lib_name.end(), ::isspace, '_'); // keep original behavior
  lib_file.SetName(lib_name);
  lib_file.SetExt("inc");
  asy_file.SetName(lib_name);
  asy_file.SetExt("asy");

  // Defaults (match original)
  inputFormat = "MAG";   // default mag/angle
  fUnits = 1e9;          // default GHz
  parameterType = "S";
  numPorts = 2;          // default to 2 ports (may be overridden)
  Z0 = 50;
  Ver = 1.0;             // Assume version 1.0 until found otherwise
  Swap = true;           // 2-port swap default for V1
  comment_strings.Empty();
  option_string.Clear();
  data_strings.clear();
  error = false;
}

bool SObject::DeterminePortsAndVersionFromExt(bool& v2) {
  string ext = snp_file.GetExt().ToStdString();
  if (ext == "ts" || ext == "TS") {
    v2 = true;
    return true;
  }
  // V1.x: infer port count from digits at start of extension (e.g., s2p, s4p)
  string strPorts;
  for (auto ch : ext) {
    if (isdigit(static_cast<unsigned char>(ch))) strPorts += ch;
    if (!strPorts.empty() && !isdigit(static_cast<unsigned char>(ch))) break;
  }
  if (!strPorts.empty()) {
    try {
      numPorts = stoi(strPorts);
      v2 = false;
      return true;
    } catch (...) {
      // fall through to error
    }
  }
  wxString mess =
      wxString::Format(_("%s:%d SObject::readSFile:Cannot read file '%s'."),
                       __FILE__, __LINE__, snp_file.GetFullPath());
  return HandleMessage(mess, be_quiet);
}

bool SObject::ParseTouchstone(wxTextInputStream& text_input, bool v2) {
  bool Trigger = false;
  // re-init containers just in case
  comment_strings.Empty();
  option_string.Clear();
  data_strings.clear();

  wxString line;
  while (!text_input.GetInputStream().Eof()) {
    line.Empty();
    line = text_input.ReadLine();
    line.Trim();
    line.Trim(wxFalse);

    if (line.StartsWith("!") || line.StartsWith(";") || line.StartsWith("*")) {
      comment_strings.push_back(line);
      continue;
    }
    if (line.StartsWith("#")) {
      option_string = line.MakeUpper();
      if (Ver < 2.0) Trigger = true;
      continue;
    }
    if (line.StartsWith("[Version]")) {
      line.AfterFirst(']').Trim().ToDouble(&Ver);
      continue;
    }
    if (line.StartsWith("[Number of Ports]")) {
      line.AfterFirst(']').Trim().ToInt(&numPorts);
      continue;
    }
    if (line.StartsWith("[Number of Frequencies]")) {
      line.AfterFirst(']').Trim().ToInt(&numFreq);
      continue;
    }
    if (line.StartsWith("[Network Data]")) {
      if (Ver >= 2.0) {
        Trigger = true;
      }
      continue;
    }
    if (line.StartsWith("[Noise Data]")) {
      if (Ver >= 2.0) {
        Trigger = false; // stop at noise in V2
      }
      continue;
    }
    if (line.StartsWith("[End]")) {
      if (Ver >= 2.0) {
        Trigger = false; // stop at end in V2
      }
      continue;
    }
    if (line.StartsWith("[Number of Noise Frequencies]")) {
      continue; // ignore
    }
    if (line.StartsWith("[Reference]")) {
      wxArrayString references(wxStringTokenize(
          line.AfterFirst(']'), wxDEFAULT_DELIMITERS, wxTOKEN_DEFAULT));
      if (references.GetCount() < 1) {
        wxString mess =
            wxString::Format(_("%s:%d SObject::readSFile:Cannot process file "
                               "'%s'. [Reference] Wrong number of ports"),
                             __FILE__, __LINE__, snp_file.GetFullPath());
        return HandleMessage(mess, be_quiet);
      }
      double Zref;
      bool ok = references[0].ToDouble(&Zref);
      if (!ok) {
        wxString mess = wxString::Format(
            _("%s:%d SObject::readSFile:Cannot process file "
              "'%s'. [%s] Not a number"),
            __FILE__, __LINE__, snp_file.GetFullPath(), references[0]);
        return HandleMessage(mess, be_quiet);
      }
      Ref = std::vector<double>(numPorts, Zref);
      if (references.GetCount() == numPorts) {
        for (size_t i = 0; i < references.GetCount(); i++) {
          ok = references[i].ToDouble(&Zref);
          if (!ok) {
            wxString mess = wxString::Format(
                _("%s:%d SObject::readSFile:Cannot process file "
                  "'%s'. [%s] Not a number"),
                __FILE__, __LINE__, snp_file.GetFullPath(), references[i]);
            return HandleMessage(mess, be_quiet);
          }
          Ref[i] = Zref;
        }
      }
      continue;
    }
    if (line.StartsWith("[Two-Port Data Order]")) {
      numPorts = 2;
      if (line.AfterFirst(']').Trim().Trim(wxFalse).StartsWith("12_21")) {
        Swap = false; // Do not swap S21 and S12
      }
      continue;
    }
    if (line.StartsWith("[Matrix Format]")) {
      wxString matrix_format_str = line.AfterFirst(']').Trim().Trim(wxFalse);
      if (!(matrix_format_str.StartsWith("Full"))) {
        wxString mess =
            wxString::Format(_("%s:%d SObject::readSFile:Cannot process file "
                               "'%s'. [Matrix Format] Unknown"),
                             __FILE__, __LINE__, snp_file.GetFullPath());
        return HandleMessage(mess, be_quiet);
      }
      continue;
    }
    if (line.StartsWith("[Mixed Mode Order]")) {
      wxString mess =
          wxString::Format(_("%s:%d SObject::readSFile:Cannot Process file "
                             "'%s'.[Mixed Mode Order] Not supported"),
                           __FILE__, __LINE__, snp_file.GetFullPath());
      return HandleMessage(mess, be_quiet);
    }
    if (Trigger) {
      data_strings.append(line.ToStdString());
      data_strings.append(" ");
    }
  }

  if (data_strings.length() < 2) {
    wxString mess = wxString::Format(
        _("%s:%d SObject::readSFile:Cannot process file '%s'."), __FILE__,
        __LINE__, snp_file.GetFullPath());
    return HandleMessage(mess, be_quiet);
  }
  return true;
}

bool SObject::ParseOptionsFromHeader() {
  wxArrayString options(
      wxStringTokenize(option_string, wxDEFAULT_DELIMITERS, wxTOKEN_DEFAULT));
  for (size_t i = 1; i < options.GetCount(); i++) {
    if (options[i].Matches("GHZ"))
      fUnits = 1e9;
    else if (options[i].Matches("MHZ"))
      fUnits = 1e6;
    else if (options[i].Matches("KHZ"))
      fUnits = 1e3;
    else if (options[i].Matches("HZ"))
      fUnits = 1;
    else if (options[i].Matches("S") || options[i].Matches("Y") ||
             options[i].Matches("Z") || options[i].Matches("H") ||
             options[i].Matches("G"))
      parameterType = options[i].ToStdString();
    else if (options[i].StartsWith("DB"))
      inputFormat = "DB";
    else if (options[i].StartsWith("MA"))
      inputFormat = "MAG";
    else if (options[i].StartsWith("RI"))
      inputFormat = "R_I";
    else if (options[i].Matches("R")) {
      // guard against out-of-range
      if (i + 1 < options.GetCount()) {
        options[i + 1].ToDouble(&Z0);
      }
    }
  }
  return true;
}

bool SObject::ValidateAfterParse() const {
  if (numPorts < 1 || numPorts > 90) {
    wxString mess =
        wxString::Format(_("%s:%d SObject::readSFile:Cannot read file '%s'."),
                         __FILE__, __LINE__, snp_file.GetFullPath());
    HandleMessage(mess, be_quiet);
  }
  return true;
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
        _("%s:%d SObject::WriteLIB:Cannot handle %s format data file."),
        __FILE__, __LINE__, wxString(parameterType));
    error = true;
    HandleMessage(mess, be_quiet);
  }
}

bool SObject::WriteLIB() {
  string libName(lib_file.GetFullPath().ToStdString());
  int npMult = 100;
  if (parameterType.compare("S") != 0) {
    wxString mess = wxString::Format(
        _("%s:%d SObject::WriteLIB:Cannot handle %s format data file."),
        __FILE__, __LINE__, wxString(parameterType));
    return HandleMessage(mess, be_quiet);
  }

  ofstream output_stream(libName);
  if (!output_stream) {
    wxString mess =
        wxString::Format(_("%s:%d SObject::WriteLIB:Cannot create file '%s'."),
                         __FILE__, __LINE__, libName);
    return HandleMessage(mess, be_quiet);
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
                                  numPorts + 1, 2 * Z0);
  }

  output_stream << "\n";
  for (int i = 0; i < numPorts; i++) {
    for (int j = 0; j < numPorts; j++) {
      output_stream << stringFormat("* S%d%d FREQ %s\n ", i + 1, j + 1,
                                    inputFormat);
      output_stream << stringFormat(
          "G%02d%02d %d %d FREQ {V(%d,%d)}= %s\n", i + 1, j + 1, numPorts + 1,
          npMult * (i + 1), npMult * (j + 1), numPorts + 1, inputFormat);
      for (auto s = SData.begin(); s != SData.end(); s++) {
        double A = s->dB(i, j);
        A = A - 20 * log10(2 * Z0);
        double B = s->Phase(i, j);
        Convert2Input(A, B);
        output_stream << stringFormat("+(%14eHz,%14e,%14e)\n", s->Freq, A, B);
      }
    }
    output_stream << "\n";
  }

  output_stream << ".ENDS ; " << lib_file.GetName() << "\n";
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
    return HandleMessage(mess, be_quiet);
  }

  list<string> sym;

  sym = Symbol(asy_file.GetName().ToStdString());

  if (sym.empty()) {
    wxString mess = wxString::Format(_("%s:%d Error creating symbol '%s'."),
                                     __FILE__, __LINE__, asy_file.GetName());
    return HandleMessage(mess, be_quiet);
  }

  string symName(asy_file.GetFullPath().ToStdString());
  ofstream output_stream(symName);
  if (!output_stream) {
    wxString mess = wxString::Format(_("%s:%d Cannot create file '%s'."),
                                     __FILE__, __LINE__, symName);
    return HandleMessage(mess, be_quiet);
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
      return HandleMessage(mess, be_quiet);
    }
  }

  int nFreqs = raw_data.size() / (numPorts * numPorts * 2 + 1);
  if ((nFreqs * (numPorts * numPorts * 2 + 1) != raw_data.size()) ||
      ((nFreqs != numFreq) && (Ver >= 20))) {
    // Maybe the file has an incomplete last frequency.  If not, it will reveal
    // itself later on because frequency will be non-monotonic and then fail
    wxString mess =
        wxString::Format(_("%s:%d WARNING: %s contains wrong number of values"),
                         __FILE__, __LINE__, snp_file.GetFullPath());
    return HandleMessage(mess, be_quiet);
  }

  Sparam S((size_t)numPorts);
  double prevFreq = 0;
  int nFrequencies = 0;
  for (auto rd = raw_data.begin(); rd != raw_data.end();) {
    if (++nFrequencies > nFreqs) break;
    S.Freq = fUnits * *rd++;
    // frequencies must be monotonically increasing
    if (S.Freq < prevFreq) {
      wxString mess = wxString::Format(
          _("%s:%d ERROR: %s contains decreasing frequency values"), __FILE__,
          __LINE__, snp_file.GetFullPath());
      return HandleMessage(mess, be_quiet);
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
      S.Phase = (180 / M_PI) * ri.cwiseArg();
    } else if (inputFormat.compare("DB") == 0) {
      // input == internal form so just copy each value
      for (size_t i = 0; i < numPorts; i++) {
        for (size_t j = 0; j < numPorts; j++) {
          S.dB(i, j) = *rd++;
          S.Phase(i, j) = *rd++;
        }
      }
    } else {
      wxString mess = wxString::Format(
          _("%s:%d Data format '%s' unsupported in file '%s'."), __FILE__,
          __LINE__, wxString(inputFormat), snp_file.GetFullPath());
      return HandleMessage(mess, be_quiet);
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
    //         Touchstone V1.0 treats 2-ports uniquely
    if (numPorts == 2 && Swap) {
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
  symbol.push_back("SYMATTR ModelFile " + symname + ".inc");
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
  symbol.push_back("SYMATTR ModelFile " + symname + ".inc");
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
      symbol.push_back("SYMATTR ModelFile " + symname + ".inc");
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
