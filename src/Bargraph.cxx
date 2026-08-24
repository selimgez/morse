/*  Copyright 1998-2004 Ward Cunningham and Jim Wilson
    Distributed under the GNU GPL V2 license.
    See http://c2.com/morse

    This file is part of Morse.

    Morse is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Morse is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with Morse; if not, write to the Free Software Foundation, Inc.,
    59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "Bargraph.h"	// Our contract specifies what we must deliver
#include <cstring>	// Strlen(), strspn()
#include <cstdlib>	// rand() for random integers
#include <FL/Fl.H>	// Event stuff (e.g., release ordinate, abscissa)

/*  learned_threshold - value() a letter must decay below to be retired.
 *
 *  select() weights every active letter by its value() (an EMA of recent
 *  error), which only ever asymptotes toward zero and never actually
 *  reaches it - so a "mastered" letter always keeps a small, nonzero
 *  chance of being asked again, forever.  Once value() crosses this
 *  threshold we retire the letter outright (see Bargraph::retire()) so it
 *  is never selected again, instead of letting it linger at a vanishing
 *  but nonzero weight.
 *
 *  Default (0.00342) is not an arbitrary guess: it's the point at which
 *  the Slider's fill is visually empty (0.5px rounding at the default
 *  640x240 window's 146px-tall bar - see Bargraph::tesselate() and
 *  Fl_Slider::draw()'s FL_DOWN_BOX inset), i.e. the retire point matches
 *  what the student already sees as "nothing left here". A perfect,
 *  never-wrong run crosses it after ~22 clean passes - grade() applies a
 *  double EMA update (0.875 twice, not once) whenever overall error is
 *  low, so decay is 0.875^2 = 0.765625 per clean answer, not 0.875; any
 *  wrong answer bumps value() back up and pushes retirement out further,
 *  proportionally to how often the student actually misses that letter.
 *
 *  Override at runtime with set_learned_threshold() - m.fl's main() calls
 *  it from argv[1] when given, so this can be tuned without a rebuild.
 */
static double learned_threshold = 0.00342;
void Bargraph::set_learned_threshold(double t) { learned_threshold = t; }

/***	Bargraph - a constructor that mimics the underlying Group
 ***   ~Bargraph - the other way 'round
 *
 *  Bargraph(int,int, int,int, char*) is passed the coordinates of the
 *  upper-left corner of the rectangular display area, its width and
 *  height, and a character string specifying a set of characters used
 *  to label a corresponding set of Fl_Sliders.
 *
 *  The Sliders are created with nonsensical positions and sizes.  They
 *  may be carefully positioned by "Bargraph::tesselate()", below
 */
Bargraph::Bargraph(int x, int y, int w, int h, const char* l):
  Fl_Group(x,y, w,h) {				// Call base-class c'tor
  int n = strlen(l);				// Number of letters taught
  slider_labels = new char[2*n];		// Characters in slider labels
  for (char* to = slider_labels; *l; l++,to+=2){// Loop to create Sliders:
    to[0] = *l; to[1] = '\0';			//   Char-> 1-char string
    Fl_Slider* s = new Fl_Slider(0,0, 0,0, to); //   Create, add to Bargraph.
    s->type(FL_VERT_FILL_SLIDER);		//   Sight-gauge style
    s->minimum(1); s->maximum(0); s->value(1);	//   Start full to the brim
    s->align(FL_ALIGN_BOTTOM);			//   Letter below Slider
    s->deactivate();				//   We'll activate later
    s->clear_visible_focus();			//   Too many to focus on.
  }
  retired = new bool[n];			// None mastered yet
  for (int i = 0; i < n; i++) retired[i] = false;
  overall = 0;					// Good error rate to start.
}
Bargraph::~Bargraph() { delete[] slider_labels; delete[] retired; }


/***	Bargraph::handle - because Fl_Group ignores releases!
 *
 *  In a concession to creeping featurism, we'll override Fl_Group's handle()
 *  method so we can manually enable and disable letters (against all common
 *  sense).  It seems an Fl_Group is intended solely as a "container" to hold
 *  its children.  It is so focused on motherhood, it cannot think on its own.
 *  Here, we'll look at release events that happen below the Fl_Slider area and
 *  use them to activate or deactivate letters (which, of course, we shouldn't
 *  be doing).
 */
