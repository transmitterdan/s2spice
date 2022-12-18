/***************************************************************************
 *
 * Project:  S2spice
 * Purpose:  S2spice wxWidgets Program converts S-parameter files to Spice 
 *           subcircuit library file.
 * Author:   Dan Dickey
 * 
 * Base on: s2spice.c
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

#include <wx/wx.h>
#ifndef WX_PRECOMP
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>
#endif /* WX_PRECOMP */

// We use the Eigen template library for 2-D matrices
#include <Core>

// std libraries we use
#include <vector>
#include <sstream>
#include <complex>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <utility>

using namespace std;

#define MAX_PORTS 99  // maximum number of ports we will handle

struct Sparam {
  double Freq;
  Eigen::Matrix<complex<double>, Eigen::Dynamic, Eigen::Dynamic> S;
};

// This is the main class for the program
class MyApp : public wxApp {
public:
  virtual bool OnInit();
};

// This is the main event handler for the program
class MyFrame : public wxFrame {
public:
  MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
  int nFreq(void) { return SData.size(); }
  int nPorts(void) { return numPorts; }

private:
  string data_strings;       // String array of data from SnP file
  wxArrayString comment_strings;  // String array of comments from SnP file
  wxArrayString option_string;    // meta data strings
  bool data_saved;                // have we saved in imported S-parameter file
  int numPorts;           // number of ports in this file (comes from file name)
  wxFileName snp_file;  // file that is currently loaded
  double fUnits;        // frequency units
  double Z0;            // reference Z
  wxString format;      // data format (DB, MA or RI)
  wxString parameter;   // type of parameter (S is the only allowed type)
  vector<Sparam> SData;

  // This function is called when the "Open" button is clicked
  void OnOpen(wxCommandEvent& event);

  // This function is called when the "Read" button is clicked
  void OnMkLIB(wxCommandEvent& event);

  // This function is called when the "Symbol" button is clicked
  void OnMkASY(wxCommandEvent& event);

  // This function is called when the "Quit" button is clicked
  void OnQuit(wxCommandEvent& event);

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
  void Convert2S();

  wxDECLARE_EVENT_TABLE();
  enum wxFrameID {
    // The ID of the "Open" button
    ID_OPEN = 1,

    // The ID of the "LIB" button
    ID_MKLIB,

    // The ID of the "SYM" button
    ID_MKSYM,

    // The ID of the "Quit" button
    ID_QUIT,

    ID_HELLO
  };
};
// This is the event table for the program
wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
  EVT_BUTTON(ID_OPEN, MyFrame::OnOpen)
  EVT_BUTTON(ID_MKLIB, MyFrame::OnMkLIB)
  EVT_BUTTON(ID_MKSYM, MyFrame::OnMkASY)
  EVT_BUTTON(ID_QUIT, MyFrame::OnQuit)
wxEND_EVENT_TABLE()

    // This is the main entry point for the program
    wxIMPLEMENT_APP(MyApp);

// This is the implementation of the MyApp class
bool MyApp::OnInit() {
  // Create the main window
  MyFrame* frame = new MyFrame("S2spice", wxPoint(50, 50), wxSize(640, 480));
  frame->Show(true);

  return true;
}

