//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#include <QtTest/QtTest>
#include "mtest/testutils.h"
#include "libmscore/score.h"
#include "libmscore/measure.h"
#include "libmscore/segment.h"
#include "libmscore/system.h"

#define DIR QString("libmscore/equalwidth/")

using namespace Ms;

//---------------------------------------------------------
//   TestEqualWidth
//    equalwidth.mscx has four systems of four measures:
//    1: uneven note densities, first measure has clef/key/time
//    2: first measure too dense for an equal share
//    (spatium 1mm so that equal shares fit four measures)
//    3: second measure has stretch 2
//    4: filler
//---------------------------------------------------------

class TestEqualWidth : public QObject, public MTest
      {
      Q_OBJECT

      MasterScore* load();

   private slots:
      void initTestCase()     { initMTest(); }
      void equalWidths();
      void overflow();
      void userStretch();
      void styleOff();
      };

//---------------------------------------------------------
//   noteArea
//    measure width without the leading header segments
//    (clef, key and time signature before the first note)
//---------------------------------------------------------

static qreal noteArea(Measure* m)
      {
      Segment* first = m->first(SegmentType::ChordRest);
      Segment* fs    = m->firstEnabled();
      qreal header   = (fs != first) ? first->x() - fs->x() : 0.0;
      return m->width() - header;
      }

static QList<Measure*> systemMeasures(MasterScore* score, int idx)
      {
      QList<Measure*> ml;
      System* s = score->systems().at(idx);
      for (MeasureBase* mb : s->measures())
            if (mb->isMeasure())
                  ml.append(toMeasure(mb));
      return ml;
      }

// measures must not overlap and must stay inside the page width
static void checkNoOverflow(MasterScore* score, int idx)
      {
      QList<Measure*> ml = systemMeasures(score, idx);
      for (int i = 1; i < ml.size(); ++i)
            QVERIFY(ml[i - 1]->x() + ml[i - 1]->width() <= ml[i]->x() + 0.01);
      qreal pageWidth = score->styleD(Sid::pagePrintableWidth) * DPI;
      QVERIFY(ml.last()->x() + ml.last()->width() <= pageWidth + 0.01);
      }

MasterScore* TestEqualWidth::load()
      {
      MasterScore* score = readScore(DIR + "equalwidth.mscx");
      score->doLayout();
      return score;
      }

//---------------------------------------------------------
//   equalWidths
//    note areas are equal although densities differ and the
//    first measure carries the system header
//---------------------------------------------------------

void TestEqualWidth::equalWidths()
      {
      MasterScore* score = load();
      QVERIFY(score->styleB(Sid::equalMeasureWidth));
      QCOMPARE(score->systems().size(), 4);
      QList<Measure*> ml = systemMeasures(score, 0);
      QCOMPARE(ml.size(), 4);
      qreal a0 = noteArea(ml[0]);
      for (Measure* m : ml)
            QVERIFY(qAbs(noteArea(m) - a0) < 0.01);
      // measures without header have exactly equal widths,
      // the header is added on top of the equal share
      QVERIFY(qAbs(ml[1]->width() - ml[2]->width()) < 0.01);
      QVERIFY(ml[0]->width() > ml[2]->width() + 1.0);
      checkNoOverflow(score, 0);
      delete score;
      }

//---------------------------------------------------------
//   overflow
//    the dense measure keeps its minimum, the others share
//    the rest equally
//---------------------------------------------------------

void TestEqualWidth::overflow()
      {
      MasterScore* score = load();
      QList<Measure*> ml = systemMeasures(score, 1);
      QCOMPARE(ml.size(), 4);
      qreal a1 = noteArea(ml[1]);
      QVERIFY(noteArea(ml[0]) > a1 + 1.0);
      QVERIFY(qAbs(noteArea(ml[2]) - a1) < 0.01);
      QVERIFY(qAbs(noteArea(ml[3]) - a1) < 0.01);
      checkNoOverflow(score, 1);
      delete score;
      }

//---------------------------------------------------------
//   userStretch
//    measure stretch multiplies the equal share
//---------------------------------------------------------

void TestEqualWidth::userStretch()
      {
      MasterScore* score = load();
      QList<Measure*> ml = systemMeasures(score, 2);
      QCOMPARE(ml.size(), 4);
      qreal a0 = noteArea(ml[0]);
      QVERIFY(qAbs(noteArea(ml[1]) - 2.0 * a0) < 0.01);
      QVERIFY(qAbs(noteArea(ml[2]) - a0) < 0.01);
      QVERIFY(qAbs(noteArea(ml[3]) - a0) < 0.01);
      checkNoOverflow(score, 2);
      delete score;
      }

//---------------------------------------------------------
//   styleOff
//    with the option off, widths follow note density again
//---------------------------------------------------------

void TestEqualWidth::styleOff()
      {
      MasterScore* score = load();
      score->style().set(Sid::equalMeasureWidth, false);
      score->doLayout();
      QList<Measure*> ml = systemMeasures(score, 0);
      QCOMPARE(ml.size(), 4);
      QVERIFY(noteArea(ml[1]) > noteArea(ml[2]) + 1.0);
      checkNoOverflow(score, 0);
      delete score;
      }

QTEST_MAIN(TestEqualWidth)
#include "tst_equalwidth.moc"
