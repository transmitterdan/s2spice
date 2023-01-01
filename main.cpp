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

// Enable leak detection under windows
// For Linux use valgrind or other leak detection tool
#if !defined(NDEBUG) && defined(__WINDOWS__)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif /* WX_PRECOMP */
#include <wx/app.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>
#include <wx/cmdline.h>
#include <wx/sizer.h>
#include <wx/event.h>

#include "SObject.h"

using namespace std;

// std libraries we use
#include <stdio.h>
#include <vector>
#include <sstream>
#include <complex>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <utility>
#include <assert.h>

#include "xqsmatrix.h"

// This is the main class for the program
class MyApp : public wxApp {
public:
  virtual bool OnInit();
  virtual void OnInitCmdLine(wxCmdLineParser& parser);
  virtual bool OnCmdLineParsed(wxCmdLineParser& parser);
  virtual int OnExit();
  virtual int OnRun();

private:
  bool silent_mode = 0;
  bool force_mode = 0;
};

// This is the main event handler for the program
class MyFrame : public wxFrame {
public:
  MyFrame(const wxString& title, const bool force_mode, const wxPoint& pos, const wxSize& size);

private:
  SObject SData;
  bool debugFlag;
  wxStreamToTextRedirector* debug_redirector;
  // This function is called when the "Open" button is clicked
  void OnOpen(wxCommandEvent& event);

  // This function is called when Frame is trying to close
  void OnClose(wxCloseEvent& event);

  // This function is called when the "Read" button is clicked
  void OnMkLIB(wxCommandEvent& event);

  // This function is called when the "Symbol" button is clicked
  void OnMkASY(wxCommandEvent& event);

  // This function is called when the "Quit" button is clicked
  void OnQuit(wxCommandEvent& event);

  void OnAbout(wxCommandEvent& event);

  wxDECLARE_EVENT_TABLE();
  enum wxMyFrameID {
    // The ID of the "Open" button
    ID_OPEN = 1,
    ID_CLOSE,

    // The ID of the "LIB" button
    ID_MKLIB,

    // The ID of the "SYM" button
    ID_MKSYM,

  };
};

// This is the event table for the GUI
wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
  EVT_BUTTON(ID_OPEN,  MyFrame::OnOpen)
  EVT_BUTTON(ID_MKLIB, MyFrame::OnMkLIB)
  EVT_BUTTON(ID_MKSYM, MyFrame::OnMkASY)
  EVT_BUTTON(ID_CLOSE,  MyFrame::OnQuit)
  EVT_CLOSE(MyFrame::OnClose)
wxEND_EVENT_TABLE()

// This is the main entry point for the program
wxIMPLEMENT_APP(MyApp);

static const wxCmdLineEntryDesc g_cmdLineDesc[] = {
    {wxCMD_LINE_SWITCH, _("h"), _("help"), _("displays command line options"),
     wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP},
    {wxCMD_LINE_SWITCH, _("f"), _("force"), _("overwrite any existing file"),
     wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL},
    {wxCMD_LINE_SWITCH, _("l"), _("lib"), _("creates LIB library file"),
     wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL},
    {wxCMD_LINE_SWITCH, _("s"), _("symbol"), _("creates ASY symbol file"),
     wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL},
    {wxCMD_LINE_SWITCH, _("q"), _("quiet"),
     _("disables the GUI (for command line only usage)")},
    {wxCMD_LINE_PARAM, _(""), _(""), _("file name"), wxCMD_LINE_VAL_STRING,
     wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE},

    {wxCMD_LINE_NONE}};

// This is the implementation of the MyApp class
int MyApp::OnRun() {
  int exitcode = wxApp::OnRun();
#if !defined(NDEBUG) && !defined(__WINDOWS__)
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF |
                 _CRTDBG_LEAK_CHECK_DF);
// wxTheClipboard->Flush();
#endif
  return exitcode;
}

void MyApp::OnInitCmdLine(wxCmdLineParser& parser) {
  parser.SetDesc(g_cmdLineDesc);
  // must refuse '/' as parameter starter or cannot use "/path" style paths
  // parser.SetSwitchChars(_("-"));
}

