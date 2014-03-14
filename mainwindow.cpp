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

#include <QtGui>
#include "mainwindow.h"
#include "thumbview.h"
#include "dialogs.h"
#include "global.h"

Phototonic::Phototonic(QWidget *parent) : QMainWindow(parent)
{
	GData::appSettings = new QSettings("Phototonic", "Phototonic");
	loadDefaultSettings();
	createThumbView();
	createActions();
	createMenus();
	createToolBars();
	createStatusBar();
	createFSTree();
	createImageView();

	connect(qApp, SIGNAL(focusChanged(QWidget* , QWidget*)), 
				this, SLOT(updateActions(QWidget*, QWidget*)));

	restoreGeometry(GData::appSettings->value("geometry").toByteArray());
	restoreState(GData::appSettings->value("MainWindowState").toByteArray());
	setWindowTitle("Phototonic");
	setWindowIcon(QIcon(":/images/phototonic.png"));

	splitter = new QSplitter(Qt::Horizontal);
	splitter->addWidget(fsTree);
	splitter->addWidget(thumbView);
	splitter->restoreState(GData::appSettings->value("splitterSizes").toByteArray());

	stackedWidget = new QStackedWidget;
	stackedWidget->addWidget(splitter);
	stackedWidget->addWidget(imageView);
	setCentralWidget(stackedWidget);

	cliImageLoaded = handleArgs();
	if (cliImageLoaded)
		loadImagefromCli();

	initComplete = true;
	thumbViewBusy = false;
	currentHistoryIdx = -1;
	needHistoryRecord = true;
	refreshThumbs(true);
}

bool Phototonic::handleArgs()
{
	if (QCoreApplication::arguments().size() == 2)
	{
		QFileInfo cliArg(QCoreApplication::arguments().at(1));
		if (cliArg.isDir())
		{
			thumbView->currentViewDir = QCoreApplication::arguments().at(1);
			restoreCurrentIdx();
			return false;
		}
		else
		{
			thumbView->currentViewDir = cliArg.absolutePath();
			restoreCurrentIdx();
			cliFileName = cliArg.fileName();
			return true;
		}
	}
	
	return false;
}

static bool removeDirOp(QString dirToDelete)
{
	bool ok = true;
	QDir dir(dirToDelete);

	Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden |
					QDir::AllDirs | QDir::Files, QDir::DirsFirst))
	{
		if (info.isDir())
			ok = removeDirOp(info.absoluteFilePath());
		else 
			ok = QFile::remove(info.absoluteFilePath());

		if (!ok)
			return ok;
	}
	ok = dir.rmdir(dirToDelete);

	return ok;
}

void Phototonic::unsetBusy()
{	
	thumbViewBusy = false;
}

void Phototonic::createThumbView()
{
	thumbView = new ThumbView(this);
	thumbView->thumbsSortFlags = (QDir::SortFlags)GData::appSettings->value("thumbsSortFlags").toInt();

	connect(this, SIGNAL(abortThumbLoading()), thumbView, SLOT(abort()));
	connect(thumbView, SIGNAL(unsetBusy()), this, SLOT(unsetBusy()));
	connect(thumbView, SIGNAL(updateState(QString)), this, SLOT(updateState(QString)));
	connect(thumbView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), 
				this, SLOT(changeActionsBySelection(QItemSelection, QItemSelection)));
}

void Phototonic::addMenuSeparator(QWidget *widget)
{
	QAction *separator = new QAction(this);
	separator->setSeparator(true);
	widget->addAction(separator);
}

void Phototonic::createImageView()
{
	GData::backgroundColor = GData::appSettings->value("backgroundColor").value<QColor>();
	imageView = new ImageView(this);
	connect(saveAction, SIGNAL(triggered()), imageView, SLOT(saveImage()));
	connect(saveAsAction, SIGNAL(triggered()), imageView, SLOT(saveImageAs()));
	imageView->ImagePopUpMenu = new QMenu();

	// Widget actions
	imageView->addAction(nextImageAction);
	imageView->addAction(prevImageAction);
	imageView->addAction(firstImageAction);
	imageView->addAction(lastImageAction);
	imageView->addAction(zoomInAct);
	imageView->addAction(zoomOutAct);
	imageView->addAction(origZoomAct);
	imageView->addAction(resetZoomAct);
	imageView->addAction(rotateRightAct);
	imageView->addAction(rotateLeftAct);
	imageView->addAction(flipHAct);
	imageView->addAction(flipVAct);
	imageView->addAction(cropAct);
	imageView->addAction(saveAction);
	imageView->addAction(deleteAction);
	imageView->addAction(closeImageAct);
	imageView->addAction(fullScreenAct);
	imageView->addAction(settingsAction);
	imageView->addAction(exitAction);
	imageView->addAction(mirrorDisabledAct);
	imageView->addAction(mirrorDualAct);
	imageView->addAction(mirrorTripleAct);
	imageView->addAction(mirrorVDualAct);
	imageView->addAction(mirrorQuadAct);

	// Actions
	imageView->ImagePopUpMenu->addAction(nextImageAction);
	imageView->ImagePopUpMenu->addAction(prevImageAction);
	imageView->ImagePopUpMenu->addAction(firstImageAction);
	imageView->ImagePopUpMenu->addAction(lastImageAction);

	addMenuSeparator(imageView->ImagePopUpMenu);
	zoomSubMenu = new QMenu("Zoom");
	zoomSubMenuAct = new QAction("Zoom", this);
	zoomSubMenuAct->setIcon(QIcon(":/images/zoom.png"));
	zoomSubMenuAct->setMenu(zoomSubMenu);
	imageView->ImagePopUpMenu->addAction(zoomSubMenuAct);
	zoomSubMenu->addAction(zoomInAct);
	zoomSubMenu->addAction(zoomOutAct);
	zoomSubMenu->addAction(origZoomAct);
	zoomSubMenu->addAction(resetZoomAct);
	addMenuSeparator(zoomSubMenu);
	zoomSubMenu->addAction(keepZoomAct);

	addMenuSeparator(imageView->ImagePopUpMenu);
	transformSubMenu = new QMenu("Transform");
	transformSubMenuAct = new QAction("Transform", this);
	transformSubMenuAct->setMenu(transformSubMenu);
	imageView->ImagePopUpMenu->addAction(transformSubMenuAct);
	transformSubMenu->addAction(rotateRightAct);
	transformSubMenu->addAction(rotateLeftAct);
	transformSubMenu->addAction(flipHAct);
	transformSubMenu->addAction(flipVAct);
	transformSubMenu->addAction(cropAct);

	MirroringSubMenu = new QMenu("Mirror");
	mirrorSubMenuAct = new QAction("Mirror", this);
	mirrorSubMenuAct->setMenu(MirroringSubMenu);
	imageView->ImagePopUpMenu->addAction(mirrorSubMenuAct);
	mirroringGroup = new QActionGroup(this);
	mirroringGroup->addAction(mirrorDisabledAct);
	mirroringGroup->addAction(mirrorDualAct);
	mirroringGroup->addAction(mirrorTripleAct);
	mirroringGroup->addAction(mirrorVDualAct);
	mirroringGroup->addAction(mirrorQuadAct);
	MirroringSubMenu->addActions(mirroringGroup->actions());
	addMenuSeparator(transformSubMenu);
	transformSubMenu->addAction(keepTransformAct);

	addMenuSeparator(imageView->ImagePopUpMenu);
	imageView->ImagePopUpMenu->addAction(saveAction);
	imageView->ImagePopUpMenu->addAction(saveAsAction);
	imageView->ImagePopUpMenu->addAction(deleteAction);

	addMenuSeparator(imageView->ImagePopUpMenu);
	imageView->ImagePopUpMenu->addAction(closeImageAct);
	imageView->ImagePopUpMenu->addAction(fullScreenAct);

	addMenuSeparator(imageView->ImagePopUpMenu);
	imageView->ImagePopUpMenu->addAction(settingsAction);

	addMenuSeparator(imageView->ImagePopUpMenu);
	imageView->ImagePopUpMenu->addAction(exitAction);

	imageView->setContextMenuPolicy(Qt::DefaultContextMenu);
	GData::isFullScreen = GData::appSettings->value("isFullScreen").toBool();
	fullScreenAct->setChecked(GData::isFullScreen); 
}