// This is the implementation of the MyFrame class
MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
    : wxFrame(nullptr, wxID_ANY, title, pos, size) {
  data_saved = false;
  numPorts = 0;

  auto menuFile = new wxMenu();
  menuFile->Append(ID_HELLO, "&Hello...\tCtrl-H",
                   "Help string shown in status bar for this menu item");
  menuFile->AppendSeparator();
  menuFile->Append(wxID_EXIT);

  auto menuHelp = new wxMenu();
  menuHelp->Append(wxID_ABOUT);
  auto menuBar = new wxMenuBar();

  menuBar->Append(menuFile, "&File");
  menuBar->Append(menuHelp, "&Help");
  SetMenuBar(menuBar);

  CreateStatusBar();
  SetStatusText("S2spice: Convert Touchstone files to LTspice subcircuit.");

  menuBar->Bind(wxEVT_MENU, [&](wxCommandEvent& event) {
    if (event.GetId() == ID_HELLO)
      wxLogMessage("Hello world from S2Spice!");
    else if (event.GetId() == wxID_ABOUT)
      wxMessageBox(
          "Utility to convert Touchstone (aka SnP files) into LTspice "
          "subcircuit file",
          "About S2spice", wxOK | wxICON_INFORMATION);
    else if (event.GetId() == wxID_EXIT)
      Close(true);
    else
      event.Skip();
  });
  // Create the "Open" button
  wxButton* openButton =
      new wxButton(this, ID_OPEN, "Open", wxPoint(10, 10), wxSize(80, 30));

  // Create the "Read" button
  wxButton* libButton = new wxButton(this, ID_MKLIB, "Make LIB",
                                     wxPoint(100, 10), wxSize(80, 30));

  // Create the "Symbol" button
  wxButton* symButton =
      new wxButton(this, ID_MKSYM, "Make SYM", wxPoint(190, 10), wxSize(80, 30));

  // Create the "Quit" button
  wxButton* quitButton =
      new wxButton(this, ID_QUIT, "Quit", wxPoint(280, 10), wxSize(80, 30));
}


void MyFrame::OnQuit(wxCommandEvent& event) {
  // shut down the main frame
  Close(true);
}

void MyFrame::OnMkLIB(wxCommandEvent& event) {
  //  wxMessageBox("Make LIB button pressed.");

  wxBusyCursor wait;
  wxFileName libFile = snp_file;
  libFile.SetExt("lib");
  if (libFile.Exists()) {
    wxString mess = wxString::Format(_("Library file '%s' exists. Overwrite?"),
                                     libFile.GetFullPath());
    if (wxMessageBox(mess, _("Please confirm"), wxICON_QUESTION | wxYES_NO,
                     this) == wxNO)
      return;
  }
  bool res = WriteLIB(libFile);
  if (res)
    SetStatusText(wxString::Format(
        "S2spice: Library object %s successfully created.", snp_file.GetName()));
}

void MyFrame::OnMkASY(wxCommandEvent& event) {
  //  wxMessageBox("Symbol button pressed.");

  wxBusyCursor wait;
  wxFileName asyFile = snp_file;
  asyFile.SetExt("asy");
  if (asyFile.Exists()) {
    wxString mess = wxString::Format(_("Symbol file '%s' exists. Overwrite?"),
                                     asyFile.GetFullPath());
    if (wxMessageBox(mess, _("Please confirm"), wxICON_QUESTION | wxYES_NO,
                     this) == wxNO)
      return;
  }
  bool res = WriteASY(asyFile);
  if (res)
    SetStatusText(wxString::Format(
        "S2spice: Symbol object %s successfully created.", asyFile.GetName()));
}

void MyFrame::OnOpen(wxCommandEvent& WXUNUSED(event)) {
  if (!data_strings.empty() && !data_saved) {
    if (wxMessageBox(_("Current content has not been saved! Proceed?"),
                     _("Please confirm"), wxICON_QUESTION | wxYES_NO,
                     this) == wxNO)
      return;
  }
  wxFileDialog openFileDialog(this, _("Open SnP file"), "", "", "",
                              wxFD_OPEN | wxFD_FILE_MUST_EXIST);

  if (openFileDialog.ShowModal() == wxID_CANCEL)
    return;  // the user changed idea...

  wxBusyCursor wait;

  snp_file.Assign(openFileDialog.GetPath());
  wxFileInputStream input_stream(snp_file.GetFullPath());
  if (!input_stream.IsOk()) {
    wxLogError("Cannot open file '%s'.", snp_file.GetFullPath());
    return;
  }
  wxString N(snp_file.GetExt().Mid(1, 1));
  if (N.IsNumber()) {
    numPorts = atoi(N.ToAscii());
    if (numPorts < 0 || numPorts > MAX_PORTS) numPorts = 0;
  } else
    numPorts = 0;
  if (numPorts < 1) {
    wxLogError("Cannot read file '%s'.", snp_file.GetFullPath());
    return;
  }
  wxTextInputStream text_input(input_stream);
  wxString line;
  comment_strings.Empty();
  option_string.Empty();
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
      option_string.push_back(line.MakeUpper());
    else {
      data_strings.append(line);
      data_strings.append(" ");
    }
  }
  if (option_string.IsEmpty()) {
    format = "MA";  // default mag/angle
    fUnits = 1e9;
    parameter = "S";
    Z0 = 50;
  } else {
    wxArrayString options(wxStringTokenize(option_string[0]));
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

  Convert2S();
  data_saved = false;
  SetStatusText(wxString::Format("S2spice: Data successfully imported from %s.",
                                 snp_file.GetFullPath()));
}

