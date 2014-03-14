/*
 *  Copyright (C) 2013 Ofer Kashayov <oferkv@live.com>
 *  This file is part of Phototonic Image Viewer.
 *
 *  Phototonic is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Phototonic is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Phototonic.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#include <QSettings>
#include <QModelIndexList>
#include <QStringList>
#include <QColor>

namespace GData
{
	// app settings
	extern QSettings *appSettings;
	extern unsigned int zoomInFlags;
	extern unsigned int zoomOutFlags;
	extern QColor backgroundColor;
	extern QColor thumbsBackgroundColor;
	extern QColor thumbsTextColor;
	extern unsigned int thumbsLayout;
	extern unsigned int thumbSpacing;
	extern bool exitInsteadOfClose;
	extern float imageZoomFactor;
	extern bool keepZoomFactor;
	extern int rotation;
	extern bool keepTransform;
	extern bool flipH;
	extern bool flipV;
	extern int defaultSaveQuality;
	extern int cropLeft;
	extern int cropTop;
	extern int cropWidth;
	extern int cropHeight;

	// app data
	extern QModelIndexList copyCutIdxList;
	extern bool copyOp;
	extern QStringList copyCutFileList;
	extern bool isFullScreen;
}

#endif // GLOBAL_H