void Phototonic::createActions()
{
	thumbsGoTopAct = new QAction("Top", this);
	connect(thumbsGoTopAct, SIGNAL(triggered()), this, SLOT(goTop()));
	thumbsGoTopAct->setShortcut(QKeySequence::MoveToStartOfDocument);

	thumbsGoBottomAct = new QAction("Bottom", this);
	connect(thumbsGoBottomAct, SIGNAL(triggered()), this, SLOT(goBottom()));
	thumbsGoBottomAct->setShortcut(QKeySequence::MoveToEndOfDocument);

	closeImageAct = new QAction("Close Image", this);
	connect(closeImageAct, SIGNAL(triggered()), this, SLOT(closeImage()));
	closeImageAct->setShortcut(Qt::Key_Escape);

	fullScreenAct = new QAction("Full Screen", this);
	fullScreenAct->setCheckable(true);
	connect(fullScreenAct, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));
	fullScreenAct->setShortcut(QKeySequence("f"));
	
	settingsAction = new QAction("Preferences", this);
	settingsAction->setIcon(QIcon(":/images/settings.png"));
	settingsAction->setShortcut(QKeySequence("Ctrl+P"));
	connect(settingsAction, SIGNAL(triggered()), this, SLOT(showSettings()));

	exitAction = new QAction("E&xit", this);
	exitAction->setShortcut(QKeySequence("Ctrl+Q"));
	connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));

	thumbsZoomInAct = new QAction("Zoom In", this);
	thumbsZoomInAct->setShortcut(QKeySequence::ZoomIn);
	connect(thumbsZoomInAct, SIGNAL(triggered()), this, SLOT(thumbsZoomIn()));
	thumbsZoomInAct->setIcon(QIcon(":/images/zoom_in.png"));
	if (thumbView->thumbHeight == 300)
		thumbsZoomInAct->setEnabled(false);

	thumbsZoomOutAct = new QAction("Zoom Out", this);
	thumbsZoomOutAct->setShortcut(QKeySequence::ZoomOut);
	connect(thumbsZoomOutAct, SIGNAL(triggered()), this, SLOT(thumbsZoomOut()));
	thumbsZoomOutAct->setIcon(QIcon(":/images/zoom_out.png"));
	if (thumbView->thumbHeight == 100)
		thumbsZoomOutAct->setEnabled(false);

	cutAction = new QAction("Cut", this);
	cutAction->setShortcut(QKeySequence::Cut);
	cutAction->setIcon(QIcon(":/images/cut.png"));
	connect(cutAction, SIGNAL(triggered()), this, SLOT(cutImages()));
	cutAction->setEnabled(false);

	copyAction = new QAction("Copy", this);
	copyAction->setShortcut(QKeySequence::Copy);
	copyAction->setIcon(QIcon(":/images/copy.png"));
	connect(copyAction, SIGNAL(triggered()), this, SLOT(copyImages()));
	copyAction->setEnabled(false);
	
	deleteAction = new QAction("Delete", this);
	deleteAction->setShortcut(QKeySequence::Delete);
	deleteAction->setIcon(QIcon(":/images/delete.png"));
	connect(deleteAction, SIGNAL(triggered()), this, SLOT(deleteOp()));

	saveAction = new QAction("Save", this);
	saveAction->setShortcut(QKeySequence::Save);

	saveAsAction = new QAction("Save As", this);

	copyToAction = new QAction("Copy to", this);
	moveToAction = new QAction("Move to", this);

	renameAction = new QAction("Rename", this);
	renameAction->setShortcut(QKeySequence("F2"));
	connect(renameAction, SIGNAL(triggered()), this, SLOT(rename()));

	selectAllAction = new QAction("Select All", this);
	connect(selectAllAction, SIGNAL(triggered()), this, SLOT(selectAllThumbs()));

	aboutAction = new QAction("&About", this);
	aboutAction->setIcon(QIcon(":/images/about.png"));
	connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

	aboutQtAction = new QAction("About &Qt", this);
	connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

	// Sort actions
	actName = new QAction("Name", this);
	actTime = new QAction("Time", this);
	actSize = new QAction("Size", this);
	actType = new QAction("Type", this);
	actReverse = new QAction("Reverse", this);
	actName->setCheckable(true);
	actTime->setCheckable(true);
	actSize->setCheckable(true);
	actType->setCheckable(true);
	actReverse->setCheckable(true);
	connect(actName, SIGNAL(triggered()), this, SLOT(sortThumbnains()));
	connect(actTime, SIGNAL(triggered()), this, SLOT(sortThumbnains()));
	connect(actSize, SIGNAL(triggered()), this, SLOT(sortThumbnains()));
	connect(actType, SIGNAL(triggered()), this, SLOT(sortThumbnains()));
	connect(actReverse, SIGNAL(triggered()), this, SLOT(sortThumbnains()));

	actName->setChecked(thumbView->thumbsSortFlags == QDir::Name || 
						thumbView->thumbsSortFlags == QDir::Reversed); 
	actTime->setChecked(thumbView->thumbsSortFlags & QDir::Time); 
	actSize->setChecked(thumbView->thumbsSortFlags & QDir::Size); 
	actType->setChecked(thumbView->thumbsSortFlags & QDir::Type); 
	actReverse->setChecked(thumbView->thumbsSortFlags & QDir::Reversed); 

	actClassic = new QAction("Classic view", this);
	actCompact = new QAction("Compact", this);
	actSquarish = new QAction("Squarish", this);
	connect(actClassic, SIGNAL(triggered()), this, SLOT(setClassicThumbs()));
	connect(actCompact, SIGNAL(triggered()), this, SLOT(setCompactThumbs()));
	connect(actSquarish, SIGNAL(triggered()), this, SLOT(setSquarishThumbs()));
	actClassic->setCheckable(true);
	actCompact->setCheckable(true);
	actSquarish->setCheckable(true);
	actClassic->setChecked(GData::thumbsLayout == ThumbView::Classic); 
	actCompact->setChecked(GData::thumbsLayout == ThumbView::Compact); 
	actSquarish->setChecked(GData::thumbsLayout == ThumbView::Squares); 

	refreshAction = new QAction("Refresh", this);
	refreshAction->setShortcut(QKeySequence("F5"));
	refreshAction->setIcon(QIcon(":/images/refresh.png"));
	connect(refreshAction, SIGNAL(triggered()), this, SLOT(refreshThumbs()));

	pasteAction = new QAction("&Paste Here", this);
	pasteAction->setShortcut(QKeySequence::Paste);
	pasteAction->setIcon(QIcon(":/images/paste.png"));    
	connect(pasteAction, SIGNAL(triggered()), this, SLOT(pasteImages()));
	pasteAction->setEnabled(false);
	
	createDirAction = new QAction("&New Folder", this);
	connect(createDirAction, SIGNAL(triggered()), this, SLOT(createSubDirectory()));
	createDirAction->setIcon(QIcon(":/images/new_folder.png"));
	
	manageDirAction = new QAction("&Manage", this);
	connect(manageDirAction, SIGNAL(triggered()), this, SLOT(manageDir()));

	goBackAction = new QAction("&Back", this);
	goBackAction->setIcon(QIcon(":/images/back.png"));
	connect(goBackAction, SIGNAL(triggered()), this, SLOT(goBack()));
	goBackAction->setEnabled(false);
	goBackAction->setShortcut(QKeySequence::Back);
	// goBackAction->setToolTip("Back");

	goFrwdAction = new QAction("&Forward", this);
	goFrwdAction->setIcon(QIcon(":/images/next.png"));
	connect(goFrwdAction, SIGNAL(triggered()), this, SLOT(goForward()));
	goFrwdAction->setEnabled(false);
	goFrwdAction->setShortcut(QKeySequence::Forward);

	goUpAction = new QAction("&Up", this);
	goUpAction->setIcon(QIcon(":/images/up.png"));
	connect(goUpAction, SIGNAL(triggered()), this, SLOT(goUp()));

	goHomeAction = new QAction("&Home", this);
	connect(goHomeAction, SIGNAL(triggered()), this, SLOT(goHome()));	
	goHomeAction->setIcon(QIcon(":/images/home.png"));

	nextImageAction = new QAction("&Next", this);
	nextImageAction->setIcon(QIcon(":/images/next.png"));
	nextImageAction->setShortcut(QKeySequence::MoveToNextPage);
	connect(nextImageAction, SIGNAL(triggered()), this, SLOT(loadNextImage()));
	
	prevImageAction = new QAction("&Previous", this);
	prevImageAction->setIcon(QIcon(":/images/back.png"));
	prevImageAction->setShortcut(QKeySequence::MoveToPreviousPage);
	connect(prevImageAction, SIGNAL(triggered()), this, SLOT(loadPrevImage()));

	firstImageAction = new QAction("&First", this);
	firstImageAction->setIcon(QIcon(":/images/first.png"));
	firstImageAction->setShortcut(QKeySequence::MoveToStartOfLine);
	connect(firstImageAction, SIGNAL(triggered()), this, SLOT(loadFirstImage()));

	lastImageAction = new QAction("&Last", this);
	lastImageAction->setIcon(QIcon(":/images/last.png"));
	lastImageAction->setShortcut(QKeySequence::MoveToEndOfLine);
	connect(lastImageAction, SIGNAL(triggered()), this, SLOT(loadLastImage()));

	openImageAction = new QAction("Open", this);
	openImageAction->setShortcut(QKeySequence::InsertParagraphSeparator);
	connect(openImageAction, SIGNAL(triggered()), this, SLOT(loadImagefromAction()));

	zoomOutAct = new QAction("Zoom Out", this);
	zoomOutAct->setShortcut(QKeySequence("-"));
	connect(zoomOutAct, SIGNAL(triggered()), this, SLOT(zoomOut()));
	zoomOutAct->setIcon(QIcon(":/images/zoom_out.png"));

	zoomInAct = new QAction("Zoom In", this);
	zoomInAct->setShortcut(QKeySequence("+"));
	connect(zoomInAct, SIGNAL(triggered()), this, SLOT(zoomIn()));
	zoomInAct->setIcon(QIcon(":/images/zoom_out.png"));

	resetZoomAct = new QAction("Reset Zoom", this);
	resetZoomAct->setShortcut(QKeySequence("*"));
	connect(resetZoomAct, SIGNAL(triggered()), this, SLOT(resetZoom()));

	origZoomAct = new QAction("Original Size", this);
	origZoomAct->setIcon(QIcon(":/images/zoom1.png"));
	origZoomAct->setShortcut(QKeySequence("/"));
	connect(origZoomAct, SIGNAL(triggered()), this, SLOT(origZoom()));

	keepZoomAct = new QAction("Keep Zoom", this);
	keepZoomAct->setCheckable(true);
	connect(keepZoomAct, SIGNAL(triggered()), this, SLOT(keepZoom()));

	rotateLeftAct = new QAction("Rotate Left", this);
	rotateLeftAct->setIcon(QIcon(":/images/rotate_left.png"));
	connect(rotateLeftAct, SIGNAL(triggered()), this, SLOT(rotateLeft()));
	rotateLeftAct->setShortcut(QKeySequence::MoveToPreviousWord);

	rotateRightAct = new QAction("Rotate Right", this);
	rotateRightAct->setIcon(QIcon(":/images/rotate_right.png"));
	connect(rotateRightAct, SIGNAL(triggered()), this, SLOT(rotateRight()));
	rotateRightAct->setShortcut(QKeySequence::MoveToNextWord);

	flipHAct = new QAction("Flip Horizontally", this);
	connect(flipHAct, SIGNAL(triggered()), this, SLOT(flipHoriz()));
	flipHAct->setShortcut(QKeySequence("Ctrl+Down"));

	flipVAct = new QAction("Flip Vertically", this);
	connect(flipVAct, SIGNAL(triggered()), this, SLOT(flipVert()));
	flipVAct->setShortcut(QKeySequence("Ctrl+Up"));

	cropAct = new QAction("Cropping", this);
	connect(cropAct, SIGNAL(triggered()), this, SLOT(cropImage()));
	cropAct->setShortcut(QKeySequence("Ctrl+R"));

	mirrorDisabledAct = new QAction("Disabled", this);
	mirrorDisabledAct->setShortcut(QKeySequence("Ctrl+1"));
	mirrorDualAct = new QAction("Dual", this);
	mirrorDualAct->setShortcut(QKeySequence("Ctrl+2"));
	mirrorTripleAct = new QAction("Triple", this);
	mirrorTripleAct->setShortcut(QKeySequence("Ctrl+3"));
	mirrorVDualAct = new QAction("Dual Vertical", this);
	mirrorVDualAct->setShortcut(QKeySequence("Ctrl+4"));
	mirrorQuadAct = new QAction("Quad", this);
	mirrorQuadAct->setShortcut(QKeySequence("Ctrl+5"));

	mirrorDisabledAct->setCheckable(true);
	mirrorDualAct->setCheckable(true);
	mirrorTripleAct->setCheckable(true);
	mirrorVDualAct->setCheckable(true);
	mirrorQuadAct->setCheckable(true);
	connect(mirrorDisabledAct, SIGNAL(triggered()), this, SLOT(setMirrorDisabled()));
	connect(mirrorDualAct, SIGNAL(triggered()), this, SLOT(setMirrorDual()));
	connect(mirrorTripleAct, SIGNAL(triggered()), this, SLOT(setMirrorTriple()));
	connect(mirrorVDualAct, SIGNAL(triggered()), this, SLOT(setMirrorVDual()));
	connect(mirrorQuadAct, SIGNAL(triggered()), this, SLOT(setMirrorQuad()));
	mirrorDisabledAct->setChecked(true); 

	keepTransformAct = new QAction("Keep Transformations", this);
	keepTransformAct->setCheckable(true);
	connect(keepTransformAct, SIGNAL(triggered()), this, SLOT(keepTransformClicked()));
}