int Bargraph::handle(int e) {
  if (e == FL_RELEASE && 		// If event is a release, and
      Fl::event_y() >= y()+h()-20)	// it's in the Slider's label area),
    click_letter(Fl::event_x());	//   activate/deactivate letter
  return Fl_Group::handle(e);		// Then, let base class have a go.
}

/***	Bargraph::tesselate - arrange visible sliders to cover area
 *
 *  Tesselate() examines the Bargraph's array of sliders, counts the visible
 *  ones, and repositions them to fill the display area.  Tesselate() should
 *  be invoked each time characters are added or subtracted from the lesson
 *  plan, and each time the Bargraph is resized.
 */
void Bargraph::tesselate(int gap) {		// Pixels between sliders
  resize_gap = gap;				// Set for sister method
  int nd = 0;					// Number (sliders) displayed
  for (int i = 0; i < children(); i++)		// Count visible sliders
    if (child(i)->visible()) nd++;
  int dx = w()/nd;				// Slider horizontal "period"
  int width = dx*nd - gap;			// Width of slider array
  int x0 = (w()-width) >> 1;			// Inset to 1st slider
  width = dx-gap;				// Width of individual Slider
  for (int i = 0; i < children(); i++) {	// Loop to reposition sliders
    Fl_Slider* s = (Fl_Slider*) child(i);	//   Pointer to this Slider
    if (s->visible()) {				//   If part of lesson plan,
      s->resize(x0,y()+15, width, h()-35);	//     Reposition Slider
      x0 += dx;					//     Advance right
    }	
  }
}

/***	Resize - resize (to reposition Sliders)
 *
 *  We'll undo the built-in resizing imposed on us by Fl_Group, since it
 *  doesn't suit our purposes very well.  Making the Group smaller can
 *  obsure the letters, and that is not acceptable.
 */
void Bargraph::resize(int x, int y, int w, int h) {
  Fl_Group::resize(x, y, w, h); tesselate(resize_gap);
}

/***	Bargraph::find - find Slider corresponding to given character
 *
 *  Hunt the letter and return its Slider or zero if we can't find it.
 */
Fl_Slider* Bargraph::find(int c) {
  for (int i = 0; i < children(); i++) {	// Loop through sliders:
    Fl_Slider* s = (Fl_Slider*) child(i);	//   Pointer for easy reference
    if (s->label()[0] == c) return s;		//   Label match?  return yes
  }
  return 0;					// Can't find it? return no
}

/***	Apply() - Apply Slider method to subset of Bargraph children
 *
 *  Apply() is passed a pointer to a Fl_Slider method accepting no arguments,
 *  and a character string describing a (sub)set of Bargraph children to call
 *  it on.  The method is called on any child whose label matches a character
 *  from the string.
 */   
void Bargraph::apply(void (Fl_Slider::*m)(),	// Isn't the syntax awful!!!
                     const char* set) {		// Set of Slider names
  for (int i = 0; i < children(); i++) {	// Loop through Sliders:
    Fl_Slider* s = (Fl_Slider*)child(i);	//   Working Slider pointer
    if (strspn(s->label(), set)) (s->*m)();	//   If label in set, apply.
  }   
}

/***	Bargraph::disable - remove characters from lesson plan
 ***	Bargraph::enable - undo disabling of the characters
 ***	Bargraph::deactivate - set characters' Sliders to inactive
 ***	Bargraph::activate - set characters' sliders to active
 *
 *  Disable() is passed a string (a subset of the string passed to the
 *  constructor).  It makes the corresponding sliders invisible.  After
 *  disabling a set of sliders, call Bargraph::tesselate() to rearrange
 *  the remainder, and redraw() (or show()) on "this".
 *
 *  Enable() does exactly the opposite of Disable().
 *
 *  Inactivate() keeps the characters' Sliders from receiving events
 *  and changes its appearance (to a washed out look).  Activate() does
 *  exactly the opposite.  NOTE:  Using FLTK's built-in widget activate/
 *  deactivate method does not seem to produce sufficient distinction
 *  between the two widget states.  Thus, we may have to resort to a whole
 *  series of changes.  We don't want to go to the trouble of subclassing
 *  the Fl_Slider, so we'll seek them out one at a time rather than using
 *  apply() (which can call only a single, parameterless method).
 */
