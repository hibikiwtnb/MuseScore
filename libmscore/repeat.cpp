//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2002-2011 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include "measure.h"
#include "repeat.h"
#include "score.h"
#include "staff.h"
#include "sym.h"
#include "system.h"
#include "xml.h"

namespace Ms {

//---------------------------------------------------------
//   RepeatMeasure
//---------------------------------------------------------

RepeatMeasure::RepeatMeasure(Score* score)
   : Rest(score)
      {
      }

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void RepeatMeasure::draw(QPainter* painter) const
      {
      if (_numMeasures > 1) {
            if (drawsSymbol()) {
                  painter->setPen(curColor());
                  drawSymbol(_numMeasures == 2 ? SymId::repeat2Bars : SymId::repeat4Bars, painter);
                  }
            return;
            }
      painter->setBrush(QBrush(curColor()));
      painter->setPen(Qt::NoPen);
      painter->drawPath(path);
      }

//---------------------------------------------------------
//   drawsSymbol
//---------------------------------------------------------

/**
 A group of 2 or 4 measures shows one symbol, over the barline in the middle of the group:
 the one before the 2nd measure of 2, before the 3rd of 4. That measure draws it.
 */

bool RepeatMeasure::drawsSymbol() const
      {
      return _numMeasures == 1 || _measureInGroup == _numMeasures / 2 + 1;
      }

//---------------------------------------------------------
//   layout
//---------------------------------------------------------

void RepeatMeasure::layout()
      {
      for (Element* e : el())
            e->layout();

      Staff* st = staff();
      if (_numMeasures > 1) {
            // centered vertically on the staff; horizontally on the barline, see Measure::layoutX
            path = QPainterPath();
            if (drawsSymbol()) {
                  const QRectF b = symBbox(_numMeasures == 2 ? SymId::repeat2Bars : SymId::repeat4Bars);
                  setPos(0.0, (st ? st->height() * .5 : spatium() * 2.0) - (b.y() + b.height() * .5));
                  setbbox(b);
                  }
            else {
                  setPos(0.0, 0.0);
                  setbbox(QRectF());
                  }
            return;
            }

      qreal ld = st ? st->lineDistance(tick()) : 1.0;
      qreal sp  = spatium();

      qreal y   = sp * 1.0 * ld;
      qreal w   = sp * 2.4 * ld;
      qreal h   = sp * 2.0 * ld;
      qreal lw  = sp * .50 * ld;  // line width
      qreal r   = sp * .20 * ld;  // dot radius

      setPos(0.0, (st ? (st->height() - h) / 2.0 : y) - y);

      path      = QPainterPath();

      path.moveTo(w - lw, y);
      path.lineTo(w,  y);
      path.lineTo(lw,  h+y);
      path.lineTo(0.0, h+y);
      path.closeSubpath();
      path.addEllipse(QRectF(w * .25 - r, y+h * .25 - r, r * 2.0, r * 2.0 ));
      path.addEllipse(QRectF(w * .75 - r, y+h * .75 - r, r * 2.0, r * 2.0 ));

      setbbox(path.boundingRect());
//      _space.setRw(width());
      }

//---------------------------------------------------------
//   ticks
//---------------------------------------------------------

Fraction RepeatMeasure::ticks() const
      {
      if (measure())
            return measure()->stretchedLen(staff());
      return Fraction(0, 1);
      }

//---------------------------------------------------------
//   writeProperties
//---------------------------------------------------------

void RepeatMeasure::writeProperties(XmlWriter& xml) const
      {
      Rest::writeProperties(xml);
      if (_numMeasures > 1) {
            xml.tag("numMeasures", _numMeasures);
            xml.tag("measureInGroup", _measureInGroup);
            }
      }

//---------------------------------------------------------
//   readProperties
//---------------------------------------------------------

bool RepeatMeasure::readProperties(XmlReader& e)
      {
      const QStringRef& tag(e.name());
      if (tag == "numMeasures")
            _numMeasures = e.readInt();
      else if (tag == "measureInGroup")
            _measureInGroup = e.readInt();
      else
            return Rest::readProperties(e);
      return true;
      }

//---------------------------------------------------------
//   accessibleInfo
//---------------------------------------------------------

QString RepeatMeasure::accessibleInfo() const
      {
      return Element::accessibleInfo();
      }

}