void Phototonic::createMenus()
{
	fileMenu = menuBar()->addMenu("&File");
	fileMenu->addAction(createDirAction);
	fileMenu->addSeparator();
	fileMenu->addAction(exitAction);

	editMenu = menuBar()->addMenu("&Edit");
	editMenu->addAction(cutAction);
	editMenu->addAction(copyAction);
	editMenu->addAction(renameAction);
	editMenu->addAction(deleteAction);
	editMenu->addSeparator();
	editMenu->addAction(pasteAction);
	editMenu->addSeparator();
	editMenu->addAction(selectAllAction);
	editMenu->addSeparator();
	editMenu->addAction(settingsAction);

	goMenu = menuBar()->addMenu("&Go");
	goMenu->addAction(goBackAction);
	goMenu->addAction(goFrwdAction);
	goMenu->addAction(goUpAction);
	goMenu->addAction(goHomeAction);
	goMenu->addSeparator();
	goMenu->addAction(thumbsGoTopAct);
	goMenu->addAction(thumbsGoBottomAct);

	viewMenu = menuBar()->addMenu("&View");
	viewMenu->addAction(thumbsZoomInAct);
	viewMenu->addAction(thumbsZoomOutAct);
	sortMenu = viewMenu->addMenu("&Sort By");
	sortTypesGroup = new QActionGroup(this);
	sortTypesGroup->addAction(actName);
	sortTypesGroup->addAction(actTime);
	sortTypesGroup->addAction(actSize);
	sortTypesGroup->addAction(actType);
	sortMenu->addActions(sortTypesGroup->actions());
	sortMenu->addSeparator();
	sortMenu->addAction(actReverse);
	viewMenu->addSeparator();

	thumbLayoutsGroup = new QActionGroup(this);
	thumbLayoutsGroup->addAction(actClassic);
	thumbLayoutsGroup->addAction(actCompact);
	thumbLayoutsGroup->addAction(actSquarish);
	viewMenu->addActions(thumbLayoutsGroup->actions());

	viewMenu->addSeparator();
	viewMenu->addAction(refreshAction);

	menuBar()->addSeparator();
	helpMenu = menuBar()->addMenu("&Help");
	helpMenu->addAction(aboutAction);
	helpMenu->addAction(aboutQtAction);

	// thumbview context menu
	thumbView->addAction(openImageAction);
	thumbView->addAction(cutAction);
	thumbView->addAction(copyAction);
	thumbView->addAction(renameAction);
	thumbView->addAction(deleteAction);
	QAction *sep = new QAction(this);
	sep->setSeparator(true);
	thumbView->addAction(sep);
	thumbView->addAction(pasteAction);
	sep = new QAction(this);
	sep->setSeparator(true);
	thumbView->addAction(sep);
	thumbView->addAction(selectAllAction);
	thumbView->setContextMenuPolicy(Qt::ActionsContextMenu);
}