void Bargraph::enable(const char* s)  { apply(&Fl_Slider::set_visible,   s); }
void Bargraph::disable(const char* s) { apply(&Fl_Slider::clear_visible, s); }
void Bargraph::deactivate(const char* set) {
  while (*set) if (Fl_Slider* s = find(*set++)) {
    s->deactivate(); s->labelfont(FL_HELVETICA);	// Less emphasis
    s->selection_color(FL_BACKGROUND_COLOR);		// Printf() claims this
    s->redraw_label();					//   is original value.
  }
}
void Bargraph::activate(const char* set) {
  while (*set) if (Fl_Slider* s = find(*set++)) {
    s->activate(); s->labelfont(FL_HELVETICA_BOLD);	// A little bolder
    s->selection_color(FL_BLUE);			// Change color
    s->redraw_label();
  }
}

/***	Bargraph::click_letter - manually (de)activate a letter
 *
 *  Against my better judgement, the user can click on a letter to force
 *  its introduction or removal to/from the set of characters currently
 *  available for random selection.  This is creeping featurism at its worst!
 */
void Bargraph::click_letter(int x) {
  bool some_active = false;			// Must have at least one!
  for (int i = 0; i < children(); i++) {	// Loop through sliders
    Fl_Slider* s = (Fl_Slider*)child(i);	//   Convenience pointer
    if (!s->visible() || retired[i]) continue;	//   Ignore nonciricula/mastered
    if(s->x() <= x && x < s->x()+s->w()){	//   If beneath this Slider,
      const char* letter = s->label();		//     Name to (de)activate
      if (s->active()) deactivate(letter);	//     If active, make it not
      else               activate(letter);	//     If not, make it so
    }
    if (s->active()) some_active = true;	// So far, so good!
  }
  if (!some_active)				// Nothing active?  Press one
    for (int i = 0; i < children(); i++) {	//   into service - but never
      Fl_Slider* s = (Fl_Slider*)child(i);	//   a retired (mastered) one.
      if (s->visible() && !retired[i]) { activate(s->label()); break; }
    }
}

/***	drand - return random double in [0, 1)
 *
 *  Drand() calls rand(), and scales the result into [0-1).
 *
 *  BUG:  MINGW rand()'s RAND_MAX seems to be only 0x7FFF = 32767
 */
static double drand() { return rand()/(1.0 + RAND_MAX); }

/***	select - randomly select next character based on error estimate
 *
 *  I was going to subclass Bargraph, but I suspect it has little use other
 *  than a Morse machine, so let's just add the method here.
 *
 *  BUG:  There must be at least one active slider!
 */
int Bargraph::select() {
  double sum = 0;				// Sum of error rates
  for (int i = 0; i < children(); i++) {	// Scan through Sliders
    Fl_Slider* s = (Fl_Slider*) child(i);	//   Convenience pointer
    if (s->active() && s->visible()) 		//   Sum repertoire
      sum += s->value();		
  }
  sum *= drand();			// Sum = random variable
  int result = 0;
  for (int i = 0; i < children(); i++) {	// Make second scan
    Fl_Slider* s = (Fl_Slider*) child(i);	//   For convenience
    if (s->active() && s->visible()) {		//   If selection candidate:
      result = *s->label();			//     Note (in case chosen)
      sum -= s->value();			//     Spin the wheel
      if (sum < 0) break;			//     If S's number hit
    }						//       it's the lucky one!
  }
  return result;				// Return lucky slider's name
}

/***	grade - update error estimates
 *
 *  Grade() is passed a the letter just taught and a boolean pass/fail
 *  score (true == pass).  The overall and letter error rates are updated.
 */
