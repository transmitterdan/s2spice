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

#define MAX_PORTS 99  // maximum number of ports we will handle

struct Sparam {
  double Freq;
  Eigen::Matrix<std::complex<double>, Eigen::Dynamic, Eigen::Dynamic> S;
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

private:
  std::string data_strings;       // String array of data from SnP file
  wxArrayString comment_strings;  // String array of comments from SnP file
  wxArrayString option_string;    // meta data strings
  bool data_saved;                // have we saved in imported S-parameter file
  int nPorts;           // number of ports in this file (comes from file name)
  wxFileName snp_file;  // file that is currently loaded
  double fUnits;        // frequency units
  double Z0;            // reference Z
  wxString format;      // data format (DB, MA or RI)
  wxString parameter;   // type of parameter (S is the only allowed type)
  std::vector<Sparam> SData;
  // This function is called when the "Open" button is clicked
  void OnOpen(wxCommandEvent& event);

  // This function is called when the "Read" button is clicked
  void OnMkLIB(wxCommandEvent& event);

  // This function is called when the "Plot" button is clicked
  void OnPlot(wxCommandEvent& event);

  // This function is called when the "Quit" button is clicked
  void OnQuit(wxCommandEvent& event);

  // This function reads the contents of the file into a vector of points
  std::vector<std::pair<double, double>> ReadFile(const std::string& fileName);

  // This function plots the given data in a new window
  void PlotData(const std::vector<std::pair<double, double>>& data);

  // Convert text to S-parameters
  void Convert2S();

  wxDECLARE_EVENT_TABLE();
  enum wxFrameID {
    // The ID of the "Open" button
    ID_OPEN = 1,

    // The ID of the "Read" button
    ID_MKLIB,

    // The ID of the "Plot" button
    ID_PLOT,

    // The ID of the "Quit" button
    ID_QUIT,

    ID_HELLO
  };
};
// This is the event table for the program
wxBEGIN_EVENT_TABLE(MyFrame, wxFrame) EVT_BUTTON(ID_OPEN, MyFrame::OnOpen)
    EVT_BUTTON(ID_MKLIB, MyFrame::OnMkLIB) EVT_BUTTON(ID_PLOT, MyFrame::OnPlot)
        EVT_BUTTON(ID_QUIT, MyFrame::OnQuit) wxEND_EVENT_TABLE()

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
  data_saved = true;

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

  // Create the "Plot" button
  wxButton* plotButton =
      new wxButton(this, ID_PLOT, "Plot", wxPoint(190, 10), wxSize(80, 30));

  // Create the "Quit" button
  wxButton* quitButton =
      new wxButton(this, ID_QUIT, "Quit", wxPoint(280, 10), wxSize(80, 30));
}

