//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2017 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENSE.GPL
//=============================================================================

#include "fontStyleSelect.h"
#include "resetButton.h"
#include "inspectorplugin.h"
#include "icons.h"

#include <QtCore/QtPlugin>

// Qt Designer loads this plugin on its own, so it links neither libmscore nor
// mscoreapp: it compiles just the few files the custom widgets need, and
// stands in for the handful of symbols those files expect. The stand-ins live
// here rather than in the header, because mscoreapp globs every header in this
// tree and would find them clashing with the real MScore and Preferences.
namespace Ms {
      static const int VOICES = 4;

      struct MScore {
            static QColor selectColor[VOICES];
            static void init();
            };

      struct Preferences {
            int getInt(QString) const;
            QColor getColor(QString) const;
            bool isThemeDark() const;
            };

      QColor  MScore::selectColor[VOICES];

      void MScore::init()
            {
            selectColor[0].setNamedColor("#0065BF");   //blue
            selectColor[1].setNamedColor("#007F00");   //green
            selectColor[2].setNamedColor("#C53F00");   //orange
            selectColor[3].setNamedColor("#C31989");   //purple
            }
      Preferences preferences;

      int Preferences::getInt(QString) const
            {
            return 20;
            }
      QColor Preferences::getColor(QString) const { return QColor(); }
      bool Preferences::isThemeDark() const { return false; }
      }

void InspectorPlugin::initialize(QDesignerFormEditorInterface *)
      {
      if (m_initialized)
	      return;
      m_initialized = true;
      Ms::iconPath = QString(":/data/icons/");
      Ms::MScore::init();
      Ms::genIcons();
	}

QWidget* FontStyleSelectPlugin::createWidget(QWidget* parent)
	{
      return new Ms::FontStyleSelect(parent);
      }

QWidget* ResetButtonPlugin::createWidget(QWidget* parent)
	{
      return new Ms::ResetButton(parent);
      }

//---------------------------------------------------------
//   customWidgets
//---------------------------------------------------------

QList<QDesignerCustomWidgetInterface*> InspectorPlugins::customWidgets() const
	{
	QList<QDesignerCustomWidgetInterface*> plugins;
            plugins
               << new FontStyleSelectPlugin
               << new ResetButtonPlugin
               ;
      return plugins;
	}

InspectorPlugins::InspectorPlugins()
      {
//      Ms::MScore::init();
//      Ms::genIcons();
      }


// Q_EXPORT_PLUGIN(InspectorPlugins)

