//=============================================================================
//  MusE Score
//  Linux Music Score Editor
//
//  Copyright (C) 2009 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "pianokeyboard.h"

#include "libmscore/staff.h"
#include "libmscore/part.h"
#include "libmscore/drumset.h"
#include "preferences.h"

#include <QColor>
#include <QPainterPath>

namespace Ms {

const char* PianoKeyboard::pitchNames[] =
            {"C", "C♯", "D", "D♯", "E", "F", "F♯", "G", "G♯", "A", "A♯", "B"}; // keep in sync with `valu` in libmscore/utils.cpp

static QPainterPath pianoKeyPaintPath(
      const QRectF& rect,
      PianoOrientation orientation,
      qreal cut)
      {
      QPainterPath path;

      if (cut <= 0.0) {
            path.addRect(rect);
            return path;
            }

      if (orientation == PianoOrientation::HORIZONTAL) {
            // Horizontal piano:
            // keyboard runs left/right and the key tips are at the bottom
            path.moveTo(rect.left(), rect.top());
            path.lineTo(rect.right(), rect.top());
            path.lineTo(rect.right(), rect.bottom() - cut);
            path.lineTo(rect.right() - cut, rect.bottom());
            path.lineTo(rect.left() + cut, rect.bottom());
            path.lineTo(rect.left(), rect.bottom() - cut);
            path.closeSubpath();
            }
      else {
            // Vertical piano:
            // keyboard runs top/bottom and the key tips are at the right
            path.moveTo(rect.left(), rect.top());
            path.lineTo(rect.right() - cut, rect.top());
            path.lineTo(rect.right(), rect.top() + cut);
            path.lineTo(rect.right(), rect.bottom() - cut);
            path.lineTo(rect.right() - cut, rect.bottom());
            path.lineTo(rect.left(), rect.bottom());
            path.closeSubpath();
            }

      return path;
      }

//---------------------------------------------------------
//   PianoKeyboard
//---------------------------------------------------------

PianoKeyboard::PianoKeyboard(QWidget* parent)
   : QWidget(parent)
      {
      setMouseTracking(true);
      setAttribute(Qt::WA_NoSystemBackground);
      setAttribute(Qt::WA_StaticContents);
      yRange   = noteHeight * 128;
      curPitch = -1;
      _ypos    = 0;
      curKeyPressed = -1;
      noteHeight = DEFAULT_KEY_HEIGHT;
      _orientation = PianoOrientation::VERTICAL;
      _staff = 0;
      }

//---------------------------------------------------------
//   keyCurrentColor
//---------------------------------------------------------

QColor PianoKeyboard::keyCurrentColor(int pitch, const QColor& defaultColor) const
      {
      if (pitch == curKeyPressed || _pressedPitches.contains(pitch)) {
            QColor color(preferences.getColor(PREF_UI_PIANO_USER_INPUT_COLOR));

            color.setAlpha(180);
            return color;
            }

      const Note* note = _playbackNotes.value(pitch, nullptr);

      if (!note && !_playbackActive)
            note = _selectionNotes.value(pitch, nullptr);

      return note
            ? pianoRollNoteColor(note, _coloring, false, _useNoteColors)
            : defaultColor;
      }

//---------------------------------------------------------
//   pitchAtPosition
//---------------------------------------------------------

int PianoKeyboard::pitchAtPosition(const QPoint& pos) const
      {
      Interval transp;

      if (_staff)
            transp = _staff->part()->instrument()->transpose();

      const QPointF point(pos);

      const int minPitch =
            pianoRollMinPitch(_use88KeyView);

      const int maxPitch =
            pianoRollMaxPitch(_use88KeyView);

      const int pitchCount =
            pianoRollPitchCount(_use88KeyView);

      const qreal keyboardLen =
            pitchCount * noteHeight;

      const qreal verticalKeyboardOffset =
            _orientation == PianoOrientation::VERTICAL
                  ? qMax<qreal>(height() - keyboardLen, 0.0)
                  : 0.0;

      const int keyboardThickness =
            _orientation == PianoOrientation::HORIZONTAL
                  ? height()
                  : width();

      const int blackKeyLen =
            pianoRollBlackKeyLength(keyboardThickness);

      const qreal whiteKeyOffset[] = {
            0, 1.5, 3.5, 5, 6.5, 8.5, 10.5, 12
            };

      const qreal blackKeyOffset[] = {
            1.5, 3.5, 6.5, 8.5, 10.5
            };

      const qreal horizontalWhiteKeyWidth =
            pianoRollWhiteKeyWidth(noteHeight);

      // Black keys must be tested first because they are painted
      // over the white keys
      for (int midiPitch = minPitch; midiPitch <= maxPitch; ++midiPitch) {
            const int instrPitch =
                  midiPitch - transp.chromatic;

            const int octave = instrPitch / 12;

            int degree = instrPitch % 12;
            if (degree < 0)
                  degree += 12;

            const int key = pianoRollBlackKeyIndex(degree);
            if (key == -1)
                  continue;

            QRectF rect;

            if (_orientation == PianoOrientation::HORIZONTAL) {
                  const qreal octaveOffset =
                        pianoRollPitchOffset(
                              octave * 12 + transp.chromatic,
                              minPitch,
                              noteHeight);

                  const qreal center =
                        octaveOffset
                        + pianoRollBlackKeyBoundary(key)
                              * horizontalWhiteKeyWidth;

                  const qreal offset =
                        center - noteHeight / 2.0;

                  rect = QRectF(
                        -_ypos + offset,
                        0,
                        noteHeight,
                        blackKeyLen);
                  }
            else {
                  const qreal center =
                        blackKeyOffset[key] * noteHeight;

                  const qreal offset =
                        center
                        - noteHeight / 2.0
                        + pianoRollPitchOffset(
                              octave * 12 + transp.chromatic,
                              minPitch,
                              noteHeight);

                  rect = QRectF(
                        0,
                        verticalKeyboardOffset
                              - _ypos
                              + keyboardLen
                              - offset
                              - noteHeight,
                        blackKeyLen,
                        noteHeight);
                  }

            if (rect.contains(point))
                  return midiPitch;
            }

      // No black key was hit - test white keys:
      for (int midiPitch = minPitch; midiPitch <= maxPitch; ++midiPitch) {
            const int instrPitch =
                  midiPitch - transp.chromatic;

            const int octave = instrPitch / 12;

            int degree = instrPitch % 12;
            if (degree < 0)
                  degree += 12;

            const int key = pianoRollWhiteKeyIndex(degree);
            if (key == -1)
                  continue;

            QRectF rect;

            if (_orientation == PianoOrientation::HORIZONTAL) {
                  const qreal octaveOffset =
                        pianoRollPitchOffset(
                              octave * 12 + transp.chromatic,
                              minPitch,
                              noteHeight);

                  const qreal x =
                        octaveOffset
                        + key * horizontalWhiteKeyWidth;

                  rect = QRectF(
                        -_ypos + x,
                        0,
                        horizontalWhiteKeyWidth,
                        height());
                  }
            else {
                  const qreal off1 =
                        whiteKeyOffset[key] * noteHeight
                        + pianoRollPitchOffset(
                              octave * 12 + transp.chromatic,
                              minPitch,
                              noteHeight);

                  const qreal off2 =
                        whiteKeyOffset[key + 1] * noteHeight
                        + pianoRollPitchOffset(
                              octave * 12 + transp.chromatic,
                              minPitch,
                              noteHeight);

                  rect = QRectF(
                        0,
                        verticalKeyboardOffset
                              - _ypos
                              + keyboardLen
                              - off2,
                        width(),
                        off2 - off1);
                  }

            if (rect.contains(point))
                  return midiPitch;
            }

      return -1;
      }

//---------------------------------------------------------
//   inputVelocityAtPosition
//---------------------------------------------------------

int PianoKeyboard::inputVelocityAtPosition(const QPoint& pos, int pitch) const
      {
      if (!preferences.getBool(PREF_UI_PIANOROLL_KEYBOARD_VELOCITY_FROM_POSITION))
            return 80;

      Interval transp;

      if (_staff)
            transp = _staff->part()->instrument()->transpose();

      const int instrPitch =
            pitch - transp.chromatic;

      int degree = instrPitch % 12;

      if (degree < 0)
            degree += 12;

      const bool blackKey = pianoRollBlackKeyIndex(degree) != -1;

      const int keyboardThickness =
            _orientation == PianoOrientation::HORIZONTAL
                  ? height()
                  : width();

      const int keyLength =
            blackKey
                  ? pianoRollBlackKeyLength(keyboardThickness)
                  : keyboardThickness;

      if (keyLength <= 0)
            return 80;

      const qreal position =
            _orientation == PianoOrientation::HORIZONTAL
                  ? pos.y()
                  : pos.x();

      const qreal ratio =
            qBound<qreal>(
                  0.0,
                  position / qreal(keyLength),
                  1.0);

      return 1 + qRound(ratio * 126.0);
      }


//---------------------------------------------------------
//   paint
//---------------------------------------------------------

void PianoKeyboard::paintEvent(QPaintEvent* /*event*/)
      {
      QPainter p(this);
      p.setRenderHints(QPainter::Antialiasing
                       | QPainter::SmoothPixmapTransform
                       | QPainter::TextAntialiasing);

      // Check for drumset, if any
      Drumset* ds = nullptr;
      Interval transp;
      if (_staff) {
            Part* part = _staff->part();
            ds = part->instrument()->drumset();
            transp = part->instrument()->transpose();
            }

      const int minPitch =
            pianoRollMinPitch(_use88KeyView);

      const int maxPitch =
            pianoRollMaxPitch(_use88KeyView);

      const int pitchCount =
            pianoRollPitchCount(_use88KeyView);

      const qreal keyboardLen =
            pitchCount * noteHeight;

      const qreal verticalKeyboardOffset =
            _orientation == PianoOrientation::VERTICAL
                  ? qMax<qreal>(height() - keyboardLen, 0.0)
                  : 0.0;

      const int keyboardThickness =
            _orientation == PianoOrientation::HORIZONTAL
                  ? height()
                  : width();

      const int blackKeyLen =
            pianoRollBlackKeyLength(keyboardThickness);

      const int baseFontSize = 12;
      const int minDrumFontSize = 6;

      int fontSize = baseFontSize;

      const int pitchNamesThreshold = 5;

      if (ds) {
            const qreal thicknessScale =
                  qreal(keyboardThickness)
                  / qreal(PIANO_KEYBOARD_WIDTH);

            fontSize = qBound(
                  baseFontSize,
                  qRound(baseFontSize * thicknessScale),
                  baseFontSize * 2);
            }

      QFont f("FreeSans", fontSize);

      // Key length may grow independently of pitch spacing
      // Keep the text short enough vertically to fit its pitch row
      while ( (fontSize > baseFontSize) && (QFontMetrics(f).height() > noteHeight - 2) ) {
            --fontSize;
            f.setPointSize(fontSize);
            }

      const int fontHeight = QFontMetrics(f).height();

      p.setFont(f);

      auto fitDrumFont = [&](const QString& text, qreal availableWidth) {
            QFont fittedFont = f;
            int size = fittedFont.pointSize();

            while (size > minDrumFontSize
                   && QFontMetrics(fittedFont).width(text) > availableWidth) {
                  --size;
                  fittedFont.setPointSize(size);
                  }

            return fittedFont;
            };

      // The original offsets are retained for the existing
      // VERTICAL keyboard implementation
      const qreal whiteKeyOffset[] = {
            0, 1.5, 3.5, 5, 6.5, 8.5, 10.5, 12
            };

      const qreal blackKeyOffset[] = {
            1.5, 3.5, 6.5, 8.5, 10.5
            };

      const qreal horizontalWhiteKeyWidth =
            pianoRollWhiteKeyWidth(noteHeight);

      //---------------------------------------------------
      // White keys
      //---------------------------------------------------

      QPen keyOutline(Qt::black);
      keyOutline.setWidthF(2.5);
      p.setPen(keyOutline);

      for (int midiPitch = minPitch; midiPitch <= maxPitch; ++midiPitch) {
            int instrPitch = midiPitch - transp.chromatic;

            int octave = instrPitch / 12;
            int degree = (instrPitch + 60) % 12;

            const int key = pianoRollWhiteKeyIndex(degree);
            if (key == -1)
                  continue;

            QString noteName =
                  qApp->translate("utils", pitchNames[degree])
                  + QString::number(octave - 1);

            if (ds) {
                  noteName = qApp->translate(
                        "drumset",
                        ds->name(instrPitch).toUtf8().constData());
                  }

            p.setBrush(keyCurrentColor(midiPitch, MScore::pianoWhiteKeysColor));

            if (_orientation == PianoOrientation::HORIZONTAL) {
                  // Seven equal-width white keys occupy the
                  // same [12 * noteHeight] octave width
                  qreal octaveOffset =
                        pianoRollPitchOffset(
                              octave * 12 + transp.chromatic,
                              minPitch,
                              noteHeight);

                  qreal x =
                        octaveOffset
                        + key * horizontalWhiteKeyWidth;

                  QRectF rect(
                        -_ypos + x,
                        0,
                        horizontalWhiteKeyWidth,
                        height()
                        );

                  const QPainterPath path =
                        pianoKeyPaintPath(
                              rect,
                              _orientation,
                              2.5);

                  p.drawPath(path);

                  if (ds && noteHeight >= fontHeight + 2) {
                        p.save();

                        p.translate(rect.left() + 1, rect.bottom() - 1);

                        p.rotate(-90.0);

                        QRectF rectText(
                              0.0,
                              0.0,
                              rect.height() - 2,
                              rect.width() - 2);

                        p.setFont(
                              fitDrumFont(
                                    noteName,
                                    rectText.width()));

                        p.drawText(rectText,
                              Qt::AlignLeft | Qt::AlignVCenter,
                              noteName);

                        p.restore();
                        }
                  else if (preferences.getBool(PREF_UI_PIANO_SHOWPITCHHELP)
                        && degree == 0
                        && noteHeight >= fontHeight - pitchNamesThreshold) {

                        const QRectF rectText =
                              rect.adjusted(1, 1, -1, -1);

                        p.drawText(rectText, Qt::AlignHCenter | Qt::AlignBottom, noteName);
                        }
                  }
            else {
                  // Original vertical keyboard geometry:
                  qreal off1 =
                        whiteKeyOffset[key] * noteHeight
                        + pianoRollPitchOffset(
                              octave * 12 + transp.chromatic,
                              minPitch,
                              noteHeight);

                  qreal off2 =
                        whiteKeyOffset[key + 1] * noteHeight
                        + pianoRollPitchOffset(
                              octave * 12 + transp.chromatic,
                              minPitch,
                              noteHeight);

                  QRectF rect(
                        0,
                        verticalKeyboardOffset
                              - _ypos
                              + keyboardLen
                              - off2,
                        width(),
                        off2 - off1
                        );

                  const QPainterPath path =
                        pianoKeyPaintPath(
                              rect,
                              _orientation,
                              2.5);

                  p.drawPath(path);

                  if (ds && noteHeight >= fontHeight + 2) {
                        QRectF rectText(
                              rect.x() + 1,
                              -_ypos
                                    + verticalKeyboardOffset
                                    + keyboardLen
                                    - pianoRollPitchOffset(
                                          midiPitch + 1,
                                          minPitch,
                                          noteHeight),
                              rect.width() - 1,
                              noteHeight);

                        p.setFont(
                              fitDrumFont(
                                    noteName,
                                    rectText.width()));

                        p.drawText(
                              rectText,
                              Qt::AlignVCenter | Qt::AlignLeft,
                              noteName);
                        }
                  else if (preferences.getBool(PREF_UI_PIANO_SHOWPITCHHELP)
                        && degree == 0
                        && noteHeight >= fontHeight - pitchNamesThreshold) {

                        QRectF rectText(
                              rect.x(),
                              rect.y(),
                              rect.width() - 4,
                              rect.height() - 1);

                        p.drawText(
                              rectText,
                              Qt::AlignRight | Qt::AlignBottom,
                              noteName);
                        }
                  }
            }

      //---------------------------------------------------
      // Black keys
      //---------------------------------------------------

      for (int midiPitch = minPitch; midiPitch <= maxPitch; ++midiPitch) {
            int instrPitch = midiPitch - transp.chromatic;

            int octave = instrPitch / 12;
            int degree = instrPitch % 12;

            if (degree < 0)
                  degree += 12;

            const int key = pianoRollBlackKeyIndex(degree);
            if (key == -1)
                  continue;

            QString noteName =
                  qApp->translate("utils", pitchNames[degree])
                  + QString::number(octave);

            if (ds) {
                  noteName = qApp->translate(
                        "drumset",
                        ds->name(instrPitch).toUtf8().constData());
                  }

            p.setBrush(keyCurrentColor(midiPitch, MScore::pianoBlackKeysColor));

            if (_orientation == PianoOrientation::HORIZONTAL) {
                  // Center each black key on the appropriate
                  // boundary between equal-width white keys
                  qreal octaveOffset =
                        pianoRollPitchOffset(
                              octave * 12 + transp.chromatic,
                              minPitch,
                              noteHeight);

                  qreal center =
                        octaveOffset
                        + pianoRollBlackKeyBoundary(key)
                              * horizontalWhiteKeyWidth;

                  qreal offset =
                        center - noteHeight / 2.0;

                  QRectF rect(
                        -_ypos + offset,
                        0,
                        noteHeight,
                        blackKeyLen
                        );

                  const QPainterPath path =
                        pianoKeyPaintPath(rect, _orientation, 1.0);

                  p.fillPath(path, MScore::pianoWhiteKeysColor);

                  p.drawPath(path);

                  if (ds && noteHeight >= fontHeight + 2) {
                        p.save();

                        p.setPen(QPen(Qt::white));

                        p.translate(rect.left() + 1,
                                    rect.bottom() - 1);
                        p.rotate(-90.0);

                        QRectF rectText(
                              0.0,
                              0.0,
                              rect.height() - 2,
                              rect.width() - 2);

                        p.setFont(
                              fitDrumFont(
                                    noteName,
                                    rectText.width()));

                        p.drawText(
                              rectText,
                              Qt::AlignLeft | Qt::AlignVCenter,
                              noteName);

                        p.restore();
                        }
                  }
            else {
                  // Original vertical keyboard geometry:
                  qreal center =
                        blackKeyOffset[key] * noteHeight;

                  qreal offset =
                        center
                        - noteHeight / 2.0
                        + pianoRollPitchOffset(
                              octave * 12 + transp.chromatic,
                              minPitch,
                              noteHeight);

                  QRectF rect(
                        0,
                        verticalKeyboardOffset
                              - _ypos
                              + keyboardLen
                              - offset
                              - noteHeight,
                        blackKeyLen,
                        noteHeight
                        );

                  const QPainterPath path =
                        pianoKeyPaintPath(rect, _orientation, 1.0);

                  p.fillPath(path, MScore::pianoWhiteKeysColor);

                  p.drawPath(path);

                  if (noteHeight >= fontHeight + 2) {
                        if (ds) {
                              p.setPen(QPen(Qt::white));

                              p.setFont(
                                    fitDrumFont(
                                          noteName,
                                          rect.width() - 2));

                              p.drawText(
                                    rect,
                                    Qt::AlignLeft | Qt::AlignVCenter,
                                    noteName);
                              }
                        }
                  }
            }
      }

//---------------------------------------------------------
//   setYpos
//---------------------------------------------------------

void PianoKeyboard::setYpos(int val)
      {
      if (_ypos != val) {
            _ypos = val;
            update();
            }
      }

//---------------------------------------------------------
//   setNoteHeight
//---------------------------------------------------------

void PianoKeyboard::setNoteHeight(int nh)
      {
      if (noteHeight != nh) {
            noteHeight = nh;
            update();
            }
      }

//---------------------------------------------------------
//   setPitch
//---------------------------------------------------------

void PianoKeyboard::setPitch(int val)
      {
      if (curPitch != val) {
            curPitch = val;
            update();
            }
      }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void PianoKeyboard::mousePressEvent(QMouseEvent* event)
      {
      const int pitch = pitchAtPosition(event->pos());

      if (!pitchIsValid(pitch))
            return;

      const Qt::KeyboardModifiers modifiers =
            QGuiApplication::keyboardModifiers();

      if (modifiers & Qt::ControlModifier) {
            emit pitchHighlightToggled(pitch);
            return;
            }

      curKeyPressed = pitch;

      _currentInputVelocity =
            inputVelocityAtPosition(
                  event->pos(),
                  curKeyPressed);

      update();

      emit keyPressed(
            curKeyPressed,
            _currentInputVelocity);
      }

//---------------------------------------------------------
//   mouseReleaseEvent
//---------------------------------------------------------

void PianoKeyboard::mouseReleaseEvent(QMouseEvent*)
      {
      const int pitch = curKeyPressed;

      curKeyPressed = -1;
      _currentInputVelocity = 80;

      update();
      emit keyReleased(pitch);
      }

//---------------------------------------------------------
//   mouseMoveEvent
//---------------------------------------------------------

void PianoKeyboard::mouseMoveEvent(QMouseEvent* event)
      {
      const int pitch = pitchAtPosition(event->pos());

      if (!pitchIsValid(pitch)) {
            if (curKeyPressed != -1) {
                  const int oldPitch = curKeyPressed;
                  curKeyPressed = -1;
                  _currentInputVelocity = 80;
                  update();
                  emit keyReleased(oldPitch);
                  }

            if (curPitch != -1) {
                  curPitch = -1;
                  emit pitchChanged(-1);
                  update();
                  }


            return;
            }

      const int velocity =
            inputVelocityAtPosition(
                  event->pos(),
                  pitch);

      if (pitch != curPitch) {
            curPitch = pitch;

            //Set tooltip
            int degree = curPitch % 12;
            int octave = curPitch / 12;
            QString text = qApp->translate("utils", pitchNames[degree]) + QString::number(octave - 1);
            Part* part = _staff->part();
            Drumset* ds = part->instrument()->drumset();
            if (ds)
                  text += " - " + qApp->translate("drumset", ds->name(curPitch).toUtf8().constData());

            setToolTip(text);

            //Send event
            emit pitchChanged(curPitch);
            if ((curKeyPressed != -1) && (curKeyPressed != pitch)) {
                  emit keyReleased(curKeyPressed);

                  curKeyPressed = pitch;
                  _currentInputVelocity = velocity;

                  emit keyPressed(
                        curKeyPressed,
                        _currentInputVelocity);
                  }
            update();
            }

      if (curKeyPressed == pitch
          && preferences.getBool(PREF_UI_PIANOROLL_KEYBOARD_VELOCITY_FROM_POSITION)
          && qAbs(velocity - _currentInputVelocity) >= 8) {

            _currentInputVelocity = velocity;

            emit keyVelocityPreview(
                  curKeyPressed,
                  _currentInputVelocity);
            }

      }

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void PianoKeyboard::leaveEvent(QEvent*)
      {
      if (curPitch != -1) {
            curPitch = -1;
            emit pitchChanged(-1);
            update();
            }
      }


//---------------------------------------------------------
//   setStaff
//---------------------------------------------------------

void PianoKeyboard::setStaff(Staff* staff)
      {
      _staff = staff;
      update();
      }

//---------------------------------------------------------
//   setOrientation
//---------------------------------------------------------

void PianoKeyboard::setOrientation(PianoOrientation o)
      {
      _orientation = o;
      update();
      }

//---------------------------------------------------------
//   setColoring
//---------------------------------------------------------

void PianoKeyboard::setColoring(Coloring c)
      {
      if (_coloring == c)
            return;

      _coloring = c;
      update();
      }

//---------------------------------------------------------
//   setUseNoteColors
//---------------------------------------------------------


void PianoKeyboard::setUseNoteColors(bool value)
      {
      _useNoteColors = value;
      }

//---------------------------------------------------------
//   set88KeyView
//---------------------------------------------------------

void PianoKeyboard::set88KeyView(bool enabled)
      {
      if (_use88KeyView == enabled)
            return;

      _use88KeyView = enabled;
      update();
      }

//---------------------------------------------------------
//   pressPitch
//---------------------------------------------------------

void PianoKeyboard::pressPitch(int pitch)
      {
      if (!pitchIsValid(pitch)
          || _pressedPitches.contains(pitch))
            return;

      _pressedPitches.insert(pitch);
      update();
      }

//---------------------------------------------------------
//   setPlaybackActive
//---------------------------------------------------------

void PianoKeyboard::setPlaybackActive(bool active)
      {
      if (_playbackActive == active)
            return;

      _playbackActive = active;
      update();
      }

//---------------------------------------------------------
//   releasePitch
//---------------------------------------------------------

void PianoKeyboard::releasePitch(int pitch)
      {
      if (!_pressedPitches.remove(pitch))
            return;

      update();
      }

//---------------------------------------------------------
//   setSelectionNotes
//---------------------------------------------------------

void PianoKeyboard::setSelectionNotes(
      const QHash<int, const Note*>& notes)
      {
      if (_selectionNotes == notes)
            return;

      _selectionNotes = notes;
      update();
      }

//---------------------------------------------------------
//   setPlaybackNotes
//---------------------------------------------------------

void PianoKeyboard::setPlaybackNotes(const QHash<int, const Note*>& notes)
      {
      if (_playbackNotes == notes)
            return;

      _playbackNotes = notes;
      update();
      }
}