void MyFrame::Convert2S() {
  Sparam S;
  istringstream iss(data_strings);
  vector<string> tokens{istream_iterator<string>{iss},
                        istream_iterator<string>{}};
  data_strings.clear();
  int nTokens = numPorts * numPorts * 2 + 1;
  int nFreqs = tokens.size();
  string Last = tokens[nFreqs - 1];
  string Next_Last = tokens[nFreqs - 2];
  nFreqs = nFreqs / nTokens;
  SData.clear();
  S.S.resize(numPorts, numPorts);

  auto i = tokens.begin();
  while (i != tokens.end()) {
    S.Freq = fUnits * atof(tokens.front().c_str());
    i = tokens.erase(i);
    for (auto j = 0; j < numPorts; j++) {
      for (auto k = 0; k < numPorts; k++) {
        complex<double> sValue;
        if (format == "MA") {
          double mag = stod(tokens.front());
          i = tokens.erase(i);
          double angle = stod(tokens.front());
          i = tokens.erase(i);
          sValue = complex(mag * cos(angle * M_PI / 180.0),
                                mag * sin(angle * M_PI / 180.0));
        } else if (format == "DB") {
          double mag = stod(tokens.front());
          i = tokens.erase(i);
          mag = pow(10.0, mag / 20.0);
          double angle = stod(tokens.front());
          i = tokens.erase(i);
          sValue = complex(mag * cos(angle * M_PI / 180.0),
                                mag * sin(angle * M_PI / 180.0));
        } else if (format == "RI") {
          double re = stod(tokens.front());
          i = tokens.erase(i);
          double im = stod(tokens.front());
          i = tokens.erase(i);
          sValue = complex(re, im);
        } else {
          wxLogError("Cannot read file '%s'.", snp_file.GetFullPath());
          return;
        }
        S.S(j, k) = sValue;
      }
    }
    if (numPorts == 2) swap(S.S(0, 1), S.S(1, 0));
    SData.push_back(S);
  }
}