bool MyApp::OnCmdLineParsed(wxCmdLineParser& parser) {
  // any remaining params should be the S-parameter file names
  int pCount = parser.GetParamCount();

  // If silent mode requested don't start the GUI
  // after handling all the comand line file names
  silent_mode = parser.Found(_("q"));
  force_mode = parser.Found(_("f"));
  SObject SData1;
  SData1.SetQuiet(silent_mode);
  for (int i = 0; i < pCount; i++) {
    wxFileName SFile(parser.GetParam(i));
    if (!SData1.readSFile(SFile)) return false;

    // Should we create the symbol file?
    if (parser.Found(_("s"))) {
      if (SData1.getASYfile().Exists() && !parser.Found(_("f"))) {
        wxString mess = wxString::Format(
            _("%s:%d ASY file %s already exists.  Delete it first."), __FILE__,
            __LINE__, SData1.getASYfile().GetFullPath().c_str());
        if (silent_mode) {
          cout << mess << endl;
        } else {
          wxLogError(mess);
        }
        return false;
      }
      if (!SData1.WriteASY()) return false;
    }

    // Should we create the library file?
    if (parser.Found(_("l"))) {
      if (SData1.getLIBfile().Exists() && !parser.Found(_("f"))) {
        wxString mess = wxString::Format(
            _("%s:%d LIB file %s already exists.  Delete it first."), __FILE__,
            __LINE__, SData1.getLIBfile().GetFullPath().c_str());
        if (silent_mode) {
          cout << mess << endl;
        } else {
          wxLogError(mess);
        }
        return false;
      }
      if (!SData1.WriteLIB()) return false;
    }
  }
  return !silent_mode;
}

bool MyApp::OnInit() {
  // init wxApp parent object
  if (!wxApp::OnInit()) return false;

  // Create the main window
  MyFrame* frame = new MyFrame(_("S2spice"), force_mode, wxPoint(50, 50), wxSize(640, 480));
  frame->Show(true);

  return true;
}

int MyApp::OnExit() {
  // clean up
  return 0;
}

// This is the implementation of the GUI
// If user selects -q at the command line the GUI is not shown unless -h is also
// used
MyFrame::MyFrame(const wxString& title, const bool force_mode, const wxPoint& pos, const wxSize& size)
    : wxFrame(nullptr, wxID_ANY, title, pos, size) {
  SData.SetForce(force_mode);
  debugFlag = true;
  debug_redirector = NULL;
#if defined(__WXMSW__)
  SetIcon(wxICON(IDI_ICON1));
#endif
  auto menuFile = new wxMenu();
  menuFile->Append(ID_OPEN, _("&Open...\tCtrl-O"), _("Open SnP file"));
  menuFile->AppendSeparator();
  menuFile->Append(ID_MKLIB, _("&Save LIB...\tCtrl-L"), _("Save Library file"));
  menuFile->Append(ID_MKSYM, _("&Save ASY...\tCtrl-L"), _("Save Symbol file"));
  menuFile->Append(wxID_EXIT);

  auto menuHelp = new wxMenu();
  menuHelp->Append(wxID_ABOUT);
  auto menuBar = new wxMenuBar();

  menuBar->Append(menuFile, _("&File"));
  menuBar->Append(menuHelp, _("&Help"));
  SetMenuBar(menuBar);

  CreateStatusBar();
  SetStatusText(
      _("S2spice: Select OPEN to start converting Touchstone files."));

  // Instead of writing an event handler we use a little functor
  // to connect to the menu events
  menuBar->Bind(wxEVT_MENU, [&](wxCommandEvent& event) {
    if (event.GetId() == ID_OPEN)
      OnOpen(event);
    else if (event.GetId() == ID_MKSYM)
      OnMkASY(event);
    else if (event.GetId() == ID_MKLIB)
      OnMkLIB(event);
    else if (event.GetId() == wxID_ABOUT)
      OnAbout(event);
    else if (event.GetId() == wxID_EXIT)
      OnQuit(event);
    else
      event.Skip();
  });

  wxBoxSizer* frameSizer = new wxBoxSizer(wxVERTICAL);
  wxPanel* mainPanel = new wxPanel(this);
  wxSizer* buttonRowSizer = new wxBoxSizer(wxHORIZONTAL);

  // Create the buttons
  wxButton* openButton = new wxButton(mainPanel, wxID_ANY, _("Open"));
  openButton->Bind(wxEVT_BUTTON, &MyFrame::OnOpen, this);
  buttonRowSizer->Add(openButton, wxALIGN_LEFT);
  wxButton* libButton = new wxButton(mainPanel, wxID_ANY, _("Save LIB"));
  libButton->Bind(wxEVT_BUTTON, &MyFrame::OnMkLIB, this);
  buttonRowSizer->Add(libButton, wxALIGN_LEFT);
  wxButton* symButton = new wxButton(mainPanel, wxID_ANY, _("Save SYM"));
  symButton->Bind(wxEVT_BUTTON, &MyFrame::OnMkASY, this);
  buttonRowSizer->Add(symButton, wxALIGN_LEFT);
  wxButton* aboutButton = new wxButton(mainPanel, wxID_ABOUT, _("About..."));
  aboutButton->Bind(wxEVT_BUTTON, &MyFrame::OnAbout, this);
  buttonRowSizer->Add(aboutButton, wxALIGN_LEFT);
  frameSizer->Add(buttonRowSizer);

  wxSizer* debugPanelSizer =
      new wxStaticBoxSizer(wxHORIZONTAL, mainPanel, "Log Messages");
  wxColour txtClr(0,180,0);
  wxColour bkgdClr(*wxBLACK);
  wxTextCtrl* debugWindow =
      new wxTextCtrl(mainPanel, wxID_ANY, wxEmptyString, wxDefaultPosition,
      wxDefaultSize, wxTE_READONLY | wxTE_MULTILINE | wxTE_RICH2);
  debugWindow->SetBackgroundColour(bkgdClr);
  debug_redirector = new wxStreamToTextRedirector(debugWindow);
  debugPanelSizer->Add(debugWindow, 1, wxEXPAND);
  frameSizer->Add(debugPanelSizer, 1, wxEXPAND);
  debugPanelSizer->SetSizeHints(this);
  debugWindow->SetDefaultStyle(wxTextAttr(txtClr, bkgdClr));

  mainPanel->SetSizer(frameSizer);
  frameSizer->SetSizeHints(this);
  Layout();
  wxSize sz = GetSize();
  sz.y = sz.y * 2;
  sz.x = sz.x * 2;
  SetMinSize(sz);
  Show();
}