void Phototonic::createToolBars()
{
	editToolBar = addToolBar("Edit");
	editToolBar->addAction(cutAction);
	editToolBar->addAction(copyAction);
	editToolBar->addAction(pasteAction);
	editToolBar->addAction(deleteAction);
	editToolBar->setObjectName("Edit");

	goToolBar = addToolBar("Navigation");
	goToolBar->addAction(goBackAction);
	goToolBar->addAction(goFrwdAction);
	goToolBar->addAction(goUpAction);
	goToolBar->addAction(goHomeAction);
	goToolBar->setObjectName("Navigation");

	// path bar
	pathBar = new QLineEdit;
	pathComplete = new QCompleter(this);
	QDirModel *pathCompleteDirMod = new QDirModel;
	pathCompleteDirMod->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
	pathComplete->setModel(pathCompleteDirMod);
	pathBar->setCompleter(pathComplete);
	pathBar->setMinimumWidth(400);
	pathBar->setMaximumWidth(600);
	connect(pathBar, SIGNAL(returnPressed()), this, SLOT(goPathBarDir()));
	goToolBar->addWidget(pathBar);

	viewToolBar = addToolBar("View");
	viewToolBar->addAction(thumbsZoomInAct);
	viewToolBar->addAction(thumbsZoomOutAct);
	viewToolBar->setObjectName("View");

	viewToolBarWasVisible = GData::appSettings->value("viewToolBarWasVisible").toBool();
	editToolBarWasVisible = GData::appSettings->value("editToolBarWasVisible").toBool();
	goToolBarWasVisible = GData::appSettings->value("goToolBarWasVisible").toBool();
}

void Phototonic::createStatusBar()
{
    stateLabel = new QLabel("Initializing...");
	statusBar()->addWidget(stateLabel);
}

void Phototonic::createFSTree()
{
	fsModel = new QFileSystemModel;
	fsModel->setRootPath("");
	fsModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);

	fsTree = new FSTree(this);

	QAction *sep1 = new QAction(this);
	QAction *sep2 = new QAction(this);
	sep1->setSeparator(true);
	sep2->setSeparator(true);

	// Context menu
	fsTree->addAction(createDirAction);
	fsTree->addAction(renameAction);
	fsTree->addAction(deleteAction);
	fsTree->addAction(sep1);
	fsTree->addAction(pasteAction);
	fsTree->addAction(sep2);
	fsTree->addAction(manageDirAction);
	fsTree->setContextMenuPolicy(Qt::ActionsContextMenu);

	fsTree->setDragEnabled(true);
	fsTree->setAcceptDrops(true);
	fsTree->setDragDropMode(QAbstractItemView::InternalMove);

	
	fsTree->setModel(fsModel);
	for (int i = 1; i <= 3; i++)
		fsTree->hideColumn(i);
	fsTree->setHeaderHidden(true);
	connect(fsTree, SIGNAL(clicked(const QModelIndex&)),
				this, SLOT(goSelectedDir(const QModelIndex &)));

	connect(fsModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
				this, SLOT(checkDirState(const QModelIndex &, int, int)));

	connect(fsTree, SIGNAL(dropOp(Qt::KeyboardModifiers, bool, QString)),
				this, SLOT(dropOp(Qt::KeyboardModifiers, bool, QString)));
	QModelIndex idx = fsModel->index(QDir::currentPath()); 
	fsTree->setCurrentIndex(idx);
}

void Phototonic::setFlagsByQAction(QAction *act, QDir::SortFlags SortFlag)
{
	if (act->isChecked())
		thumbView->thumbsSortFlags |= SortFlag;
}

void Phototonic::sortThumbnains()
{
	thumbView->thumbsSortFlags = 0;
	setFlagsByQAction(actName, QDir::Name);
	setFlagsByQAction(actTime, QDir::Time);
	setFlagsByQAction(actSize, QDir::Size);
	setFlagsByQAction(actType, QDir::Type);
	setFlagsByQAction(actReverse, QDir::Reversed);
	refreshThumbs(false);
}

void Phototonic::refreshThumbs()
{
	refreshThumbs(false);
}

void Phototonic::refreshThumbs(bool scrollToTop)
{
	if (scrollToTop)
	{
		thumbView->setNeedScroll(true);
		QTimer::singleShot(0, this, SLOT(reloadThumbsSlot()));
	}
	else
	{
		thumbView->setNeedScroll(false);
		QTimer::singleShot(0, this, SLOT(reloadThumbsSlot()));
	}
}

void Phototonic::setClassicThumbs()
{
	GData::thumbsLayout = ThumbView::Classic;
	refreshThumbs(false);
}