vector<string> MyFrame::Symbol2port(const string& symname) const
{
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

vector<string> MyFrame::Symbol1port(const string& symname) const
{
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

vector<string> MyFrame::Symbol(const string& symname) const
{
  vector<string> symbol;
  switch (numPorts)
  {
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
      for (int i = 0; i < numPorts; i++)
      {
        if (!(i % 2) && pinsLeft.size() < numPorts/2)
          pinsLeft.push_back(i+1);
        else
          pinsRight.push_back(i+1);
      }
      int symWidth = 96;
      int symHeight = max ( pinsLeft.size() * 32, pinsRight.size() * 32 );
      int xur = symWidth/2;
      int yur = -32;
      int yll = yur + symHeight;
      int xll = xur - symWidth;
      stringstream ss;
      ss << "RECTANGLE Normal " << xll << " " << yll << " " << xur << " " << yur;
      symbol.push_back(ss.str()); ss.str(std::string());
      symbol.push_back("TEXT 0 -48 Center 2 " + symname);
      symbol.push_back("SYMATTR Prefix X");
      symbol.push_back("SYMATTR SpiceModel " + symname);
      symbol.push_back("SYMATTR ModelFile " + symname + ".lib");
      // Do the left pins
      int yPin;
      int pinName = -1;
      if (pinsLeft.size() % 2)
      {
        yPin = 0;
      } else
      {
        yPin = -16;
      }
      for ( auto i: pinsLeft)
      {
        ss << "PIN " << xll << " " << yPin << " LEFT 8";
        symbol.push_back(ss.str()); ss.str(std::string());
        yPin += 32;
        ss << "PINATTR PinName " << i;
        symbol.push_back(ss.str()); ss.str(std::string());
        ss << "PINATTR SpiceOrder " << i;
        symbol.push_back(ss.str()); ss.str(std::string());
      }
      if (pinsRight.size() % 2)
      {
        yPin = 0;
      } else
      {
        yPin = -16;
      }
      pinName = 0;
      for (auto i : pinsRight)
      {
        ss << "PIN " << xur << " " << yPin << " RIGHT 8";
        symbol.push_back(ss.str()); ss.str(std::string());
        yPin += 32;
        ss << "PINATTR PinName " << i;
        symbol.push_back(ss.str()); ss.str(std::string());
        ss << "PINATTR SpiceOrder " << i;
        symbol.push_back(ss.str()); ss.str(std::string());
      }
      ss << "PIN 0 " << yll << " Bottom 8";
      symbol.push_back(ss.str()); ss.str(std::string());
      ss << "PINATTR PinName " << numPorts + 1;
      symbol.push_back(ss.str()); ss.str(std::string());
      ss << "PINATTR SpiceOrder " << numPorts + 1;
      symbol.push_back(ss.str()); ss.str(std::string());
      break;
  }
  return symbol;
}

bool MyFrame::WriteLIB(const wxFileName& lib_file) {
  string libName(lib_file.GetFullPath().ToStdString());

  ofstream output_stream(libName);
  if (!output_stream) {
    wxLogError("Cannot create file '%s'.", libName);
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
  output_stream << "*" << option_string[0].Mid(1) << "\n";
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
  output_stream << "\n";
  char* out = new char[2049];
  for (int i = 0; i < numPorts; i++) {
    for (int j = 0; j < numPorts; j++) {
      snprintf(out, 2048, "*S%d%d FREQ DB PHASE\n", i + 1, j + 1);
      output_stream << out;
      if (j + 1 == numPorts) {
        snprintf(out, 2048, "E%d%d %d%d %d FREQ {V(%d,%d)}= DB\n", i + 1, j + 1,
                 i + 1, j + 1, numPorts + 1, 10 * (j + 1), numPorts + 1);
        output_stream << out;
      } else {
        snprintf(out, 2048, "E%d%d %d%d %d%d FREQ {V(%d,%d)}= DB\n", i + 1,
                 j + 1, i + 1, j + 1, i + 1, j + 2, 10 * (j + 1), numPorts + 1);
        output_stream << out;
      }
      double offset = 0;
      double prevph = 0;
      for (auto& sparam : SData) {
        double phs(atan2(sparam.S(i, j).imag(), sparam.S(i, j).real()));
        double mag(20.0 * log10(abs(sparam.S(i, j))));
        phs *= 180.0 / M_PI;

        if ((abs(phs - prevph)) > 180.0) {
          offset = offset - 360.0 * (double)signbit(prevph - phs);
        }
        prevph = phs;
        snprintf(out, 2048, "+(%14eHz,%14e,%14e)\n", sparam.Freq, mag,
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

bool MyFrame::WriteASY(const wxFileName& asyFile) {

  if (numPorts < 1) {
    wxString mess = wxString::Format(
        _("No data. Please open SnP file and make LIB first."));
    wxLogError(mess);
    return false;
  }

  vector<string> sym;

  sym = Symbol(asyFile.GetName().ToStdString());

  if (sym.empty()) {
    wxLogError("Error creating symbol '%s'.", asyFile.GetName());
    return false;
  }

  string symName(asyFile.GetFullPath().ToStdString());
  ofstream output_stream(symName);
  if (!output_stream) {
    wxLogError("Cannot create file '%s'.", symName);
    return false;
  }
  for (auto i : sym) {
    output_stream << i << "\n";
  }
  return true;
}
