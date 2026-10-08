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

#ifndef __REPEAT_H__
#define __REPEAT_H__

#include "text.h"
#include "rest.h"

namespace Ms {

class Score;
class Segment;

//---------------------------------------------------------
//   @@ RepeatMeasure
//---------------------------------------------------------

class RepeatMeasure final : public Rest {
      QPainterPath path;
      int _numMeasures    { 1 };      ///< length of the repeated pattern: 1, 2 or 4 measures
      int _measureInGroup { 1 };      ///< 1-based place of this measure in its group of _numMeasures

   public:
      RepeatMeasure(Score*);
      RepeatMeasure &operator=(const RepeatMeasure&) = delete;

      RepeatMeasure* clone() const override   { return new RepeatMeasure(*this); }
      Element* linkedClone() override         { return Element::linkedClone(); }
      ElementType type() const override       { return ElementType::REPEAT_MEASURE; }
      void draw(QPainter*) const override;
      void layout() override;
      Fraction ticks() const override;
      Fraction actualTicks() const { return Rest::ticks(); }

      QString accessibleInfo() const override;

      int numMeasures() const             { return _numMeasures;    }
      void setNumMeasures(int n)          { _numMeasures = n;       }
      int measureInGroup() const          { return _measureInGroup; }
      void setMeasureInGroup(int n)       { _measureInGroup = n;    }
      bool drawsSymbol() const;

      void writeProperties(XmlWriter& xml) const override;
      bool readProperties(XmlReader& e) override;
      };


}     // namespace Ms
#endif