void Phototonic::setCompactThumbs()
{
	GData::thumbsLayout = ThumbView::Compact;
	refreshThumbs(false);
}

void Phototonic::setSquarishThumbs()
{
	GData::thumbsLayout = ThumbView::Squares;
	refreshThumbs(false);
}

void Phototonic::about()
{
	QMessageBox::about(this, "About Phototonic", "<h2>Phototonic v0.90</h2>"
							"<p>Image viewer and organizer</p>"
							"<p><a href=\"https://gitorious.org/phototonic\">Home page</a></p>"
							"<p><a href=\"https://github.com/oferkv/phototonic/issues\">Bug reports</a></p>"
							"<p>Copyright &copy; 2013 Ofer Kashayov</p>");
}

void Phototonic::showSettings()
{
	if(isFullScreen())
		imageView->setCursorOverrides(false);
	
	SettingsDialog *dialog = new SettingsDialog(this);
	if (dialog->exec())
	{
		imageView->setPalette(QPalette(GData::backgroundColor));
		thumbView->setThumbColors();
		GData::imageZoomFactor = 1.0;

		if (stackedWidget->currentIndex() == imageViewIdx)
		{
			imageView->resizeImage();
			newSettingsRefreshThumbs = true;
		}
		else
			refreshThumbs(false);
	}

	delete dialog;

	if(isFullScreen())
		imageView->setCursorOverrides(true);
}

void Phototonic::toggleFullScreen()
{
	if (fullScreenAct->isChecked())
	{
		shouldMaximize = isMaximized();
		showFullScreen();
		GData::isFullScreen = true;
		imageView->setCursorHiding(true);
	}
	else
	{
		showNormal();
		if (shouldMaximize)
			showMaximized();
		imageView->setCursorHiding(false);
		GData::isFullScreen = false;
	}
}

void Phototonic::selectAllThumbs()
{
	thumbView->selectAll();
}

void Phototonic::createCopyCutFileList()
{
	GData::copyCutFileList.clear();
	for (int tn = 0; tn < copyCutCount; tn++)
	{
		QString sourceFile = thumbView->currentViewDir + QDir::separator() +
			thumbView->thumbViewModel->item(GData::copyCutIdxList[tn].row())->data(thumbView->FileNameRole).toString();
		GData::copyCutFileList.append(sourceFile);
	}
}

void Phototonic::cutImages()
{
	GData::copyCutIdxList = thumbView->selectionModel()->selectedIndexes();
	copyCutCount = GData::copyCutIdxList.size();
	createCopyCutFileList();
	GData::copyOp = false;
	pasteAction->setEnabled(true);
}

void Phototonic::copyImages()
{
	GData::copyCutIdxList = thumbView->selectionModel()->selectedIndexes();
	copyCutCount = GData::copyCutIdxList.size();
	createCopyCutFileList();
	GData::copyOp = true;
	pasteAction->setEnabled(true);
}

void Phototonic::thumbsZoomIn()
{
	if (thumbView->thumbSize < 300)
	{
		thumbView->thumbSize += 50;
		thumbsZoomOutAct->setEnabled(true);
		if (thumbView->thumbSize == 300)
			thumbsZoomInAct->setEnabled(false);
		refreshThumbs(false);
	}
}

void Phototonic::thumbsZoomOut()
{
	if (thumbView->thumbSize > 100)
	{
		thumbView->thumbSize -= 50;
		thumbsZoomInAct->setEnabled(true);
		if (thumbView->thumbSize == 100)
			thumbsZoomOutAct->setEnabled(false);
		refreshThumbs(false);
	}
}

void Phototonic::zoomOut()
{
	GData::imageZoomFactor -= (GData::imageZoomFactor <= 0.25)? 0 : 0.25;
	imageView->tempDisableResize = false;
	imageView->resizeImage();
}

void Phototonic::zoomIn()
{
	GData::imageZoomFactor += (GData::imageZoomFactor >= 3.25)? 0 : 0.25;
	imageView->tempDisableResize = false;
	imageView->resizeImage();
}

void Phototonic::resetZoom()
{
	GData::imageZoomFactor = 1.0;
	imageView->tempDisableResize = false;
	imageView->resizeImage();
}

void Phototonic::origZoom()
{
	GData::imageZoomFactor = 1.0;
	imageView->tempDisableResize = true;
	imageView->resizeImage();
}

void Phototonic::keepZoom()
{
	GData::keepZoomFactor = keepZoomAct->isChecked();
}

void Phototonic::keepTransformClicked()
{
	GData::keepTransform = keepTransformAct->isChecked();
}

void Phototonic::rotateLeft()
{
	GData::rotation -= 90;
	if (GData::rotation < 0)
		GData::rotation = 270;
	imageView->refresh();
}

void Phototonic::rotateRight()
{
	GData::rotation += 90;
	if (GData::rotation > 270)
		GData::rotation = 0;
	imageView->refresh();
}

void Phototonic::flipVert()
{
	GData::flipV = !GData::flipV;
	imageView->refresh();
}

void Phototonic::cropImage()
{
	CropDialog *dialog = new CropDialog(this, imageView);

	imageView->setCursorOverrides(false);
	dialog->exec();
	imageView->setCursorOverrides(true);
}

void Phototonic::flipHoriz()
{
	GData::flipH = !GData::flipH;
	imageView->refresh();
}

void Phototonic::setMirrorDisabled()
{
	imageView->mirrorLayout = ImageView::LayNone;
	imageView->refresh();
}

void Phototonic::setMirrorDual()
{
	imageView->mirrorLayout = ImageView::LayDual;
	imageView->refresh();
}

void Phototonic::setMirrorTriple()
{
	imageView->mirrorLayout = ImageView::LayTriple;
	imageView->refresh();
}

void Phototonic::setMirrorVDual()
{
	imageView->mirrorLayout = ImageView::LayVDual;
	imageView->refresh();
}

void Phototonic::setMirrorQuad()
{
	imageView->mirrorLayout = ImageView::LayQuad;
	imageView->refresh();
}

bool Phototonic::isValidPath(QString &path)
{
	QDir checkPath(path);
	if (!checkPath.exists() || !checkPath.isReadable())
	{
		return false;
	}
	return true;
}

void Phototonic::pasteImages()
{
	if (!copyCutCount)
		return;

	QString destDir = getSelectedPath();
	if (!isValidPath(destDir))
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Can not paste in " + destDir);
		restoreCurrentIdx();
		return;
	}

	if (thumbViewBusy)
	{
		abortThumbsLoad();
	}
	
	CpMvDialog *dialog = new CpMvDialog(this);
	bool pasteInCurrDir = (thumbView->currentViewDir == destDir);
		
	dialog->exec(thumbView, destDir, pasteInCurrDir);
	QString state = QString((GData::copyOp? "Copied " : "Moved ") + 
								QString::number(dialog->nfiles) + " images");
	updateState(state);

	delete(dialog);

	restoreCurrentIdx();

	copyCutCount = 0;
	GData::copyCutIdxList.clear();
	GData::copyCutFileList.clear();
	pasteAction->setEnabled(false);

	refreshThumbs(false);
}

