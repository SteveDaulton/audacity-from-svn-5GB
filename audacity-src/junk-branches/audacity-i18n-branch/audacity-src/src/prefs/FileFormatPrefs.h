/**********************************************************************

  Audacity: A Digital Audio Editor

  FileFormatPrefs.h

  Joshua Haberman
  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_FILE_FORMAT_PREFS__
#define __AUDACITY_FILE_FORMAT_PREFS__

#include <wx/window.h>
#include <wx/statbox.h>
#include <wx/radiobut.h>
#include <wx/choice.h>

#include "PrefsPanel.h"

class FileFormatPrefs:public PrefsPanel {

 public:
   FileFormatPrefs(wxWindow * parent);
   ~FileFormatPrefs();
   bool Apply();
   
   void OnFormatChoice(wxCommandEvent& evt);
   void OnMP3FindButton(wxCommandEvent& evt);

 private:
   wxRadioButton *mCopyOrEdit[2];
   wxChoice *mMP3Bitrate;
   wxButton *mMP3FindButton;
   wxStaticText *mMP3Version;

   wxChoice *mDefaultExportFormat;
   wxButton *mExportOptionsButton;
   wxStaticText *mFormatText;

   int mFormat;
   int mFormatBits;

   void SetMP3VersionText();
   void SetFormatText();

   void Other();

 public:
   DECLARE_EVENT_TABLE();
};

#endif