void MyFrame::OnQuit(wxCommandEvent& event) {
  // shut down the main frame
  Close(false);
  assert(!debugFlag);
}

void MyFrame::OnClose(wxCloseEvent& event) {
  if (event.CanVeto() && !SData.dataSaved()) {
    if (wxMessageBox(
            _("The data has not been saved in library... continue closing?"),
            _("Please confirm"), wxICON_QUESTION | wxYES_NO) != wxYES) {
      event.Veto();
      return;
    }
  }
  if (debug_redirector != NULL) delete debug_redirector;
  debug_redirector = NULL;
  debugFlag = false;
  event.Skip();
}

void MyFrame::OnMkLIB(wxCommandEvent& event) {
  //  wxMessageBox("Make LIB button pressed.");
  if (SData.nPorts() < 1) {
    wxString mess = wxString::Format(
        _("%s:%d No data. Please open SnP file first."), __FILE__, __LINE__);
    wxLogError(mess);
    cout << mess << "\n";
    return;
  }

  wxBusyCursor wait;
  bool res = SData.writeLibFile(this);
  if (res) {
    wxString mess =
        wxString::Format(_("S2spice: Library file %s successfully created."),
                         SData.getSNPfile().GetFullPath());
    SetStatusText(mess);
    cout << mess << "\n";
  }
}

void MyFrame::OnMkASY(wxCommandEvent& event) {
  //  wxMessageBox("Symbol button pressed.");
  wxBusyCursor wait;
  bool res = SData.writeSymFile(this);
  if (res) {
    wxString mess(
        wxString::Format(_("S2spice: Symbol file %s successfully created."),
                         SData.getASYfile().GetFullPath()));
    SetStatusText(mess);
    cout << mess << "\n";
  }
}

void MyFrame::OnOpen(wxCommandEvent& event) {
  wxString mess;
  if (SData.openSFile(this))
    mess = wxString::Format(_("S2spice: Data successfully imported from %s."),
                            SData.getSNPfile().GetFullPath());
  else
    mess = wxString::Format(_("S2spice: Data import failed from %s!"),
                            SData.getSNPfile().GetFullPath());
  SetStatusText(mess);
  cout << mess << "\n";
}

void MyFrame::OnAbout(wxCommandEvent& event) {
  wxVersionInfo info;
  wxMessageBox(
      wxString::Format(
          _("s2spice  Copyright (C) <2023>  Dan Dickey\n"
            "This program comes with ABSOLUTELY NO WARRANTY.\n"
            "This is free software, and you are welcome to redistribute it\n"
            "under certain conditions.\n\n"
            "Use to convert Touchstone (aka SnP) file into LTspice\n"
            "subcircuit file. Open .SnP file, then use buttons to create\n"
            "and save library (LIB) and symbol (ASY) files.\n"
            "wxWidgets version: %s\n"
            "Running on: %s\n"),
          info.GetVersionString(), wxGetOsDescription()),
      _("About S2spice"), wxOK | wxICON_INFORMATION, this);
}
