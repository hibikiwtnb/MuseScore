//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//=============================================================================

#include "debuglog.h"

#include "preferences.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCursor>
#include <QDateTime>
#include <QEvent>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QMutex>
#include <QMutexLocker>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include <cstdio>

namespace Ms {

namespace {

constexpr int MAX_LOG_MESSAGES = 10000;

QMutex debugLogMutex;

QStringList pendingCompactMessages;
QStringList pendingSourceMessages;
QStringList pendingDetailedMessages;
QStringList pendingDetailedNoFileMessages;

QtMessageHandler previousMessageHandler = nullptr;
bool messageHandlerInstalled = false;

//---------------------------------------------------------
//   messageTypeName
//---------------------------------------------------------

const char* messageTypeName(QtMsgType type)
	  {
	  switch (type) {
			case QtDebugMsg:
				  return "Debug";
			case QtInfoMsg:
				  return "Info";
			case QtWarningMsg:
				  return "Warning";
			case QtCriticalMsg:
				  return "Critical";
			case QtFatalMsg:
				  return "Fatal";
			}
	  return "Log";
	  }

//---------------------------------------------------------
//   debugLogMessageHandler
//---------------------------------------------------------

void debugLogMessageHandler(QtMsgType type,
							const QMessageLogContext& context,
							const QString& msg)
	  {
	  const QString typeName =
			QString::fromLatin1(messageTypeName(type));

	  const QString compactMessage = msg;

	  QString fileInfo;
	  if (context.file && *context.file) {
			fileInfo = QString::fromUtf8(context.file);

			if (context.line > 0)
				  fileInfo += QString(":%1").arg(context.line);
			}

	  QString functionInfo;
	  if (context.function && *context.function)
			functionInfo = QString::fromUtf8(context.function);

	  QString sourceMessage;

	  if (!functionInfo.isEmpty())
			sourceMessage = QString("%1: %2").arg(functionInfo, msg);
	  else
			sourceMessage = msg;

	  const QString timestamp =
			QDateTime::currentDateTime().toString("HH:mm:ss.zzz");

	  // Full detailed form: [file/line] + [function]
	  QStringList contextParts;

	  if (!fileInfo.isEmpty())
			contextParts.append(fileInfo);

	  if (!functionInfo.isEmpty())
			contextParts.append(functionInfo);

	  QString detailedMessage =
			QString("%1 %2").arg(timestamp, typeName);

	  if (!contextParts.isEmpty())
			detailedMessage += QString(" [%1]").arg(contextParts.join(" | "));

	  detailedMessage += QString(": %1").arg(msg);

	  // Reduced detailed form: [function] only
	  QString detailedNoFileMessage =
			QString("%1 %2").arg(timestamp, typeName);

	  if (!functionInfo.isEmpty())
			detailedNoFileMessage += QString(" [%1]").arg(functionInfo);

	  detailedNoFileMessage += QString(": %1").arg(msg);

	  {
	  QMutexLocker locker(&debugLogMutex);

	  pendingCompactMessages.append(compactMessage);
	  pendingSourceMessages.append(sourceMessage);
	  pendingDetailedMessages.append(detailedMessage);
	  pendingDetailedNoFileMessages.append(detailedNoFileMessage);

	  while (pendingCompactMessages.size() > MAX_LOG_MESSAGES) {
			pendingCompactMessages.removeFirst();
			pendingSourceMessages.removeFirst();
			pendingDetailedMessages.removeFirst();
			pendingDetailedNoFileMessages.removeFirst();
			}
	  }

	  // Preserve the existing message output:
	  if (previousMessageHandler)
			// Windows debug build may already have mscoreMessageHandler()
			previousMessageHandler(type, context, msg);
	  else {
			const QByteArray formatted =
				  qFormatLogMessage(type, context, msg).toLocal8Bit();

			fprintf(stderr, "%s\n", formatted.constData());
			fflush(stderr);
			}
	  }

//---------------------------------------------------------
//   takePendingMessages
//---------------------------------------------------------

void takePendingMessages(QStringList* compactMessages,
						 QStringList* sourceMessages,
						 QStringList* detailedMessages,
						 QStringList* detailedNoFileMessages)
	  {
	  QMutexLocker locker(&debugLogMutex);

	  compactMessages->swap(pendingCompactMessages);
	  sourceMessages->swap(pendingSourceMessages);
	  detailedMessages->swap(pendingDetailedMessages);
	  detailedNoFileMessages->swap(pendingDetailedNoFileMessages);
	  }

//---------------------------------------------------------
//   clearPendingMessages
//---------------------------------------------------------

void clearPendingMessages()
	  {
	  QMutexLocker locker(&debugLogMutex);

	  pendingCompactMessages.clear();
	  pendingSourceMessages.clear();
	  pendingDetailedMessages.clear();
	  pendingDetailedNoFileMessages.clear();
	  }

} // namespace

//---------------------------------------------------------
//   setDebugLogMessageHandlerEnabled
//---------------------------------------------------------

void setDebugLogMessageHandlerEnabled(bool enabled)
	  {
	  if (enabled) {
			if (messageHandlerInstalled)
				  return;

			previousMessageHandler =
				  qInstallMessageHandler(debugLogMessageHandler);

			messageHandlerInstalled = true;
			}
	  else {
			if (!messageHandlerInstalled)
				  return;

			qInstallMessageHandler(previousMessageHandler);

			previousMessageHandler = nullptr;
			messageHandlerInstalled = false;
			}
	  }

//---------------------------------------------------------
//   debugLogMessageHandlerEnabled
//---------------------------------------------------------

bool debugLogMessageHandlerEnabled()
	  {
	  return messageHandlerInstalled;
	  }

//---------------------------------------------------------
//   DebugLogDock
//---------------------------------------------------------

DebugLogDock::DebugLogDock(QWidget* parent)
   : QDockWidget(parent)
	  {
	  setObjectName("debug-log");
	  setWindowTitle("Debug Log");

	  QWidget* content = new QWidget(this);
	  QVBoxLayout* layout = new QVBoxLayout(content);
	  layout->setContentsMargins(4, 4, 4, 4);
	  layout->setSpacing(4);

	  QHBoxLayout* controls = new QHBoxLayout;

	  QPushButton* clearButton = new QPushButton(tr("Clear"), content);
	  QPushButton* copyButton = new QPushButton(tr("Copy All"), content);

	  _enabledCheck = new QCheckBox(tr("Enabled"), content);
	  _enabledCheck->setChecked(preferences.getBool(PREF_APP_DEBUG_LOG_ENABLED));

	  _showDetails = preferences.getBool(PREF_APP_DEBUG_LOG_DETAILS);
	  _showSource = preferences.getBool(PREF_APP_DEBUG_LOG_SHOW_SOURCE);
	  _autoScroll = preferences.getBool(PREF_APP_DEBUG_LOG_AUTOSCROLL);

	  _detailsCheck = new QCheckBox(tr("Details"), content);
	  _detailsCheck->setChecked(_showDetails);

	  _sourceCheck = new QCheckBox(tr("Source"), content);
	  _sourceCheck->setChecked(_showSource);

	  _autoScrollCheck = new QCheckBox(tr("Autoscroll"), content);
	  _autoScrollCheck->setChecked(_autoScroll);

	  controls->addWidget(clearButton);
	  controls->addWidget(copyButton);
	  controls->addStretch();
	  controls->addWidget(_enabledCheck);
	  controls->addWidget(_detailsCheck);
	  controls->addWidget(_sourceCheck);
	  controls->addWidget(_autoScrollCheck);

	  layout->addLayout(controls);

	  QHBoxLayout* searchControls = new QHBoxLayout;

	  _searchEdit = new QLineEdit(content);
	  _searchEdit->setPlaceholderText(tr("Find in log"));
	  _searchEdit->setClearButtonEnabled(true);

	  QToolButton* findPreviousButton = new QToolButton(content);
	  findPreviousButton->setArrowType(Qt::UpArrow);
	  findPreviousButton->setToolTip(tr("Find previous"));

	  QToolButton* findNextButton = new QToolButton(content);
	  findNextButton->setArrowType(Qt::DownArrow);
	  findNextButton->setToolTip(tr("Find next"));

	  searchControls->addWidget(_searchEdit);
	  searchControls->addWidget(findPreviousButton);
	  searchControls->addWidget(findNextButton);

	  layout->addLayout(searchControls);

	  _output = new QPlainTextEdit(content);
	  _output->setReadOnly(true);
	  _output->setLineWrapMode(QPlainTextEdit::NoWrap);
	  _output->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

	  layout->addWidget(_output);

	  setWidget(content);

	  _selectionCopyMenu = new QMenu(_output);
	  _selectionCopyMenu->setFocusPolicy(Qt::NoFocus);
	  _selectionCopyMenu->setAttribute(Qt::WA_ShowWithoutActivating);

	  QAction* copySelectionAction =
			_selectionCopyMenu->addAction(tr("Copy"));

	  connect(copySelectionAction, &QAction::triggered,
			  this, [this]() {
			const QTextCursor cursor = _output->textCursor();

			if (!cursor.hasSelection())
				  return;

			QApplication::clipboard()->setText(
				  cursor.selection().toPlainText());
			});

	  _output->viewport()->installEventFilter(this);
	  _searchEdit->installEventFilter(this);

	  connect(clearButton, &QPushButton::clicked, this, [this]() {
			clearPendingMessages();

			_compactMessages.clear();
			_sourceMessages.clear();
			_detailedMessages.clear();
			_detailedNoFileMessages.clear();

			_output->clear();
			});

	  connect(copyButton, &QPushButton::clicked, this, [this]() {
			QApplication::clipboard()->setText(_output->toPlainText());
			});


	  connect(_enabledCheck, &QCheckBox::toggled, this, [](bool checked) {
			preferences.setPreference(PREF_APP_DEBUG_LOG_ENABLED, checked);
			});

	  connect(_detailsCheck, &QCheckBox::toggled, this, [](bool checked) {
			preferences.setPreference(PREF_APP_DEBUG_LOG_DETAILS, checked);
			});

	  connect(_sourceCheck, &QCheckBox::toggled, this, [](bool checked) {
			preferences.setPreference(PREF_APP_DEBUG_LOG_SHOW_SOURCE, checked);
			});

	  connect(_autoScrollCheck, &QCheckBox::toggled, this, [](bool checked) {
			preferences.setPreference(PREF_APP_DEBUG_LOG_AUTOSCROLL, checked);
			});

	  connect(findPreviousButton, &QToolButton::clicked, this, [this]() {
			findText(true);
			});

	  connect(findNextButton, &QToolButton::clicked, this, [this]() {
			findText(false);
			});

	  _flushTimer = new QTimer(this);
	  _flushTimer->setInterval(100);

	  connect(_flushTimer, &QTimer::timeout, this, [this]() {
			flushMessages();
			});

	  _preferenceListenerId =
			preferences.addOnSetListener([this](const QString& key, const QVariant& value) {
				  const bool checked = value.toBool();

				  if (key == PREF_APP_DEBUG_LOG_ENABLED) {
						if (_enabledCheck->isChecked() != checked) {
							  QSignalBlocker blocker(_enabledCheck);
							  _enabledCheck->setChecked(checked);
							  }

						setLoggingEnabled(checked);
						return;
						}

				  if (key == PREF_APP_DEBUG_LOG_DETAILS) {
						flushMessages();

						_showDetails = checked;

						if (_detailsCheck->isChecked() != checked) {
							  QSignalBlocker blocker(_detailsCheck);
							  _detailsCheck->setChecked(checked);
							  }

						refreshOutput();
						return;
						}

				  if (key == PREF_APP_DEBUG_LOG_SHOW_SOURCE) {
						flushMessages();

						_showSource = checked;

						if (_sourceCheck->isChecked() != checked) {
							  QSignalBlocker blocker(_sourceCheck);
							  _sourceCheck->setChecked(checked);
							  }

						refreshOutput();
						return;
						}

				  if (key == PREF_APP_DEBUG_LOG_AUTOSCROLL) {
						_autoScroll = checked;

						if (_autoScrollCheck->isChecked() != checked) {
							  QSignalBlocker blocker(_autoScrollCheck);
							  _autoScrollCheck->setChecked(checked);
							  }

						if (_autoScroll) {
							  QScrollBar* scrollBar = _output->verticalScrollBar();
							  scrollBar->setValue(scrollBar->maximum());
							  }

						return;
						}
				  });

	  setLoggingEnabled(preferences.getBool(PREF_APP_DEBUG_LOG_ENABLED));

	  // Don't let the debug logger steal keyboard focus from ScoreView:
	  setFocusPolicy(Qt::NoFocus);
	  content->setFocusPolicy(Qt::NoFocus);
	  const auto childWidgets = content->findChildren<QWidget*>();
	  for (QWidget* widget : childWidgets)
			widget->setFocusPolicy(Qt::NoFocus);

	  // Search is the key exception:
	  _searchEdit->setFocusPolicy(Qt::StrongFocus);
	  }

//---------------------------------------------------------
//   setLoggingEnabled
//---------------------------------------------------------

void DebugLogDock::setLoggingEnabled(bool enabled)
	  {
	  if (enabled) {
			setDebugLogMessageHandlerEnabled(true);

			// Immediately display anything collected between
			// startup and creation the DebugLogDock:
			flushMessages();

			if (_flushTimer && !_flushTimer->isActive())
				  _flushTimer->start();
			}
	  else {
			// Stop new messages entering the queue
			setDebugLogMessageHandlerEnabled(false);

			// Flush anything that reached the handler before it was removed
			flushMessages();

			if (_flushTimer)
				  _flushTimer->stop();
			}
	  }

//---------------------------------------------------------
//   ~DebugLogDock
//---------------------------------------------------------

DebugLogDock::~DebugLogDock()
	  {
	  if (_preferenceListenerId)
			preferences.removeOnSetListener(_preferenceListenerId);

	  setDebugLogMessageHandlerEnabled(false);
	  }

//---------------------------------------------------------
//   flushMessages
//---------------------------------------------------------

void DebugLogDock::flushMessages()
	  {
	  QStringList compactMessages;
	  QStringList sourceMessages;
	  QStringList detailedMessages;
	  QStringList detailedNoFileMessages;

	  takePendingMessages(&compactMessages,
						  &sourceMessages,
						  &detailedMessages,
						  &detailedNoFileMessages);

	  if (compactMessages.isEmpty())
			return;

	  QScrollBar* scrollBar = _output->verticalScrollBar();
	  const int oldScrollValue = scrollBar->value();

	  _compactMessages.append(compactMessages);
	  _sourceMessages.append(sourceMessages);
	  _detailedMessages.append(detailedMessages);
	  _detailedNoFileMessages.append(detailedNoFileMessages);

	  bool removedOldMessages = false;

	  while (_compactMessages.size() > MAX_LOG_MESSAGES) {
			_compactMessages.removeFirst();
			_sourceMessages.removeFirst();
			_detailedMessages.removeFirst();
			_detailedNoFileMessages.removeFirst();
			removedOldMessages = true;
			}

	  if (removedOldMessages) {
            // Synchronize the retained message lists and the widget
			refreshOutput();
			return;
			}

	  const QStringList& messages =
			_showDetails
				  ? (_showSource
						? detailedMessages
						: detailedNoFileMessages)
				  : (_showSource
						? sourceMessages
						: compactMessages);

	  _output->appendPlainText(messages.join('\n'));

	  if (_autoScroll)
			scrollBar->setValue(scrollBar->maximum());
	  else
			scrollBar->setValue(oldScrollValue);
	  }

//---------------------------------------------------------
//   findText
//---------------------------------------------------------

void DebugLogDock::findText(bool backward)
	  {
	  const QString text = _searchEdit->text();

	  if (text.isEmpty())
			return;

	  QTextDocument::FindFlags flags;

	  if (backward)
			flags |= QTextDocument::FindBackward;

	  if (_output->find(text, flags))
			return;

	  QTextCursor cursor(_output->document());

	  cursor.movePosition(backward ? QTextCursor::End
								   : QTextCursor::Start);

	  _output->setTextCursor(cursor);
	  _output->find(text, flags);
	  }

//---------------------------------------------------------
//   eventFilter
//---------------------------------------------------------

bool DebugLogDock::eventFilter(QObject* watched, QEvent* event)
	  {
	  // Show a small Copy popup when mouse text selection finishes:
	  if (watched == _output->viewport() && event->type() == QEvent::MouseButtonRelease) {
			QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);

			if (mouseEvent->button() == Qt::LeftButton) {
				  // QPlainTextEdit needs to finalize selection first:
				  QTimer::singleShot(0, this, [this]() {
						if (!_output->textCursor().hasSelection())
							  return;

						_selectionCopyMenu->popup(QCursor::pos());
						});
				  }
			}