void Phototonic::deleteSingleImage()
{
	bool ok;
	int ret;

	ret = QMessageBox::warning(this, "Delete image", "Permanently delete this image?",
									QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

	int currentRow = thumbView->getCurrentRow();
	if (ret == QMessageBox::Yes)
	{
		ok = QFile::remove(thumbView->currentViewDir + QDir::separator() + 
			thumbView->thumbViewModel->item(currentRow)->data(thumbView->FileNameRole).toString());

		if (ok)
		{
			 thumbView->thumbViewModel->removeRow(currentRow);
		}
		else
		{
			QMessageBox msgBox;
			msgBox.critical(this, "Error", "failed to delete image");
			return;
		}

		thumbView->setCurrentRow(currentRow - 1);
		if (thumbView->getNextRow() > currentRow)
			loadPrevImage();
		else
			loadNextImage();
	}
}

void Phototonic::deleteOp()
{
	if (QApplication::focusWidget() == fsTree)
	{
		deleteDir();
		return;
	}

	if (stackedWidget->currentIndex() == imageViewIdx)
	{
		deleteSingleImage();
		return;
	}

	bool ok;
	int ret = QMessageBox::warning(this, "Delete images", "Permanently delete selected images?",
										QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
	QModelIndexList indexesList;
	int nfiles = 0;
	if (ret == QMessageBox::Yes)
	{

		if (thumbViewBusy)
			abortThumbsLoad();

		while((indexesList = thumbView->selectionModel()->selectedIndexes()).size())
		{
			ok = QFile::remove(thumbView->currentViewDir + QDir::separator() + 
				thumbView->thumbViewModel->item(indexesList.first().row())->data(thumbView->FileNameRole).toString());
				
			nfiles++;
			if (ok)
			{
				 thumbView->thumbViewModel->removeRow(indexesList.first().row());
			}
			else
			{
				QMessageBox msgBox;
				msgBox.critical(this, "Error", "failed to delete image");
				return;
			}
		}
		
		QString state = QString("Deleted " + QString::number(nfiles) + " images");
		updateState(state);

		if (thumbViewBusy)
			refreshThumbs(false);
	}
}

void Phototonic::goTo(QString path)
{
	thumbView->setNeedScroll(true);
	fsTree->setCurrentIndex(fsModel->index(path));
	thumbView->currentViewDir = path;
	refreshThumbs(true);
}

void Phototonic::goSelectedDir(const QModelIndex&)
{
	thumbView->setNeedScroll(true);
	thumbView->currentViewDir = getSelectedPath();
	refreshThumbs(true);
}

void Phototonic::goPathBarDir()
{
	thumbView->setNeedScroll(true);

	QDir checkPath(pathBar->text());
	if (!checkPath.exists() || !checkPath.isReadable())
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Invalid Path: " + thumbView->currentViewDir);
		pathBar->setText(thumbView->currentViewDir);
		return;
	}
	
	thumbView->currentViewDir = pathBar->text();
	restoreCurrentIdx();
	refreshThumbs(true);
}

void Phototonic::goBack()
{
	if (currentHistoryIdx > 0)
	{
		needHistoryRecord = false;
		goTo(pathHistory.at(--currentHistoryIdx));
		goFrwdAction->setEnabled(true);
		if (currentHistoryIdx == 0)
			goBackAction->setEnabled(false);
	}
}

void Phototonic::goForward()
{

	if (currentHistoryIdx < pathHistory.size() - 1)
	{
		needHistoryRecord = false;
		goTo(pathHistory.at(++currentHistoryIdx));
		if (currentHistoryIdx == (pathHistory.size() - 1))
			goFrwdAction->setEnabled(false);
	}
}

void Phototonic::goUp()
{
	QString parentDir = 
			thumbView->currentViewDir.left(thumbView->currentViewDir.lastIndexOf(QDir::separator()));

	if (parentDir.size() == 0)
		parentDir = QDir::separator();
	goTo(parentDir);
}

void Phototonic::goHome()
{
	goTo(QDir::homePath());
}

void Phototonic::setCopyCutActions(bool setEnabled)
{
	cutAction->setEnabled(setEnabled);
	copyAction->setEnabled(setEnabled);
}

void Phototonic::changeActionsBySelection(const QItemSelection&, const QItemSelection&)
{
	if (thumbView->selectionModel()->selectedIndexes().size())
		setCopyCutActions(true);
	else
		setCopyCutActions(false);
}

void Phototonic::updateActions(QWidget*, QWidget *selectedWidget)
{
	if (selectedWidget == fsTree)
		setCopyCutActions(false);
	else if (selectedWidget == thumbView)
	{
		if (thumbView->selectionModel()->selectedIndexes().size())
			setCopyCutActions(true);
	}
}

void Phototonic::writeSettings()
{
	if (stackedWidget->currentIndex() == imageViewIdx)
		setThumbViewWidgetsVisible(true);

	if(isFullScreen())
	{
		showNormal();
		if (shouldMaximize)
			showMaximized();
	}

	QApplication::processEvents();
	GData::appSettings->setValue("geometry", saveGeometry());
	GData::appSettings->setValue("mainWindowState", saveState());
	GData::appSettings->setValue("splitterSizes", splitter->saveState());
	GData::appSettings->setValue("thumbsSortFlags", (int)thumbView->thumbsSortFlags);
	GData::appSettings->setValue("thumbsZoomVal", (int)thumbView->thumbSize);
	GData::appSettings->setValue("isFullScreen", (bool)GData::isFullScreen);
	GData::appSettings->setValue("backgroundColor", GData::backgroundColor);
	GData::appSettings->setValue("backgroundThumbColor", GData::thumbsBackgroundColor);
	GData::appSettings->setValue("textThumbColor", GData::thumbsTextColor);
	GData::appSettings->setValue("thumbSpacing", (int)GData::thumbSpacing);
	GData::appSettings->setValue("thumbLayout", (int)GData::thumbsLayout);
	GData::appSettings->setValue("viewToolBarWasVisible", (bool)viewToolBar->isVisible());
	GData::appSettings->setValue("editToolBarWasVisible", (bool)editToolBar->isVisible());
	GData::appSettings->setValue("goToolBarWasVisible", (bool)goToolBar->isVisible());
	GData::appSettings->setValue("exitInsteadOfClose", (int)GData::exitInsteadOfClose);
	GData::appSettings->setValue("imageZoomFactor", (float)GData::imageZoomFactor);
	GData::appSettings->setValue("shouldMaximize", (bool)isMaximized());
	GData::appSettings->setValue("defaultSaveQuality", (int)GData::defaultSaveQuality);
}

void Phototonic::loadDefaultSettings()
{
	initComplete = false;
	newSettingsRefreshThumbs = false;

	if (!GData::appSettings->contains("thumbsZoomVal"))
	{
		resize(800, 600);
		GData::appSettings->setValue("thumbsSortFlags", (int)0);
		GData::appSettings->setValue("thumbsZoomVal", (int)200);
		GData::appSettings->setValue("isFullScreen", (bool)false);
		GData::appSettings->setValue("backgroundColor", QColor(25, 25, 25));
		GData::appSettings->setValue("backgroundThumbColor", QColor(200, 200, 200));
		GData::appSettings->setValue("textThumbColor", QColor(25, 25, 25));
		GData::appSettings->setValue("thumbSpacing", (int)5);
		GData::appSettings->setValue("thumbLayout", (int)GData::thumbsLayout);
		GData::appSettings->setValue("viewToolBarWasVisible", (bool)true);
		GData::appSettings->setValue("editToolBarWasVisible", (bool)true);
		GData::appSettings->setValue("goToolBarWasVisible", (bool)true);
		GData::appSettings->setValue("zoomOutFlags", (int)1);
		GData::appSettings->setValue("zoomInFlags", (int)0);
		GData::appSettings->setValue("exitInsteadOfClose", (int)0);
		GData::appSettings->setValue("imageZoomFactor", (float)1.0);
		GData::appSettings->setValue("defaultSaveQuality", (int)75);
	}

	GData::exitInsteadOfClose = GData::appSettings->value("exitInsteadOfClose").toBool();
	GData::imageZoomFactor = GData::appSettings->value("imageZoomFactor").toFloat();
	GData::zoomOutFlags = GData::appSettings->value("zoomOutFlags").toInt();
	GData::zoomInFlags = GData::appSettings->value("zoomInFlags").toInt();
	GData::rotation = 0;
	GData::keepTransform = false;
	shouldMaximize = GData::appSettings->value("shouldMaximize").toBool();
	GData::flipH = false;
	GData::flipV = false;
	GData::defaultSaveQuality = GData::appSettings->value("defaultSaveQuality").toInt();
}

void Phototonic::closeEvent(QCloseEvent *event)
{
	abortThumbsLoad();
	writeSettings();
	event->accept();
}

void Phototonic::updateState(QString state)
{
	stateLabel->setText(state);
}

void Phototonic::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		if (stackedWidget->currentIndex() == imageViewIdx)
		{
			closeImage();
		    event->accept();
		}
	}
}

