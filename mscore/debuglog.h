//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//=============================================================================

#ifndef __DEBUGLOG_H__
#define __DEBUGLOG_H__

#include <QDockWidget>
#include <QPointer>
#include <QStringList>

#include <cstdint>

class QCheckBox;
class QEvent;
class QLineEdit;
class QMenu;
class QPlainTextEdit;
class QTimer;

namespace Ms {

//---------------------------------------------------------
//   DebugLogDock
//---------------------------------------------------------

class DebugLogDock : public QDockWidget {
	  Q_OBJECT

	  QPlainTextEdit* _output { nullptr };
	  QLineEdit* _searchEdit { nullptr };
	  QMenu* _selectionCopyMenu { nullptr };
	  QPointer<QWidget> _focusBeforeSearch;

	  QCheckBox* _enabledCheck { nullptr };
	  QCheckBox* _detailsCheck { nullptr };
	  QCheckBox* _sourceCheck { nullptr };
	  QCheckBox* _autoScrollCheck { nullptr };

	  QTimer* _flushTimer { nullptr };

	  QStringList _compactMessages;
	  QStringList _sourceMessages;
	  QStringList _detailedMessages;
	  QStringList _detailedNoFileMessages;

	  bool _autoScroll { true };
	  bool _showDetails { false };
	  bool _showSource { false };

	  uint32_t _preferenceListenerId { 0 };

	  void findText(bool backward);
	  void flushMessages();
	  void refreshOutput();
	  void setLoggingEnabled(bool enabled);

   protected:
	  bool eventFilter(QObject* watched, QEvent* event) override;

   public:
	  explicit DebugLogDock(QWidget* parent = nullptr);
	  ~DebugLogDock();
	  };

void setDebugLogMessageHandlerEnabled(bool enabled);
bool debugLogMessageHandlerEnabled();

} // namespace Ms

#endif
