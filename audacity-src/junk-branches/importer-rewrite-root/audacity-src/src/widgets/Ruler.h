/**********************************************************************

  Audacity: A Digital Audio Editor

  Ruler.h

  Dominic Mazzoni

  This is a generic class which can be used to display just about
  any kind of ruler.  In addition to the Ruler class, which
  draws onto any wxDC, this file also contains a simple
  RulerPanel class, which allows you to work with a Ruler like
  any other wxWindow.

  At a minimum, the user must specify the dimensions of the
  ruler, its orientation (horizontal or vertical), and the
  values displayed at the two ends of the ruler (min and max).
  By default, this class will display tick marks at reasonable
  round numbers and fractions, for example, 100, 50, 10, 5, 1,
  0.5, 0.1, etc.

  The class is designed to display a small handful of
  labeled Major ticks, and a few Minor ticks between each of
  these.  Minor ticks are labeled if there is enough space.
  Labels will never run into each other.

  In addition to Real numbers, the Ruler currently supports
  two other formats for its display:

  Integer - never shows tick marks for fractions of an integer
  
  Time - Assumes values represent seconds, and labels the tick
         marks in "HH:MM:SS" format, e.g. 4000 seconds becomes
         "1:06:40", for example.  Will display fractions of
         a second, and tick marks are all reasonable round
         numbers for time (i.e. 15 seconds, 30 seconds, etc.)

**********************************************************************/

#ifndef __AUDACITY_RULER__
#define __AUDACITY_RULER__

#include <wx/dc.h>
#include <wx/event.h>
#include <wx/font.h>
#include <wx/panel.h>
#include <wx/window.h>

class Ruler {
 public:

   enum RulerFormat {
      IntFormat,
      RealFormat,
      TimeFormat
   };

   //
   // Constructor / Destructor
   //

   Ruler();
   ~Ruler();

   //
   // Required Ruler Parameters
   //

   void SetBounds(int left, int top, int right, int bottom);

   // wxHORIZONTAL || wxVERTICAL
   void SetOrientation(int orient);

   // min is the value at (x, y)
   // max is the value at (x+width, y+height)
   // (at the center of the pixel, in both cases)
   void SetRange(double min, double max);

   //
   // Optional Ruler Parameters
   //

   // IntFormat, RealFormat, or TimeFormat
   void SetFormat(RulerFormat format);

   // Specify the name of the units (like "dB") if you
   // want numbers like "1.6" formatted as "1.6 dB".
   void SetUnits(wxString units);

   // Logarithmic
   void SetLog(bool log);

   // Minimum number of pixels between labels
   void SetSpacing(int spacing);

   // If this is true, the edges of the ruler will always
   // receive a label.  If not, the nearest round number is
   // labeled (which may or may not be the edge).
   void SetLabelEdges(bool labelEdges);

   // Makes a vertical ruler hug the left side (instead of right)
   // and a horizontal ruler hug the top (instead of bottom)
   void SetFlip(bool flip);

   // Good defaults are provided, but you can override here
   void SetFonts(wxFont& minorFont, wxFont& majorFont);

   //
   // Drawing
   //

   // Note that it will not erase for you...
   void Draw(wxDC& dc);

 private:
   void Invalidate();
   void Update();
   void FindTickSizes();
   wxString LabelString(double d, bool major);
   void Tick(int pos, double d, bool major);

 private:
   int          mLeft, mTop, mRight, mBottom;
   int          mLength;
   wxDC        *mDC;

   wxFont      *mMinorFont, *mMajorFont;

   double       mMin, mMax;
   double       mUPP; // Units per pixel

   double       mMajor;
   double       mMinor;

   int          mDigits;

   int         *mBits;

   bool         mValid;

   struct Label {
      int pos;
      int lx, ly;
      wxString text;
   };
   
   int          mNumMajor;
   Label       *mMajorLabels;
   int          mNumMinor;
   Label       *mMinorLabels;   

   int          mOrientation;
   int          mSpacing;
   bool         mHasSetSpacing;
   bool         mLabelEdges;
   RulerFormat  mFormat;
   bool         mLog;
   bool         mFlip;
   wxString     mUnits;
};

class RulerPanel : public wxPanel {
   DECLARE_DYNAMIC_CLASS(RulerPanel)

 public:
   RulerPanel(wxWindow* parent, wxWindowID id,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize);

   ~RulerPanel();

   void OnPaint(wxPaintEvent &evt);
   void OnSize(wxSizeEvent &evt);

 public:

   Ruler  ruler;

private:
    DECLARE_EVENT_TABLE()
};

#endif //define __AUDACITY_RULER__