void Phototonic::mousePressEvent(QMouseEvent *event)
{
	if (stackedWidget->currentIndex() == imageViewIdx)
	{
		if (event->button() == Qt::MiddleButton)
		{
			fullScreenAct->setChecked(!(fullScreenAct->isChecked()));
				
			toggleFullScreen();
			event->accept();
		}
		else if (event->button() == Qt::LeftButton)
		{
			imageView->setMouseMoveData(true, event->x(), event->y());
			QApplication::setOverrideCursor(Qt::ClosedHandCursor);
			event->accept();
		}
	}
}

void Phototonic::mouseReleaseEvent(QMouseEvent *event)
{
	if (stackedWidget->currentIndex() == imageViewIdx)
	{
		if (event->button() == Qt::LeftButton)
		{
			imageView->setMouseMoveData(false, 0, 0);
			QApplication::setOverrideCursor(Qt::OpenHandCursor);
		}
	}
}

void Phototonic::setThumbViewWidgetsVisible(bool visible)
{
	menuBar()->setVisible(visible);
	editToolBar->setVisible(visible? editToolBarWasVisible : false);
	goToolBar->setVisible(visible? goToolBarWasVisible : false);
	viewToolBar->setVisible(visible? viewToolBarWasVisible : false);
	statusBar()->setVisible(visible);
}

void Phototonic::loadImageFile(QString imageFileName)
{
	if (stackedWidget->currentIndex() != imageViewIdx)
	{
		setThumbViewWidgetsVisible(false);
		if (GData::isFullScreen == true)
		{
			shouldMaximize = isMaximized();
			showFullScreen();
			imageView->setCursorHiding(true);
		}
		stackedWidget->setCurrentIndex(imageViewIdx);
	}
	imageView->loadImage(thumbView->currentViewDir, imageFileName);
	QApplication::setOverrideCursor(Qt::OpenHandCursor);
	setWindowTitle(imageFileName + " - Phototonic");
}

void Phototonic::loadImagefromThumb(const QModelIndex &idx)
{
	thumbView->setCurrentRow(idx.row());
	loadImageFile(thumbView->thumbViewModel->item(idx.row())->data(thumbView->FileNameRole).toString());
}

void Phototonic::loadImagefromAction()
{
	QModelIndexList indexesList;
	indexesList = thumbView->selectionModel()->selectedIndexes();

	if (indexesList.size() != 1)
		return;

	loadImagefromThumb(indexesList.first());
}

void Phototonic::loadImagefromCli()
{
	loadImageFile(cliFileName);
	thumbView->setCurrentIndexByName(cliFileName);
}

void Phototonic::loadNextImage()
{
	int nextRow = thumbView->getNextRow();
	loadImageFile(thumbView->thumbViewModel->item(nextRow)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(nextRow);
}

void Phototonic::loadPrevImage()
{
	int prevRow = thumbView->getPrevRow();
	loadImageFile(thumbView->thumbViewModel->item(prevRow)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(prevRow);
}

void Phototonic::loadFirstImage()
{
	loadImageFile(thumbView->thumbViewModel->item(0)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(0);
}

void Phototonic::loadLastImage()
{
	int lastRow = thumbView->getLastRow();
	loadImageFile(thumbView->thumbViewModel->item(lastRow)->data(thumbView->FileNameRole).toString());
	thumbView->setCurrentRow(lastRow);
}

void Phototonic::closeImage()
{
	if (cliImageLoaded && GData::exitInsteadOfClose)
		close();

	if(isFullScreen())
	{
		imageView->setCursorHiding(false);
		showNormal();
		if (shouldMaximize)
			showMaximized();
	}
	while (QApplication::overrideCursor())
		QApplication::restoreOverrideCursor();
	setThumbViewWidgetsVisible(true);
	stackedWidget->setCurrentIndex(thumbViewIdx);
	setWindowTitle(thumbView->currentViewDir + " - Phototonic");
	thumbView->setCurrentIndexByName(imageView->currentImage);
	thumbView->selectCurrentIndex();

	if (newSettingsRefreshThumbs)
	{
		newSettingsRefreshThumbs = false;
		refreshThumbs(false);
	}
}

void Phototonic::goBottom()
{
	thumbView->scrollToBottom();
}

void Phototonic::goTop()
{
	thumbView->scrollToTop();
}

void Phototonic::dropOp(Qt::KeyboardModifiers keyMods, bool dirOp, QString cpMvDirPath)
{
	QApplication::restoreOverrideCursor();
	GData::copyOp = (keyMods == Qt::ControlModifier);
	QMessageBox msgBox;

	QString destDir = getSelectedPath();
	if (!isValidPath(destDir))
	{
		msgBox.critical(this, "Error", "Can not move or copy images to this folder");
		restoreCurrentIdx();
		return;
	}
	
	if (destDir == 	thumbView->currentViewDir)
	{
		msgBox.critical(this, "Error", "Destination folder is same as source");
		return;
	}

	if (thumbViewBusy)
		abortThumbsLoad();

	if (dirOp)
	{
		QString dirOnly = cpMvDirPath.right(cpMvDirPath.size() - cpMvDirPath.lastIndexOf(QDir::separator()) - 1);
		QFile dir(cpMvDirPath);
		bool ok = dir.rename(destDir + QDir::separator() + dirOnly);
		if (!ok)
		{
			QMessageBox msgBox;
			msgBox.critical(this, "Error", "failed to move folder");
		}
		updateState("Folder moved");
	}
	else
	{
		CpMvDialog *cpMvdialog = new CpMvDialog(this);
		GData::copyCutIdxList = thumbView->selectionModel()->selectedIndexes();
		cpMvdialog->exec(thumbView, destDir, false);
		QString state = QString((GData::copyOp? "Copied " : "Moved ") + QString::number(cpMvdialog->nfiles) + " images");
		updateState(state);
		delete(cpMvdialog);
	}

	if (thumbViewBusy)
		refreshThumbs(false);
}

void Phototonic::restoreCurrentIdx()
{
	QModelIndex idx = fsModel->index(thumbView->currentViewDir); 
	if (idx.isValid())
		fsTree->setCurrentIndex(idx);
}

void Phototonic::checkDirState(const QModelIndex &, int, int)
{
	if (!initComplete)
	{
		return;
	}

	if (thumbViewBusy)
	{
		abortThumbsLoad();
	}

	if (!QDir().exists(thumbView->currentViewDir))
	{
		thumbView->currentViewDir = "";
		QTimer::singleShot(0, this, SLOT(reloadThumbsSlot()));
	}
}

void Phototonic::abortThumbsLoad()
{
	emit abortThumbLoading();
}

void Phototonic::recordHistory(QString dir)
{
	if (!needHistoryRecord)
	{
		needHistoryRecord = true;
		return;
	}

	if (pathHistory.size() && dir == pathHistory.at(currentHistoryIdx))
		return;
		
	pathHistory.insert(++currentHistoryIdx, dir);

	// Need to clear irrelevant items from list
	if (currentHistoryIdx != pathHistory.size() - 1)
	{	
		goFrwdAction->setEnabled(false);
		for (int i = pathHistory.size() - 1; i > currentHistoryIdx ; i--)
		{
			pathHistory.removeAt(i);
		}
	}
}

void Phototonic::reloadThumbsSlot()
{
	if (thumbViewBusy || !initComplete)
	{	
		abortThumbsLoad();
		QTimer::singleShot(250, this, SLOT(reloadThumbsSlot()));
		return;
	}

	if (thumbView->currentViewDir == "")
	{
		thumbView->currentViewDir = getSelectedPath();
		if (thumbView->currentViewDir == "")
			return;
	}

	QDir checkPath(thumbView->currentViewDir);
	if (!checkPath.exists() || !checkPath.isReadable())
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Failed to open folder: " + thumbView->currentViewDir);
		return;
	}

	pathBar->setText(thumbView->currentViewDir);
	recordHistory(thumbView->currentViewDir);
	if (currentHistoryIdx > 0)
		goBackAction->setEnabled(true);

	if (stackedWidget->currentIndex() == thumbViewIdx)
		setWindowTitle(thumbView->currentViewDir + " - Phototonic");

	thumbViewBusy = true;
	thumbView->load(cliFileName);
}

void Phototonic::renameDir()
{
	QModelIndexList selectedDirs = fsTree->selectionModel()->selectedRows();
	QFileInfo dirInfo = QFileInfo(fsModel->filePath(selectedDirs[0]));

	bool ok;
	QString title = "Rename " + dirInfo.completeBaseName();
	QString newDirName = QInputDialog::getText(this, title, 
							"New name:", QLineEdit::Normal, dirInfo.completeBaseName(), &ok);

	if (!ok)
	{
		restoreCurrentIdx();
		return;
	}

	if(newDirName.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Invalid name entered");
		restoreCurrentIdx();
		return;
	}

	QFile dir(dirInfo.absoluteFilePath());
	QString newFullPathName = dirInfo.absolutePath() + QDir::separator() + newDirName;
	ok = dir.rename(newFullPathName);
	if (!ok)
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "failed to rename folder");
		restoreCurrentIdx();
		return;
	}

	if (thumbView->currentViewDir == dirInfo.absoluteFilePath()) 
		fsTree->setCurrentIndex(fsModel->index(newFullPathName));
	else
		restoreCurrentIdx();
}

