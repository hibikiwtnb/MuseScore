#include "pianolevelschooser.h"
#include "pianolevelsfilter.h"
#include "pianoview.h"

#include "libmscore/chord.h"
#include "libmscore/score.h"

#include <limits>

#include <QEvent>
#include <QSignalBlocker>

namespace Ms {

//---------------------------------------------------------
//   PianoLevelsChooser
//---------------------------------------------------------

PianoLevelsChooser::PianoLevelsChooser(QWidget *parent)
      : QWidget(parent)
      {
      setupUi(this);

      _levelsIndex = 2; // Velocity (relative)

      for (int i = 0; PianoLevelsFilter::FILTER_LIST[i]; ++i) {
            QString name = PianoLevelsFilter::FILTER_LIST[i]->name();
            levelsCombo->addItem(name, i);
            levelsCombo->setItemData(i, PianoLevelsFilter::FILTER_LIST[i]->tooltip(), Qt::ToolTipRole);
            }

      levelsCombo->setCurrentIndex(_levelsIndex);

      connect(levelsCombo, SIGNAL(activated(int)), SLOT(setLevelsIndex(int)));
      connect(setEventsBn, SIGNAL(clicked(bool)), SLOT(setEventDataPressed()));
      }

//---------------------------------------------------------
//   changeEvent
//---------------------------------------------------------

void PianoLevelsChooser::changeEvent(QEvent* event)
      {
      QWidget::changeEvent(event);

      if (event->type() != QEvent::LanguageChange)
            return;

      // Refresh controls generated from the .ui file
      retranslateUi(this);

      // Refresh dynamically populated filter names and tooltips
      QSignalBlocker blocker(levelsCombo);

      for (int i = 0; i < levelsCombo->count(); ++i) {
            const int index = levelsCombo->itemData(i).toInt();
            PianoLevelsFilter* filter = PianoLevelsFilter::FILTER_LIST[index];

            if (!filter)
                  continue;

            levelsCombo->setItemText(i, filter->name());
            levelsCombo->setItemData(i, filter->tooltip(), Qt::ToolTipRole);
            }
      }

//---------------------------------------------------------
//   setPianoView
//---------------------------------------------------------

void PianoLevelsChooser::setPianoView(PianoView* pianoView)
      {
      _pianoView = pianoView;
      }

//---------------------------------------------------------
//   setPlaybackEditingEnabled
//---------------------------------------------------------

void PianoLevelsChooser::setPlaybackEditingEnabled(bool enabled)
      {
      _playbackEditingEnabled = enabled;
      updateEditorEnabled();
      }

//---------------------------------------------------------
//   updateEditorEnabled
//---------------------------------------------------------

void PianoLevelsChooser::updateEditorEnabled()
      {
      PianoLevelsFilter* filter =
            PianoLevelsFilter::FILTER_LIST[_levelsIndex];

      const bool enabled =
            !filter->isPerEvent() || _playbackEditingEnabled;

      eventValSpinBox->setEnabled(enabled);
      setEventsBn->setEnabled(enabled);
      }

//---------------------------------------------------------
//   setEventPreviewValues
//---------------------------------------------------------

void PianoLevelsChooser::setEventPreviewValues(int ontime, int len)
      {
      PianoLevelsFilter* filter =
            PianoLevelsFilter::FILTER_LIST[_levelsIndex];

      if (!filter->isPerEvent())
            return;

      QList<PianoItem*> items = _pianoView->getSelectedItems();
      if (items.isEmpty())
            return;

      Note* note = items.front()->note();
      if (!note)
            return;

      Fraction noteLen = note->chord()->ticks();

      int value;
      if (filter->previewValue(note,
                               ontime,
                               len,
                               noteLen,
                               value)) {
            eventValSpinBox->setValue(value);
            }
      }

//---------------------------------------------------------
//   updateSetboxValue
//---------------------------------------------------------

void PianoLevelsChooser::updateSetboxValue()
      {
      PianoLevelsFilter* filter =
            PianoLevelsFilter::FILTER_LIST[_levelsIndex];

      if (filter->isPerEvent()) {
            eventValSpinBox->setRange(
                  std::numeric_limits<int>::min(),
                  std::numeric_limits<int>::max());
            }
      else {
            eventValSpinBox->setRange(
                  filter->minRange(),
                  filter->maxRange());
            }

      QList<PianoItem*> items = _pianoView->getSelectedItems();

      if (items.size() == 1) {
            PianoItem* item = items[0];
            Note* note = item->note();

            NoteEvent* event = item->getTweakNoteEvent();

            if (filter->isPerEvent() && !event)
                  return;

            int value = filter->value(note, event);
            eventValSpinBox->setValue(value);
            }
      }

//---------------------------------------------------------
//   setLevelsIndex
//---------------------------------------------------------

void PianoLevelsChooser::setLevelsIndex(int index)
      {
      if (_levelsIndex != index) {
            _levelsIndex = index;
            updateSetboxValue();
            updateEditorEnabled();
            emit levelsIndexChanged(index);
            }
      }

//---------------------------------------------------------
//   setEventDataPressed
//---------------------------------------------------------

void PianoLevelsChooser::setEventDataPressed()
      {
      if (!_pianoView)
            return;

      QList<PianoItem*> items =
            _pianoView->getSelectedItems();

      if (items.isEmpty())
            return;

      PianoLevelsFilter* filter =
            PianoLevelsFilter::FILTER_LIST[_levelsIndex];

      const int val =
            eventValSpinBox->value();

      Note* firstNote =
            items.front()->note();

      if (!firstNote)
            return;

      Score* score =
            firstNote->score();

      score->startCmd();

      for (PianoItem* item : qAsConst(items)) {
            if (!item)
                  continue;

            Note* note = item->note();
            if (!note)
                  continue;

            if (filter->isPerEvent()) {
                  for (NoteEvent& event : note->playEvents())
                        filter->setValue(note, &event, val);
                  }
            else {
                  filter->setValue(note, nullptr, val);
                  }
            }

      score->endCmd();

      emit notesChanged();
      }

}