static void update(Fl_Slider* s, bool pass) {	// Lil' helper function
  if (s) s->value(0.875 * s->value() + (pass? 0: 0.125));
}
void Bargraph::grade(int c, bool pass) {
  Fl_Slider* s = find(c);		// Find pertinent Slider
  update(s, pass);			// Update (at least) once
  overall = overall*0.875 +		// Update overall error rate
    (pass? 0: 0.125);			// If overall error rate is low,
  if (overall < 0.1) update(s, pass);	//   accelerate  character decay rate
  if (s && s->active() && s->value() < learned_threshold)
    retire(s);				// Mastered: retire it for good.
}

/***	Bargraph::retire - mark a mastered letter learned, for good
 *
 *  Once a letter's error estimate has decayed below learned_threshold, we
 *  consider it learned.  Deactivating it keeps select() from choosing it.
 *  We leave it visible (rather than hiding it) so the student can still
 *  see it on the board - its bar just stays frozen, near-empty, since
 *  grade() is never called for it again.  The "retired" flag (separate
 *  from visible()/active()) is what actually keeps it retired: it stops
 *  graduate() and click_letter() from mistaking a mastered letter for one
 *  that simply hasn't been introduced yet (both look visible+inactive).
 */
void Bargraph::retire(Fl_Slider* s) {
  s->deactivate();			// Never selectable again ...
  s->value(0);				// ... bar reads fully empty, not just
					//   near-empty (frozen error estimate
					//   would otherwise leave a sliver) ...
  for (int i = 0; i < children(); i++)	// ... and remember which one, so
    if (child(i) == s) { retired[i] = true; break; }	//   it stays that way.
  redraw();
}

/***	Bargraph::complete - true once every letter has been retired
 *
 *  Every letter still in the lesson plan (visible) must be retired for
 *  the lesson to be complete - a visible, non-retired letter is either
 *  still being taught (active) or hasn't been introduced yet, either way
 *  there's still something left to do.
 */
bool Bargraph::complete() {
  for (int i = 0; i < children(); i++)
    if (child(i)->visible() && !retired[i]) return false;
  return true;
}

/***	Bargraph::reset - return every letter to its first-run state
 *
 *  Undoes retire()/graduate()/grade() history: every Slider goes back to
 *  the same value(), activation, visibility and appearance it had right
 *  after the constructor ran.  Callers still need to reapply whatever
 *  app-specific starting lesson plan the constructor's caller applied
 *  (e.g. Bargraph doesn't know which letters should start active) -
 *  reset() only restores the generic per-Slider state.
 */
void Bargraph::reset() {
  for (int i = 0; i < children(); i++) {
    Fl_Slider* s = (Fl_Slider*) child(i);
    s->value(1);			//   Start full to the brim again
    s->deactivate();			//   Not yet (re)taught
    s->labelfont(FL_HELVETICA);	//   Un-bold ...
    s->selection_color(FL_BACKGROUND_COLOR);	// ... and un-blue.
    s->set_visible();			//   Back on the board
    s->redraw_label();
    retired[i] = false;		//   Nothing learned (yet) this time.
  }
  overall = 0;				// Good error rate to start.
  tesselate(resize_gap);
  redraw();
}

/***	graduate - add another letter!
 *
 *  Graduate() scans the lesson looking for the first inactive Slider.
 *  It activates the slider and returns.
 */
void Bargraph::graduate() {
  if (overall > 0.3) return;		// If overall error rate too high ...
  const char* next_letter = 0;		// Candidate for new letter
  for (int i = 0; i < children(); i++) {// Scan for worst letter, new letter
    Fl_Slider* s = (Fl_Slider*)child(i);//   Convenience pointer
    if (s->visible() && !retired[i]) {	//   If in course outline, unretired
      if (s->active()) {		//     If already introduced,
        if (s->value() >0.4) return;	//       If unlearned, hold 'em back.
      } else if (!next_letter)		//   Note first untaught letter.
        next_letter = s->label();	//     we'll introduce it next time.
    }
  }
  if (next_letter) activate(next_letter);
}
