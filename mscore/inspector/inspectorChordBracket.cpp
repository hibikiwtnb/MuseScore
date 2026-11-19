// MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
#include <QFrame>
#include "inspectorChordBracket.h"
#include "resetButton.h"
#include "libmscore/property.h"

namespace Ms {
InspectorChordBracket::InspectorChordBracket(QWidget* parent)
   : InspectorElementBase(parent)
      {
      QWidget* container = addWidget();
      container->setObjectName("inspectorChordBracket");
      QVBoxLayout* box = new QVBoxLayout(container);
      box->setContentsMargins(0, 0, 0, 0);
      box->setSpacing(0);
      QFrame* frame = new QFrame(container);
      frame->setFrameShape(QFrame::HLine);
      frame->setFrameShadow(QFrame::Sunken);
      box->addWidget(frame);
      QToolButton* title = new QToolButton(container);
      title->setObjectName("title");
      title->setText(tr("Chord bracket"));
      box->addWidget(title);
      QWidget* panel = new QWidget(container);
      box->addWidget(panel);
      QGridLayout* grid = new QGridLayout(panel);
      grid->setContentsMargins(3, 3, 3, 3);
      grid->setSpacing(3);
      grid->setColumnStretch(1, 1);

      QComboBox* hooks = new QComboBox(panel);
      hooks->addItem(tr("Both"), int(Direction::AUTO));
      hooks->addItem(tr("Top (left hand)"), int(Direction::UP));
      hooks->addItem(tr("Bottom (right hand)"), int(Direction::DOWN));

      QDoubleSpinBox* length = new QDoubleSpinBox(panel);
      length->setRange(0.0, 20.0);
      length->setSingleStep(.1);
      length->setSuffix(tr(" sp"));

      QCheckBox* right = new QCheckBox(tr("Right side"), panel);

      std::vector<InspectorItem> items;
      auto addRow = [&](int row, const QString& label, QWidget* control, Pid property) {
            QLabel* caption = new QLabel(label, panel);
            caption->setBuddy(control);
            caption->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
            QSizePolicy controlPolicy = control->sizePolicy();
            controlPolicy.setHorizontalPolicy(QSizePolicy::Expanding);
            control->setSizePolicy(controlPolicy);
            grid->addWidget(caption, row, 0);
            grid->addWidget(control, row, 1);
            ResetButton* reset = new ResetButton(panel);
            reset->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
            grid->addWidget(reset, row, 2);
            items.push_back({ property, 0, control, reset });
            };
      addRow(0, tr("Hooks:"), hooks, Pid::BRACKET_HOOK_POS);
      addRow(1, tr("Hook length:"), length, Pid::BRACKET_HOOK_LEN);
      addRow(2, tr("Placement:"), right, Pid::BRACKET_RIGHT_SIDE);
      mapSignals(items, { { title, panel } });
      }
}