	  // Register keyboard-focus before entering search-box:
	  if (watched == _searchEdit && event->type() == QEvent::MouseButtonPress) {
			QWidget* focusWidget = QApplication::focusWidget();

			if (focusWidget && focusWidget != _searchEdit)
				  _focusBeforeSearch = focusWidget;
			}


	  // Prevent MuseScore's global shortcuts from accepting keys
	  // belonging to the Debug Log search field:
	  if (watched == _searchEdit && event->type() == QEvent::ShortcutOverride) {
			QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

			switch (keyEvent->key()) {
				  case Qt::Key_Return:
				  case Qt::Key_Enter:
				  case Qt::Key_Escape:
						event->accept();
						return true;

				  default:
						break;
				  }
			}

	  // Escape will return keyboard control to whatever had focus previously (i.e. ScoreView)
	  if (watched == _searchEdit && event->type() == QEvent::KeyPress) {
			QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

			if (keyEvent->key() == Qt::Key_Escape) {
				  if (_focusBeforeSearch)
						_focusBeforeSearch->setFocus();
				  else
						_searchEdit->clearFocus();

				  return true;
				  }

			if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
				  findText(keyEvent->modifiers() & Qt::ShiftModifier);
				  return true;
				  }
			}

	  return QDockWidget::eventFilter(watched, event);
	  }

//---------------------------------------------------------
//   refreshOutput
//---------------------------------------------------------

void DebugLogDock::refreshOutput()
	  {
	  QScrollBar* scrollBar = _output->verticalScrollBar();
	  const int oldScrollValue = scrollBar->value();

	  const QStringList& messages =
			_showDetails
				  ? (_showSource
						? _detailedMessages
						: _detailedNoFileMessages)
				  : (_showSource
						? _sourceMessages
						: _compactMessages);

	  _output->setPlainText(messages.join('\n'));

	  if (_autoScroll)
			scrollBar->setValue(scrollBar->maximum());
	  else
			scrollBar->setValue(oldScrollValue);
	  }

} // namespace Ms