void MyFrame::OnMkLIB(wxCommandEvent& event) {
  //  wxMessageBox("Make LIB button pressed.");
  wxFileName lib_file = snp_file;
  lib_file.SetExt("lib");
  wxString libName(lib_file.GetFullPath());
  if (lib_file.Exists()) {
    wxString mess =
        wxString::Format(_("Lib file '%s' exists. Overwrite?"), libName);
    if (wxMessageBox(mess, _("Please confirm"), wxICON_QUESTION | wxYES_NO,
                     this) == wxNO)
      return;
  }

  wxBusyCursor wait;

  std::ofstream output_stream(libName.c_str());
  if (!output_stream) {
    wxLogError("Cannot create file '%s'.", libName);
    return;
  }
  output_stream << ".SUBCKT " << lib_file.GetName() << " ";
  for (int i = 0; i < nPorts + 1; i++) output_stream << " " << i + 1;
  output_stream << "\n";

  for (int i = 0; i < comment_strings.Count(); i++) {
    output_stream << "*" << comment_strings[i].Mid(1) << "\n";
  }
  output_stream << "*" << option_string[0].Mid(1) << "\n";
  output_stream << "*";

  for (int i = 0; i < nPorts; i++) {
    output_stream << " Z" << i + 1 << " = " << Z0;
  }
  output_stream << "\n";

  /* define resistances for Spice model */

  for (int i = 0; i < nPorts; i++) {
    output_stream << (wxString::Format("R%dN %d %d %e\n", i + 1, i + 1,
                                       10 * (i + 1), -Z0)
                          .c_str());
    output_stream << (wxString::Format("R%dP %d %d %e\n", i + 1, 10 * (i + 1),
                                       10 * (i + 1) + 1, 2 * Z0));
  }
  output_stream << "\n";
  char* out = new char[2049];
  for (int i = 0; i < nPorts; i++) {
    for (int j = 0; j < nPorts; j++) {
      snprintf(out, 2048, "*S%d%d FREQ DB PHASE\n", i + 1, j + 1);
      output_stream << out;
      if (j + 1 == nPorts) {
        snprintf(out, 2048, "E%d%d %d%d %d FREQ {V(%d,%d)}= DB\n", i + 1, j + 1,
                 i + 1, j + 1, nPorts + 1, 10 * (j + 1), nPorts + 1);
        output_stream << out;
      } else {
        snprintf(out, 2048, "E%d%d %d%d %d%d FREQ {V(%d,%d)}= DB\n", i + 1,
                 j + 1, i + 1, j + 1, i + 1, j + 2, 10 * (j + 1), nPorts + 1);
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
  SetStatusText(
      wxString::Format("S2spice: Data successfully written to %s.", libName));
  data_saved = true;
}

void MyFrame::OnPlot(wxCommandEvent& event) {
  wxMessageBox("Plot button pressed.");
}

void MyFrame::OnQuit(wxCommandEvent& event) {
  // shut down the main frame
  Close(true);
}

void MyFrame::OnOpen(wxCommandEvent& WXUNUSED(event)) {
  if (data_strings.empty() && !data_saved) {
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
    nPorts = atoi(N.ToAscii());
    if (nPorts < 0 || nPorts > MAX_PORTS) nPorts = 0;
  } else
    nPorts = 0;
  if (nPorts < 1) {
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
  using namespace std;
  Sparam S;
  istringstream iss(data_strings);
  vector<string> tokens{istream_iterator<string>{iss},
                        istream_iterator<string>{}};
  data_strings.clear();
  int nTokens = nPorts * nPorts * 2 + 1;
  int nFreqs = tokens.size();
  string Last = tokens[nFreqs - 1];
  string Next_Last = tokens[nFreqs - 2];
  nFreqs = nFreqs / nTokens;
  SData.clear();
  S.S.resize(nPorts, nPorts);

  auto i = tokens.begin();
  while (i != tokens.end()) {
    S.Freq = fUnits * atof(tokens.front().c_str());
    i = tokens.erase(i);
    for (auto j = 0; j < nPorts; j++) {
      for (auto k = 0; k < nPorts; k++) {
        std::complex<double> sValue;
        if (format == "MA") {
          double mag = std::stod(tokens.front());
          i = tokens.erase(i);
          double angle = std::stod(tokens.front());
          i = tokens.erase(i);
          sValue = std::complex(mag * cos(angle * M_PI / 180.0),
                                mag * sin(angle * M_PI / 180.0));
        } else if (format == "DB") {
          double mag = std::stod(tokens.front());
          i = tokens.erase(i);
          mag = pow(10.0, mag / 20.0);
          double angle = std::stod(tokens.front());
          i = tokens.erase(i);
          sValue = std::complex(mag * cos(angle * M_PI / 180.0),
                                mag * sin(angle * M_PI / 180.0));
        } else if (format == "RI") {
          double re = std::stod(tokens.front());
          i = tokens.erase(i);
          double im = std::stod(tokens.front());
          i = tokens.erase(i);
          sValue = std::complex(re, im);
        } else {
          wxLogError("Cannot read file '%s'.", snp_file.GetFullPath());
          return;
        }
        S.S(j, k) = sValue;
      }
    }
    if (nPorts == 2) swap(S.S(0, 1), S.S(1, 0));
    SData.push_back(S);
  }
}