void Phototonic::rename()
{
	if (QApplication::focusWidget() == fsTree)
	{
		renameDir();
		return;
	}
		
	QModelIndexList indexesList;
	indexesList = thumbView->selectionModel()->selectedIndexes();

	if (indexesList.size() != 1)
		return;

	bool ok;
	QString renameImageName = 
		thumbView->thumbViewModel->item(indexesList.first().row())->data(thumbView->FileNameRole).toString();
	QString title = "Rename " + renameImageName;
	QString newImageName = QInputDialog::getText(this, title, 
										"New name:", QLineEdit::Normal, renameImageName, &ok);

	if (!ok)													
		return;

	if(newImageName.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Invalid name entered");
		return;
	}

	QString currnetFilePath = thumbView->currentViewDir + QDir::separator() + renameImageName;
	QFile currentFile(currnetFilePath);
	ok = currentFile.rename(thumbView->currentViewDir + QDir::separator() + newImageName);

	if (ok)
	{
		thumbView->thumbViewModel->item(indexesList.first().row())->setData(newImageName, thumbView->FileNameRole);
		if (GData::thumbsLayout == ThumbView::Classic)
			thumbView->thumbViewModel->item(indexesList.first().row())->setData(newImageName, Qt::DisplayRole);
	}
	else
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "failed to rename file");
	}
}

void Phototonic::deleteDir()
{
	bool ok = true;
	QModelIndexList selectedDirs = fsTree->selectionModel()->selectedRows();
	QString deletePath = fsModel->filePath(selectedDirs[0]);
	QModelIndex idxAbove = fsTree->indexAbove(selectedDirs[0]);
	QFileInfo dirInfo = QFileInfo(deletePath);
	QString question = "Permanently delete " + dirInfo.completeBaseName() + " and all of its contents?";

	int ret = QMessageBox::warning(this, "Delete folder", question,
								QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

	if (ret == QMessageBox::Yes)
		ok = removeDirOp(deletePath);
	else
	{
		restoreCurrentIdx();
		return;
	}

	if (!ok)
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "failed to delete folder");
		restoreCurrentIdx();
	}

	QString state = QString("Removed " + deletePath);
	updateState(state);

	if (thumbView->currentViewDir == deletePath) 
	{
		if (idxAbove.isValid())
			fsTree->setCurrentIndex(idxAbove);
	}
	else
		restoreCurrentIdx();
}

void Phototonic::createSubDirectory()
{
	QModelIndexList selectedDirs = fsTree->selectionModel()->selectedRows();
	QFileInfo dirInfo = QFileInfo(fsModel->filePath(selectedDirs[0]));

	bool ok;
	QString newDirName = QInputDialog::getText(this, "New Sub folder", 
							"New folder name:", QLineEdit::Normal, "", &ok);

	if (!ok)
	{
		restoreCurrentIdx();
		return;
	}

	if(newDirName.isEmpty())
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "Invalid name entered");
		restoreCurrentIdx();
		return;
	}

	QDir dir(dirInfo.absoluteFilePath());
	ok = dir.mkdir(dirInfo.absoluteFilePath() + QDir::separator() + newDirName);

	if (!ok)
	{
		QMessageBox msgBox;
		msgBox.critical(this, "Error", "failed to create new folder");
		restoreCurrentIdx();
		return;
	}

	updateState("Created " + newDirName);
	fsTree->expand(selectedDirs[0]);
	restoreCurrentIdx();
}

void Phototonic::manageDir()
{
	updateState("Opening file manager...");
	QDesktopServices::openUrl(QUrl("file:///" + getSelectedPath()));
}

QString Phototonic::getSelectedPath()
{
	QModelIndexList selectedDirs = fsTree->selectionModel()->selectedRows();
	if (selectedDirs.size() && selectedDirs[0].isValid())
	{
		QFileInfo dirInfo = QFileInfo(fsModel->filePath(selectedDirs[0]));
		return dirInfo.absoluteFilePath();
	}
	else
		return 0;
}

void Phototonic::wheelEvent(QWheelEvent *event)
{
	if (stackedWidget->currentIndex() == imageViewIdx)
	{
		if (event->delta() < 0)
			loadNextImage();
		else
			loadPrevImage();
	}
}

