//#include "editormanager.h"
#include "openmvplugin.h"

#include "app/app_version.h"

// for release, set VIEW_DEV to 0 
// for development, set VIEW_DEV to 1
#define VIEW_DEV 0

static void logLine(QString msg)
{
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir::root().mkpath(logDir);
    QDir::setCurrent(logDir);

    QFile logFile;
    logFile.setFileName(QStringLiteral("nxtcamview_log.txt"));
    if (!logFile.open(QIODevice::Append)) {
        return;
    }

    QTextStream logStream(&logFile);
    logStream << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz: "));
    logStream << msg << QChar(QChar::SpecialCharacter::LineFeed);
}

namespace OpenMV {
namespace Internal {

OpenMVPlugin::OpenMVPlugin() : IPlugin()
{
    qRegisterMetaType<OpenMVPluginSerialPortCommand>("OpenMVPluginSerialPortCommand");
    qRegisterMetaType<OpenMVPluginSerialPortCommandResult>("OpenMVPluginSerialPortCommandResult");

    m_ioport = new OpenMVPluginSerialPort(this);
    m_iodevice = new OpenMVPluginIO(m_ioport, this);

    m_frameSizeDumpTimer.start();
    m_getScriptRunningTimer.start();
    m_getTxBufferTimer.start();

    m_timer.start();
    m_queue = QQueue<qint64>();

    m_working = false;
    m_connected = false;
    m_running = false;
    m_major = int();
    m_minor = int();
    m_patch = int();
    m_portName = QString();
    m_portPath = QString();

    m_errorFilterRegex = QRegularExpression(QStringLiteral(
        "  File \"(.+?)\", line (\\d+).*?\n"
        "(?!Exception: IDE interrupt)(.+?:.+?)\n"));
    m_errorFilterString = QString();

    QTimer *timer = new QTimer(this);

    connect(timer, &QTimer::timeout,
            this, &OpenMVPlugin::processEvents);

    timer->start(1);
}

bool OpenMVPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    /* DGP 
    QSplashScreen *splashScreen = new QSplashScreen(QPixmap(QStringLiteral(SPLASH_PATH)));

    connect(Core::ICore::instance(), &Core::ICore::coreOpened,
            splashScreen, &QSplashScreen::deleteLater);

    splashScreen->show();
    DGP */

    m_feature = QStringLiteral("Object");

    return true;
}

void OpenMVPlugin::emitLoadColorMap()
{
    emit loadColorMap();
}

void OpenMVPlugin::loadFeatureFile()
{

    /* DGP: always the main.py from the camera SD card is opened.
     */
    Core::EditorManager *em = Core::EditorManager::instance();
    if ( em != NULL) {
        QString fileName =
            QDir::cleanPath(QDir::fromNativeSeparators(m_portPath)) + QStringLiteral("/main.py");
        QFileInfo fi(fileName);
        if (!fi.exists()) {
            logLine(QStringLiteral("file %1 is missing?").arg(fileName));
        } else {
            logLine(QStringLiteral("openmvplugin.cpp: loading initial file: main.py"));
            em->openEditor(fileName);
        }
    } else {
        logLine(QStringLiteral("em is null"));
    }
    showFeatureStatus();
}

void OpenMVPlugin::extensionsInitialized()
{
    //Id id;
    QApplication::setApplicationDisplayName(tr("NXTCamView5"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(ICON_PATH)));

    connect(Core::ActionManager::command(Core::Constants::NEW)->action(), &QAction::triggered, this, [this] {
        Core::EditorManager::cutForwardNavigationHistory();
        Core::EditorManager::addCurrentPositionToNavigationHistory();
        QString titlePattern = tr("untitled_$.py");
        TextEditor::BaseTextEditor *editor = qobject_cast<TextEditor::BaseTextEditor *>(Core::EditorManager::openEditorWithContents(Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &titlePattern,
            tr("# Untitled - By: %L1 - %L2\n"
               "\n"
               "import sensor\n"
               "\n"
               "sensor.reset()\n"
               "sensor.set_pixformat(sensor.RGB565)\n"
               "sensor.set_framesize(sensor.QVGA)\n"
               "sensor.skip_frames()\n"
               "\n"
               "while(True):\n"
               "    img = sensor.snapshot()\n").
            arg(Utils::Environment::systemEnvironment().userName()).arg(QDate::currentDate().toString()).toUtf8()));
        if(editor)
        {
            editor->editorWidget()->configureGenericHighlighter();
            Core::EditorManager::activateEditor(editor);
        }
        else
        {
            QMessageBox::critical(Core::ICore::dialogParent(),
                QObject::tr("New File"),
                QObject::tr("Can't open the new file!"));
        }
    });

    /* DGP 
    Core::ActionContainer *filesMenu = Core::ActionManager::actionContainer(Core::Constants::M_FILE);
    Core::ActionContainer *examplesMenu = Core::ActionManager::createMenu(Core::Id("NXTCam5.Examples"));
    filesMenu->addMenu(Core::ActionManager::actionContainer(Core::Constants::M_FILE_RECENTFILES), examplesMenu, Core::Constants::G_FILE_OPEN);
    examplesMenu->menu()->setTitle(tr("Examples"));
    examplesMenu->setOnAllDisabledBehavior(Core::ActionContainer::Show);
    connect(filesMenu->menu(), &QMenu::aboutToShow, this, [this, examplesMenu] {
        examplesMenu->menu()->clear();
        QMap<QString, QAction *> actions = aboutToShowExamplesRecursive(Core::ICore::userResourcePath() + QStringLiteral("/examples"), examplesMenu->menu());
        examplesMenu->menu()->addActions(actions.values());
        examplesMenu->menu()->setDisabled(actions.values().isEmpty());
    });
     DGP */

    ///////////////////////////////////////////////////////////////////////////

    Core::ActionContainer *toolsMenu = Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);
    Core::ActionContainer *helpMenu = Core::ActionManager::actionContainer(Core::Constants::M_HELP);

    QAction *bootloaderCommand = new QAction(tr("Run Bootloader"), this);
    m_bootloaderCommand = Core::ActionManager::registerAction(bootloaderCommand, Core::Id("NXTCam5.Bootloader"));
    //toolsMenu->addAction(m_bootloaderCommand);
    bootloaderCommand->setEnabled(true);
    connect(bootloaderCommand, &QAction::triggered, this, &OpenMVPlugin::bootloaderClicked);
    //toolsMenu->addSeparator();

    QAction *saveCommand = new QAction(tr("Save open script to NXTCam5 Cam"), this);
    m_saveCommand = Core::ActionManager::registerAction(saveCommand, Core::Id("NXTCam5.Save"));
    //toolsMenu->addAction(m_saveCommand);
    saveCommand->setEnabled(false);
    connect(saveCommand, &QAction::triggered, this, &OpenMVPlugin::saveScript);

    QAction *resetCommand = new QAction(tr("Restore Defaults"), this);
    /* DGP */
    m_resetCommand = Core::ActionManager::registerAction(resetCommand, Core::Id("NXTCamView5.RestoreDefaults"));
    toolsMenu->addAction(m_resetCommand);
    resetCommand->setEnabled(false);
    connect(resetCommand, &QAction::triggered, this, [this] {restoreDefaults();});
    /* DGP */

    //toolsMenu->addSeparator();
    m_openTerminalMenu = Core::ActionManager::createMenu(Core::Id("NXTCam5.OpenTermnial"));
    m_openTerminalMenu->setOnAllDisabledBehavior(Core::ActionContainer::Show);
    m_openTerminalMenu->menu()->setTitle(tr("Open Terminal"));
    //toolsMenu->addMenu(m_openTerminalMenu);
    connect(m_openTerminalMenu->menu(), &QMenu::aboutToShow, this, &OpenMVPlugin::openTerminalAboutToShow);

    m_featureMenu = Core::ActionManager::createMenu(Core::Id("NXTCamView.ChooseFeature"));
    m_featureMenu->setOnAllDisabledBehavior(Core::ActionContainer::Show);
    m_featureMenu->menu()->setTitle(tr("Choose Feature"));
    toolsMenu->addMenu(m_featureMenu);
    
    QAction *featureObjectCommand = new QAction(tr("Object Tracking"), this);
    m_featureObjectCommand =
    Core::ActionManager::registerAction(featureObjectCommand,
    Core::Id("NXTCamView.ObjectTracking"));
    m_featureMenu->addAction(m_featureObjectCommand);
    connect(featureObjectCommand, &QAction::triggered, this, [this] {chooseFeature(QStringLiteral("Object"));});

    QAction *featureLineCommand = new QAction(tr("Line Tracking"), this);
    m_featureLineCommand =
    Core::ActionManager::registerAction(featureLineCommand,
    Core::Id("NXTCamView.LineTracking"));
    m_featureMenu->addAction(m_featureLineCommand);
    connect(featureLineCommand, &QAction::triggered, this, [this] {chooseFeature(QStringLiteral("Line"));});

    QAction *featureFaceCommand = new QAction(tr("Face Tracking"), this);
    m_featureFaceCommand =
    Core::ActionManager::registerAction(featureFaceCommand,
    Core::Id("NXTCamView.FaceTracking"));
    m_featureMenu->addAction(m_featureFaceCommand);
    connect(featureFaceCommand, &QAction::triggered, this, [this] {chooseFeature(QStringLiteral("Face"));});

    QAction *featureEyeCommand = new QAction(tr("Eye Tracking"), this);
    m_featureEyeCommand =
    Core::ActionManager::registerAction(featureEyeCommand,
    Core::Id("NXTCamView.EyeTracking"));
    m_featureMenu->addAction(m_featureEyeCommand);
    connect(featureEyeCommand, &QAction::triggered, this, [this] {chooseFeature(QStringLiteral("Eye"));});

    QAction *featureQRCodeCommand = new QAction(tr("QRCode Tracking"), this);
    m_featureQRCodeCommand =
    Core::ActionManager::registerAction(featureQRCodeCommand,
    Core::Id("NXTCamView.QRCodeTracking"));
    //m_featureMenu->addAction(m_featureQRCodeCommand);
    //connect(featureQRCodeCommand, &QAction::triggered, this, [this] {chooseFeature(QStringLiteral("QRCode"));});

    QAction *featureMotionCommand = new QAction(tr("Motion Detection"), this);
    m_featureMotionCommand =
    Core::ActionManager::registerAction(featureMotionCommand,
    Core::Id("NXTCamView.MotionTracking"));
    //m_featureMenu->addAction(m_featureMotionCommand);
    //connect(featureMotionCommand, &QAction::triggered, this, [this] {chooseFeature(QStringLiteral("Motion"));});

    m_machineVisionToolsMenu = Core::ActionManager::createMenu(Core::Id("NXTCam5.MachineVision"));
    m_machineVisionToolsMenu->menu()->setTitle(tr("Machine Vision"));
    m_machineVisionToolsMenu->setOnAllDisabledBehavior(Core::ActionContainer::Show);
    //toolsMenu->addMenu(m_machineVisionToolsMenu);

    QAction *thresholdEditorCommand = new QAction(tr("Threshold Editor"), this);
    m_thresholdEditorCommand = Core::ActionManager::registerAction(thresholdEditorCommand, Core::Id("NXTCam5.ThresholdEditor"));
    m_machineVisionToolsMenu->addAction(m_thresholdEditorCommand);
    connect(thresholdEditorCommand, &QAction::triggered, this, &OpenMVPlugin::openThresholdEditor);

    QAction *keypointsEditorCommand = new QAction(tr("Keypoints Editor"), this);
    m_keypointsEditorCommand = Core::ActionManager::registerAction(keypointsEditorCommand, Core::Id("NXTCam5.KeypointsEditor"));
    m_machineVisionToolsMenu->addAction(m_keypointsEditorCommand);
    connect(keypointsEditorCommand, &QAction::triggered, this, &OpenMVPlugin::openKeypointsEditor);

    m_machineVisionToolsMenu->addSeparator();
    m_AprilTagGeneratorSubmenu = Core::ActionManager::createMenu(Core::Id("NXTCam5.AprilTagGenerator"));
    m_AprilTagGeneratorSubmenu->menu()->setTitle(tr("AprilTag Generator"));
    m_machineVisionToolsMenu->addMenu(m_AprilTagGeneratorSubmenu);

    QAction *tag16h5Command = new QAction(tr("TAG16H5 Family (30 Tags)"), this);
    m_tag16h5Command = Core::ActionManager::registerAction(tag16h5Command, Core::Id("NXTCam5.TAG16H5"));
    m_AprilTagGeneratorSubmenu->addAction(m_tag16h5Command);
    connect(tag16h5Command, &QAction::triggered, this, [this] {openAprilTagGenerator(tag16h5_create());});

    QAction *tag25h7Command = new QAction(tr("TAG25H7 Family (242 Tags)"), this);
    m_tag25h7Command = Core::ActionManager::registerAction(tag25h7Command, Core::Id("NXTCam5.TAG25H7"));
    m_AprilTagGeneratorSubmenu->addAction(m_tag25h7Command);
    connect(tag25h7Command, &QAction::triggered, this, [this] {openAprilTagGenerator(tag25h7_create());});

    QAction *tag25h9Command = new QAction(tr("TAG25H9 Family (35 Tags)"), this);
    m_tag25h9Command = Core::ActionManager::registerAction(tag25h9Command, Core::Id("NXTCam5.TAG25H9"));
    m_AprilTagGeneratorSubmenu->addAction(m_tag25h9Command);
    connect(tag25h9Command, &QAction::triggered, this, [this] {openAprilTagGenerator(tag25h9_create());});

    QAction *tag36h10Command = new QAction(tr("TAG36H10 Family (2320 Tags)"), this);
    m_tag36h10Command = Core::ActionManager::registerAction(tag36h10Command, Core::Id("NXTCam5.TAG36H10"));
    m_AprilTagGeneratorSubmenu->addAction(m_tag36h10Command);
    connect(tag36h10Command, &QAction::triggered, this, [this] {openAprilTagGenerator(tag36h10_create());});

    QAction *tag36h11Command = new QAction(tr("TAG36H11 Family (587 Tags - Recommended)"), this);
    m_tag36h11Command = Core::ActionManager::registerAction(tag36h11Command, Core::Id("NXTCam5.TAG36H11"));
    m_AprilTagGeneratorSubmenu->addAction(m_tag36h11Command);
    connect(tag36h11Command, &QAction::triggered, this, [this] {openAprilTagGenerator(tag36h11_create());});

    QAction *tag36artoolkitCommand = new QAction(tr("ARKTOOLKIT Family (512 Tags)"), this);
    m_tag36artoolkitCommand = Core::ActionManager::registerAction(tag36artoolkitCommand, Core::Id("NXTCam5.ARKTOOLKIT"));
    m_AprilTagGeneratorSubmenu->addAction(m_tag36artoolkitCommand);
    connect(tag36artoolkitCommand, &QAction::triggered, this, [this] {openAprilTagGenerator(tag36artoolkit_create());});

    QAction *QRCodeGeneratorCommand = new QAction(tr("QRCode Generator"), this);
    m_QRCodeGeneratorCommand = Core::ActionManager::registerAction(QRCodeGeneratorCommand, Core::Id("NXTCam5.QRCodeGenerator"));
    m_machineVisionToolsMenu->addAction(m_QRCodeGeneratorCommand);
    connect(QRCodeGeneratorCommand, &QAction::triggered, this, &OpenMVPlugin::openQRCodeGenerator);

    /*
    QAction *docsCommand = new QAction(tr("NXTCam5 Docs"), this);
    m_docsCommand = Core::ActionManager::registerAction(docsCommand, Core::Id("NXTCam5.Docs"));
    helpMenu->addAction(m_docsCommand, Core::Constants::G_HELP_SUPPORT);
    docsCommand->setEnabled(true);
    connect(docsCommand, &QAction::triggered, this, [this] {
        QUrl url = QUrl::fromLocalFile(Core::ICore::userResourcePath() + QStringLiteral("/html/index.html"));

        if(!QDesktopServices::openUrl(url))
        {
            QMessageBox::critical(Core::ICore::dialogParent(),
                                  QString(),
                                  tr("Failed to open: \"%L1\"").arg(url.toString()));
        }
    });

    QAction *forumsCommand = new QAction(tr("NXTCam5 Forums"), this);
    m_forumsCommand = Core::ActionManager::registerAction(forumsCommand, Core::Id("NXTCam5.Forums"));
    helpMenu->addAction(m_forumsCommand, Core::Constants::G_HELP_SUPPORT);
    forumsCommand->setEnabled(true);
    connect(forumsCommand, &QAction::triggered, this, [this] {
        QUrl url = QUrl(QStringLiteral("http://forums.openmv.io/"));

        if(!QDesktopServices::openUrl(url))
        {
            QMessageBox::critical(Core::ICore::dialogParent(),
                                  QString(),
                                  tr("Failed to open: \"%L1\"").arg(url.toString()));
        }
    });

    QAction *pinoutAction = new QAction(
         Utils::HostOsInfo::isMacHost() ? tr("About NXTCam5 Cam") : tr("About NXTCam5Cam..."), this);
    pinoutAction->setMenuRole(QAction::ApplicationSpecificRole);
    m_pinoutCommand = Core::ActionManager::registerAction(pinoutAction, Core::Id("NXTCam5.Pinout"));
    helpMenu->addAction(m_pinoutCommand, Core::Constants::G_HELP_ABOUT);
    pinoutAction->setEnabled(true);
    connect(pinoutAction, &QAction::triggered, this, [this] {
        QUrl url = QUrl::fromLocalFile(Core::ICore::userResourcePath() + QStringLiteral("/html/_images/pinout.png"));

        if(!QDesktopServices::openUrl(url))
        {
            QMessageBox::critical(Core::ICore::dialogParent(),
                                  QString(),
                                  tr("Failed to open: \"%L1\"").arg(url.toString()));
        }
    });
    */

    QAction *aboutAction = new QAction(QIcon::fromTheme(QStringLiteral("help-about")),
        Utils::HostOsInfo::isMacHost() ? tr("About NXTCamView5") : tr("About NXTCamView5 ..."), this);
    aboutAction->setMenuRole(QAction::AboutRole);
    m_aboutCommand = Core::ActionManager::registerAction(aboutAction, Core::Id("NXTCamView5.About"));
    helpMenu->addAction(m_aboutCommand, Core::Constants::G_HELP_ABOUT);
    aboutAction->setEnabled(true);
    connect(aboutAction, &QAction::triggered, this, [this] {
        QMessageBox::about(Core::ICore::dialogParent(), tr("About NXTCamView5"), tr(
        "<p><b>Config Tool for <a "
        "href=\"http://mindsensors.com/pages/317\">NXTCam5</a></b></p>"
        "<p>Version %L1</p>"
        "<p>By: <a href=\"http://mindsensors.com\">mindsensors.com</a></p>"
        "<p>Based on OpenMV IDE and QtCreator</p>"
        "<p><b>GNU GENERAL PUBLIC LICENSE</b></p>"
        "<p>Copyright (C) %L2 %L3</p>"
        "<p>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the <a href=\"https://raw.githubusercontent.com/qtproject/qt-creator/master/LICENSE.GPL3-EXCEPT\">GNU General Public License</a> for more details.</p>"
        "<p><b>Questions or Comments?</b></p>"
        "<p>Contact us at <a href=\"mailto:support@mindsensors.com\">support@mindsensors.com</a>.</p>"
        ).arg(QLatin1String(Core::Constants::NXTCAMVIEW_VERSION_LONG)).arg(QLatin1String(Core::Constants::NXTCAMVIEW_YEAR)).arg(QLatin1String(Core::Constants::NXTCAMVIEW_AUTHOR)));
    });

    ///////////////////////////////////////////////////////////////////////////

    m_connectCommand =
        Core::ActionManager::registerAction(new QAction(QIcon(QStringLiteral(CONNECT_PATH)),
        tr("Connect"), this), Core::Id("NXTCam5.Connect"));
    m_connectCommand->setDefaultKeySequence(tr("Ctrl+E"));
    m_connectCommand->action()->setEnabled(true);
    m_connectCommand->action()->setVisible(true);
    connect(m_connectCommand->action(), &QAction::triggered, this, [this] {connectClicked();});

    m_disconnectCommand =
        Core::ActionManager::registerAction(new QAction(QIcon(QStringLiteral(DISCONNECT_PATH)),
        tr("Disconnect"), this), Core::Id("NXTCam5.Disconnect"));
    m_disconnectCommand->setDefaultKeySequence(tr("Ctrl+E"));
    m_disconnectCommand->action()->setEnabled(false);
    m_disconnectCommand->action()->setVisible(false);
    connect(m_disconnectCommand->action(), &QAction::triggered, this, [this] {disconnectClicked();});

    m_startCommand =
        Core::ActionManager::registerAction(new QAction(QIcon(QStringLiteral(START_PATH)),
        tr("Start Tracking"), this), Core::Id("NXTCam5.Start"));
    m_startCommand->setDefaultKeySequence(tr("Ctrl+R"));
    m_startCommand->action()->setEnabled(false);
    m_startCommand->action()->setVisible(true);
    connect(m_startCommand->action(), &QAction::triggered, this, &OpenMVPlugin::startClicked);
    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged, [this] (Core::IEditor *editor) {
        if(m_connected)
        {
            m_saveCommand->action()->setEnabled((!m_portPath.isEmpty()) && (editor ? (editor->document() ? (!editor->document()->contents().isEmpty()) : false) : false));
            m_startCommand->action()->setEnabled((!m_running) && (editor ? (editor->document() ? (!editor->document()->contents().isEmpty()) : false) : false));
            m_startCommand->action()->setVisible(!m_running);
            m_stopCommand->action()->setEnabled(m_running);
            m_stopCommand->action()->setVisible(m_running);
        }
    });

    m_stopCommand =
        Core::ActionManager::registerAction(new QAction(QIcon(QStringLiteral(STOP_PATH)),
        tr("Stop Tracking"), this), Core::Id("NXTCam5.Stop"));
    m_stopCommand->setDefaultKeySequence(tr("Ctrl+R"));
    m_stopCommand->action()->setEnabled(false);
    m_stopCommand->action()->setVisible(false);
    connect(m_stopCommand->action(), &QAction::triggered, this, &OpenMVPlugin::stopClicked);
    connect(m_iodevice, &OpenMVPluginIO::scriptRunning, this, [this] (bool running) {
        if(m_connected)
        {
            Core::IEditor *editor = Core::EditorManager::currentEditor();
            m_saveCommand->action()->setEnabled((!m_portPath.isEmpty()) && (editor ? (editor->document() ? (!editor->document()->contents().isEmpty()) : false) : false));
            m_startCommand->action()->setEnabled((!running) && (editor ? (editor->document() ? (!editor->document()->contents().isEmpty()) : false) : false));
            m_startCommand->action()->setVisible(!running);
            m_stopCommand->action()->setEnabled(running);
            m_stopCommand->action()->setVisible(running);
            m_running = running;
        }
    });

    ///////////////////////////////////////////////////////////////////////////

    QMainWindow *mainWindow = q_check_ptr(qobject_cast<QMainWindow *>(Core::ICore::mainWindow()));
    Core::Internal::FancyTabWidget *widget = q_check_ptr(qobject_cast<Core::Internal::FancyTabWidget *>(mainWindow->centralWidget()));

    //Core::Internal::FancyActionBar *actionBar0 = new Core::Internal::FancyActionBar(widget);
    //widget->insertCornerWidget(0, actionBar0);

    //actionBar0->insertAction(0, Core::ActionManager::command(Core::Constants::NEW)->action());
    //actionBar0->insertAction(1, Core::ActionManager::command(Core::Constants::OPEN)->action());
    //actionBar0->insertAction(2, Core::ActionManager::command(Core::Constants::SAVE)->action());

    //actionBar0->setProperty("no_separator", true);
    //actionBar0->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    //Core::Internal::FancyActionBar *actionBar1 = new Core::Internal::FancyActionBar(widget);
    //widget->insertCornerWidget(1, actionBar1);

    //actionBar1->insertAction(0, Core::ActionManager::command(Core::Constants::UNDO)->action());
    //actionBar1->insertAction(1, Core::ActionManager::command(Core::Constants::REDO)->action());
    //actionBar1->insertAction(2, Core::ActionManager::command(Core::Constants::CUT)->action());
    //actionBar1->insertAction(3, Core::ActionManager::command(Core::Constants::COPY)->action());
    //actionBar1->insertAction(4, Core::ActionManager::command(Core::Constants::PASTE)->action());

    //actionBar1->setProperty("no_separator", false);
    //actionBar1->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    Core::Internal::FancyActionBar *actionBar2 = new Core::Internal::FancyActionBar(widget);
    widget->insertCornerWidget(2, actionBar2);

    actionBar2->insertAction(0, m_connectCommand->action());
    actionBar2->insertAction(1, m_disconnectCommand->action());
    actionBar2->insertAction(2, m_startCommand->action());
    actionBar2->insertAction(3, m_stopCommand->action());

    actionBar2->setProperty("no_separator", false);
    actionBar2->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    ///////////////////////////////////////////////////////////////////////////

    Utils::StyledBar *styledBar0 = new Utils::StyledBar;
    QHBoxLayout *styledBar0Layout = new QHBoxLayout;
    styledBar0Layout->setMargin(0);
    styledBar0Layout->setSpacing(0);
    styledBar0Layout->addSpacing(4);
    styledBar0Layout->addWidget(new QLabel(tr("Frame Buffer")));
    styledBar0Layout->addSpacing(6);
    styledBar0->setLayout(styledBar0Layout);

    m_zoom = new QToolButton;
    m_zoom->setText(tr("Zoom"));
    m_zoom->setToolTip(tr("Zoom to fit"));
    m_zoom->setCheckable(true);
    m_zoom->setChecked(false);
    //styledBar0Layout->addWidget(m_zoom);

    m_jpgCompress = new QToolButton;
    m_jpgCompress->setText(tr("JPG"));
    m_jpgCompress->setToolTip(tr("JPEG compress the Frame Buffer for higher performance"));
    m_jpgCompress->setCheckable(true);
    m_jpgCompress->setChecked(true);
    ///// Disable JPEG Compress /////
    m_jpgCompress->setVisible(false);
    styledBar0Layout->addWidget(m_jpgCompress);
    connect(m_jpgCompress, &QToolButton::clicked, this, [this] {
        if(m_connected)
        {
            if(!m_working)
            {
                m_iodevice->jpegEnable(m_jpgCompress->isChecked());
            }
            else
            {
                QMessageBox::critical(Core::ICore::dialogParent(),
                    tr("JPG"),
                    tr("Busy... please wait..."));
            }
        }
    });

    m_disableFrameBuffer = new QToolButton;
    m_disableFrameBuffer->setText(tr("Disable"));
    m_disableFrameBuffer->setToolTip(tr("Disable the Frame Buffer for maximum performance"));
    m_disableFrameBuffer->setCheckable(true);
    m_disableFrameBuffer->setChecked(false);
    styledBar0Layout->addWidget(m_disableFrameBuffer);
    connect(m_disableFrameBuffer, &QToolButton::clicked, this, [this] {
        if(m_connected)
        {
            if(!m_working)
            {
                m_iodevice->fbEnable(!m_disableFrameBuffer->isChecked());
            }
            else
            {
                QMessageBox::critical(Core::ICore::dialogParent(),
                    tr("Disable"),
                    tr("Busy... please wait..."));
            }
        }
    });

    m_frameBuffer = new OpenMVPluginFB;
    QWidget *tempWidget0 = new QWidget;
    QVBoxLayout *tempLayout0 = new QVBoxLayout;
    tempLayout0->setMargin(0);
    tempLayout0->setSpacing(0);
    //tempLayout0->addWidget(styledBar0); // top right button bar - DGP
    tempLayout0->addWidget(m_frameBuffer);
    tempWidget0->setLayout(tempLayout0);
    connect(m_zoom, &QToolButton::toggled, m_frameBuffer, &OpenMVPluginFB::enableFitInView);
    connect(m_iodevice, &OpenMVPluginIO::frameBufferData, m_frameBuffer, &OpenMVPluginFB::frameBufferData);
    connect(m_frameBuffer, &OpenMVPluginFB::saveImage, this, &OpenMVPlugin::saveImage);
    connect(m_frameBuffer, &OpenMVPluginFB::saveTemplate, this, &OpenMVPlugin::saveTemplate);
    connect(m_frameBuffer, &OpenMVPluginFB::saveDescriptor, this, &OpenMVPlugin::saveDescriptor);

    Utils::StyledBar *styledBar1 = new Utils::StyledBar;
    QHBoxLayout *styledBar1Layout = new QHBoxLayout;
    styledBar1Layout->setMargin(0);
    styledBar1Layout->setSpacing(0);
    styledBar1Layout->addSpacing(4);
    styledBar1Layout->addWidget(new QLabel(tr("Histogram")));
    styledBar1Layout->addSpacing(6);
    styledBar1->setLayout(styledBar1Layout);

    m_histogramColorSpace = new QComboBox;
    m_histogramColorSpace->setProperty("hideborder", true);
    m_histogramColorSpace->setProperty("drawleftborder", false);
    m_histogramColorSpace->insertItem(RGB_COLOR_SPACE, tr("RGB Color Space"));
    m_histogramColorSpace->insertItem(GRAYSCALE_COLOR_SPACE, tr("Grayscale Color Space"));
    m_histogramColorSpace->insertItem(LAB_COLOR_SPACE, tr("LAB Color Space"));
    m_histogramColorSpace->insertItem(YUV_COLOR_SPACE, tr("YUV Color Space"));
    m_histogramColorSpace->setCurrentIndex(RGB_COLOR_SPACE);
    m_histogramColorSpace->setToolTip(tr("Use Grayscale/LAB for color tracking"));
    styledBar1Layout->addWidget(m_histogramColorSpace);

    m_histogram = new OpenMVPluginHistogram;

    QWidget *tempWidget1 = new QWidget;
    QVBoxLayout *tempLayout1 = new QVBoxLayout;
    tempLayout1->setMargin(0);
    tempLayout1->setSpacing(0);
    tempLayout1->addWidget(styledBar1);
    tempLayout1->addWidget(m_histogram);
    tempWidget1->setLayout(tempLayout1);
    connect(m_histogramColorSpace, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), m_histogram, &OpenMVPluginHistogram::colorSpaceChanged);
    connect(m_frameBuffer, &OpenMVPluginFB::pixmapUpdate, m_histogram, &OpenMVPluginHistogram::pixmapUpdate);
    connect(m_frameBuffer, &OpenMVPluginFB::captureColorMap, m_histogram, &OpenMVPluginHistogram::captureColorMap);
    connect(m_frameBuffer, &OpenMVPluginFB::clearColorMap, m_histogram, &OpenMVPluginHistogram::clearColorMap);
    connect(this, &OpenMVPlugin::loadColorMap, m_histogram, &OpenMVPluginHistogram::loadColorMap);

    m_hsplitter = widget->m_hsplitter;
    m_vsplitter = widget->m_vsplitter;
    m_vsplitter->insertWidget(0, tempWidget0);
    //m_vsplitter->insertWidget(1, tempWidget1);
    m_vsplitter->setStretchFactor(0, 0);
    m_vsplitter->setStretchFactor(1, 1);
    m_vsplitter->setCollapsible(0, true);
    m_vsplitter->setCollapsible(1, true);

    connect(m_iodevice, &OpenMVPluginIO::printData, Core::MessageManager::instance(), &Core::MessageManager::printData);
    connect(m_iodevice, &OpenMVPluginIO::printData, this, &OpenMVPlugin::errorFilter);
    connect(m_iodevice, &OpenMVPluginIO::frameBufferData, this, [this] {
        m_queue.push_back(m_timer.restart());

        if(m_queue.size() > FPS_AVERAGE_BUFFER_DEPTH)
        {
            m_queue.pop_front();
        }

        qint64 average = 0;

        for(int i = 0; i < m_queue.size(); i++)
        {
            average += m_queue.at(i);
        }

        average /= m_queue.size();

        m_fpsLabel->setText(tr("FPS: %L1").arg(average ? (1000 / double(average)) : 0, 5, 'f', 1));
    });

    ///////////////////////////////////////////////////////////////////////////

    m_statusLabel = new Utils::ElidingLabel(tr(""));
    m_statusLabel->setToolTip(tr("Status Message"));
    m_statusLabel->setDisabled(true);
    Core::ICore::statusBar()->addPermanentWidget(m_statusLabel);
    Core::ICore::statusBar()->addPermanentWidget(new QLabel());

    m_featureLabel = new Utils::ElidingLabel(tr("Feature"));
    m_featureLabel->setToolTip(tr("Feature"));
    m_featureLabel->setEnabled(true);
    Core::ICore::statusBar()->addPermanentWidget(m_featureLabel);
    Core::ICore::statusBar()->addPermanentWidget(new QLabel());

    m_versionButton = new Utils::ElidingToolButton;
    m_versionButton->setText(tr("Firmware Version:"));
    m_versionButton->setToolTip(tr("NXTCam-v5 firmware version"));
    m_versionButton->setCheckable(false);
    m_versionButton->setDisabled(true);
    Core::ICore::statusBar()->addPermanentWidget(m_versionButton);
    Core::ICore::statusBar()->addPermanentWidget(new QLabel());
    connect(m_versionButton, &QToolButton::clicked, this, &OpenMVPlugin::updateCam);

    m_portLabel = new Utils::ElidingLabel(tr("Serial Port:"));
    m_portLabel->setToolTip(tr("NXTCam-v5 serial port"));
    m_portLabel->setDisabled(true);
    Core::ICore::statusBar()->addPermanentWidget(m_portLabel);
    Core::ICore::statusBar()->addPermanentWidget(new QLabel());

    m_pathButton = new Utils::ElidingToolButton;
    m_pathButton->setText(tr("Drive:"));
    m_pathButton->setToolTip(tr("Drive associated with port"));
    m_pathButton->setCheckable(false);
    m_pathButton->setDisabled(true);
    Core::ICore::statusBar()->addPermanentWidget(m_pathButton);
    Core::ICore::statusBar()->addPermanentWidget(new QLabel());
    connect(m_pathButton, &QToolButton::clicked, this, &OpenMVPlugin::setPortPath);

    m_fpsLabel = new Utils::ElidingLabel(tr("FPS:"));
    m_fpsLabel->setToolTip(tr("May be different from camera FPS"));
    m_fpsLabel->setDisabled(true);
    m_fpsLabel->setMinimumWidth(m_fpsLabel->fontMetrics().width(QStringLiteral("FPS: 000.000")));
    Core::ICore::statusBar()->addPermanentWidget(m_fpsLabel);

    ///////////////////////////////////////////////////////////////////////////

    QSettings *settings = ExtensionSystem::PluginManager::settings();
    settings->beginGroup(QStringLiteral(SETTINGS_GROUP));
    Core::EditorManager::restoreState(
        settings->value(QStringLiteral(EDITOR_MANAGER_STATE)).toByteArray());

    {
        // set the vertical splitter to closed position at the
        // start of program.
        char a[] = {'\0','\0','\0','\xff',
            '\0','\0','\0','\x1',
            '\0','\0','\0','\x2',
            '\0','\0','\0','\0',
            '\0','\0','\x3','\xbf',
            '\0','\0','\0','\0',
            '\x1','\x1','\0','\0',
            '\0','\x1','\0'};
        QByteArray qba = QByteArray::fromRawData(a, sizeof(a));

    #if (VIEW_DEV == 1)
        // development stage (open splitter based on ini value)
        QString def_state = QString::fromUtf8((char *)qba.data());
        m_hsplitter->restoreState( settings->value(QStringLiteral(HSPLITTER_STATE), def_state).toByteArray());
    #else
        // release stage (splitter will always be closed)
        m_hsplitter->restoreState( qba );
    #endif
    }


    m_vsplitter->restoreState(
        settings->value(QStringLiteral(VSPLITTER_STATE)).toByteArray());
    m_zoom->setChecked( true
        /*settings->value(QStringLiteral(ZOOM_STATE), * m_zoom->isChecked()).toBool()*/);
    m_jpgCompress->setChecked(
        settings->value(QStringLiteral(JPG_COMPRESS_STATE), m_jpgCompress->isChecked()).toBool());
    m_disableFrameBuffer->setChecked( false
        /*settings->value(QStringLiteral(DISABLE_FRAME_BUFFER_STATE), * m_disableFrameBuffer->isChecked()).toBool()*/);
    m_histogramColorSpace->setCurrentIndex(
        settings->value(QStringLiteral(HISTOGRAM_COLOR_SPACE_STATE), m_histogramColorSpace->currentIndex()).toInt());
    QFont font = TextEditor::TextEditorSettings::fontSettings().defaultFixedFontFamily();
    font.setPointSize(TextEditor::TextEditorSettings::fontSettings().defaultFontSize());
    Core::MessageManager::outputWindow()->setBaseFont(font);
    Core::MessageManager::outputWindow()->setWheelZoomEnabled(true);
    Core::MessageManager::outputWindow()->setFontZoom(
        settings->value(QStringLiteral(OUTPUT_WINDOW_FONT_ZOOM_STATE)).toFloat());
    settings->endGroup();

    m_openTerminalMenuData = QList<openTerminalMenuData_t>();

    for(int i = 0, j = settings->beginReadArray(QStringLiteral(OPEN_TERMINAL_SETTINGS_GROUP)); i < j; i++)
    {
        settings->setArrayIndex(i);
        openTerminalMenuData_t data;
        data.displayName = settings->value(QStringLiteral(OPEN_TERMINAL_DISPLAY_NAME)).toString();
        data.optionIndex = settings->value(QStringLiteral(OPEN_TERMINAL_OPTION_INDEX)).toInt();
        data.commandStr = settings->value(QStringLiteral(OPEN_TERMINAL_COMMAND_STR)).toString();
        data.commandVal = settings->value(QStringLiteral(OPEN_TERMINAL_COMMAND_VAL)).toInt();
        m_openTerminalMenuData.append(data);
    }

    settings->endArray();

    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested, this, [this] {
        QSettings *settings = ExtensionSystem::PluginManager::settings();
        settings->beginGroup(QStringLiteral(SETTINGS_GROUP));

        settings->setValue(QStringLiteral(EDITOR_MANAGER_STATE),
            Core::EditorManager::saveState());
        settings->setValue(QStringLiteral(HSPLITTER_STATE),
            m_hsplitter->saveState());
        settings->setValue(QStringLiteral(VSPLITTER_STATE),
            m_vsplitter->saveState());
        settings->setValue(QStringLiteral(ZOOM_STATE),
            /*m_zoom->isChecked()*/ true);
        settings->setValue(QStringLiteral(JPG_COMPRESS_STATE),
            m_jpgCompress->isChecked());
        settings->setValue(QStringLiteral(DISABLE_FRAME_BUFFER_STATE),
            /*m_disableFrameBuffer->isChecked()*/ false);
        settings->setValue(QStringLiteral(HISTOGRAM_COLOR_SPACE_STATE),
            m_histogramColorSpace->currentIndex());
        settings->setValue(QStringLiteral(OUTPUT_WINDOW_FONT_ZOOM_STATE),
            Core::MessageManager::outputWindow()->fontZoom());
        settings->endGroup();

        settings->beginWriteArray(QStringLiteral(OPEN_TERMINAL_SETTINGS_GROUP));

        for(int i = 0, j = m_openTerminalMenuData.size(); i < j; i++)
        {
            settings->setArrayIndex(i);
            settings->setValue(QStringLiteral(OPEN_TERMINAL_DISPLAY_NAME), m_openTerminalMenuData.at(i).displayName);
            settings->setValue(QStringLiteral(OPEN_TERMINAL_OPTION_INDEX), m_openTerminalMenuData.at(i).optionIndex);
            settings->setValue(QStringLiteral(OPEN_TERMINAL_COMMAND_STR), m_openTerminalMenuData.at(i).commandStr);
            settings->setValue(QStringLiteral(OPEN_TERMINAL_COMMAND_VAL), m_openTerminalMenuData.at(i).commandVal);
        }

        settings->endArray();
    });

    ///////////////////////////////////////////////////////////////////////////

    settings->beginGroup(QStringLiteral(SETTINGS_GROUP));

    /* DGP */
    /*
    int major = settings->value(QStringLiteral(RESOURCES_MAJOR), 0).toInt();
    int minor = settings->value(QStringLiteral(RESOURCES_MINOR), 0).toInt();
    int patch = settings->value(QStringLiteral(RESOURCES_PATCH), 0).toInt();

    if((major < OMV_IDE_VERSION_MAJOR)
    || ((major == OMV_IDE_VERSION_MAJOR) && (minor < OMV_IDE_VERSION_MINOR))
    || ((major == OMV_IDE_VERSION_MAJOR) && (minor ==
    OMV_IDE_VERSION_MINOR) && (patch < OMV_IDE_VERSION_RELEASE)))
    {
        settings->setValue(QStringLiteral(RESOURCES_MAJOR), 0);
        settings->setValue(QStringLiteral(RESOURCES_MINOR), 0);
        settings->setValue(QStringLiteral(RESOURCES_PATCH), 0);
        settings->sync();

        bool ok = true;

        QString error;

        if(!Utils::FileUtils::removeRecursively(Utils::FileName::fromString(Core::ICore::userResourcePath()), &error))
        {
            QMessageBox::critical(Core::ICore::dialogParent(),
                QString(),
                error + tr("\n\nPlease close any programs that are viewing/editing NXTCamView5's application data and then restart NXTCamView5!"));

            QApplication::quit();
            ok = false;
        }
        else
        {
            QStringList list = QStringList() << QStringLiteral("examples") << QStringLiteral("firmware") << QStringLiteral("html");

            foreach(const QString &dir, list)
            {
                QString error;

                if(!Utils::FileUtils::copyRecursively(Utils::FileName::fromString(Core::ICore::resourcePath() + QLatin1Char('/') + dir),
                                                      Utils::FileName::fromString(Core::ICore::userResourcePath() + QLatin1Char('/') + dir),
                                                      &error))
                {
                    QMessageBox::critical(Core::ICore::dialogParent(),
                        QString(),
                        error + tr("\n\nPlease close any programs that are viewing/editing NXTCamView5's application data and then restart NXTCamView5!"));

                    QApplication::quit();
                    ok = false;
                    break;
                }
            }
        }

        if(ok)
        {
            settings->setValue(QStringLiteral(RESOURCES_MAJOR), OMV_IDE_VERSION_MAJOR);
            settings->setValue(QStringLiteral(RESOURCES_MINOR), OMV_IDE_VERSION_MINOR);
            settings->setValue(QStringLiteral(RESOURCES_PATCH), OMV_IDE_VERSION_RELEASE);
            settings->sync();
        }
    }
    */

    settings->endGroup();

    logLine(QStringLiteral("before connect bindings"));
    connect(m_histogram, &OpenMVPluginHistogram::updateColorsOnMenu,
                            m_frameBuffer, &OpenMVPluginFB::updateColorsOnMenu);
    connect(m_histogram, &OpenMVPluginHistogram::statusUpdate,
                            this, &OpenMVPlugin::statusUpdate);
    //connect(m_histogram, &OpenMVPluginHistogram::restartIfNeeded,
                            //this, &OpenMVPlugin::restartIfNeeded);
    connect(m_histogram, &OpenMVPluginHistogram::script_checkIfRunning,
                            this, &OpenMVPlugin::script_checkIfRunning);
    connect(m_histogram, &OpenMVPluginHistogram::stopClicked,
                            this, &OpenMVPlugin::stopClicked);
    connect(m_histogram, &OpenMVPluginHistogram::startClicked,
                            this, &OpenMVPlugin::startClicked);
    logLine(QStringLiteral("after  connect bindings"));

    ///////////////////////////////////////////////////////////////////////////

    QStringList providerVariables;
    QStringList providerFunctions;
    QMap<QString, QStringList> providerFunctionArgs;

    QRegularExpression moduleRegEx(QStringLiteral("<div class=\"section\" id=\"module-(.+?)\">(.*?)<div class=\"section\""), QRegularExpression::DotMatchesEverythingOption);
    QRegularExpression spanRegEx(QStringLiteral("<span.*?>"), QRegularExpression::DotMatchesEverythingOption);
    QRegularExpression linkRegEx(QStringLiteral("<a.*?>"), QRegularExpression::DotMatchesEverythingOption);
    QRegularExpression classRegEx(QStringLiteral(" class=\".*?\""), QRegularExpression::DotMatchesEverythingOption);
    QRegularExpression cdfmRegEx(QStringLiteral("<dl class=\"(class|data|function|method)\">\\s*<dt id=\"(.+?)\">(.*?)</dt>\\s*<dd>(.*?)</dd>\\s*</dl>"), QRegularExpression::DotMatchesEverythingOption);
    QRegularExpression argumentRegEx(QStringLiteral("<span class=\"sig-paren\">\\(</span>(.*?)<span class=\"sig-paren\">\\)</span>"), QRegularExpression::DotMatchesEverythingOption);
    QRegularExpression tupleRegEx(QStringLiteral("\\(.*?\\)"), QRegularExpression::DotMatchesEverythingOption);
    QRegularExpression listRegEx(QStringLiteral("\\[.*?\\]"), QRegularExpression::DotMatchesEverythingOption);
    QRegularExpression dictionaryRegEx(QStringLiteral("\\{.*?\\}"), QRegularExpression::DotMatchesEverythingOption);

    QDirIterator it(Core::ICore::userResourcePath() + QStringLiteral("/html/library"), QDir::Files);

    while(it.hasNext())
    {
        QFile file(it.next());

        if(file.open(QIODevice::ReadOnly))
        {
            QString data = QString::fromUtf8(file.readAll());

            if((file.error() == QFile::NoError) && (!data.isEmpty()))
            {
                file.close();

                QRegularExpressionMatch moduleMatch = moduleRegEx.match(data);

                if(moduleMatch.hasMatch())
                {
                    QString name = moduleMatch.captured(1);
                    QString text = moduleMatch.captured(2).
                                   remove(QStringLiteral("\u00B6")).
                                   remove(spanRegEx).
                                   remove(QStringLiteral("</span>")).
                                   remove(linkRegEx).
                                   remove(QStringLiteral("</a>")).
                                   remove(classRegEx).
                                   replace(QStringLiteral("<h1>"), QStringLiteral("<h3>")).
                                   replace(QStringLiteral("</h1>"), QStringLiteral("</h3>"));

                    documentation_t d;
                    d.moduleName = QString();
                    d.className = QString();
                    d.name = name;
                    d.text = text;
                    m_modules.append(d);

                    if(name.startsWith(QLatin1Char('u')))
                    {
                        d.name = name.mid(1);
                        m_modules.append(d);
                    }
                }

                QRegularExpressionMatchIterator matches = cdfmRegEx.globalMatch(data);

                while(matches.hasNext())
                {
                    QRegularExpressionMatch match = matches.next();
                    QString type = match.captured(1);
                    QString id = match.captured(2);
                    QString head = match.captured(3);
                    QString body = match.captured(4);
                    QStringList idList = id.split(QLatin1Char('.'), QString::SkipEmptyParts);

                    if((1 <= idList.size()) && (idList.size() <= 3))
                    {
                        documentation_t d;
                        d.moduleName = (idList.size() > 1) ? idList.at(0) : QString();
                        d.className = (idList.size() > 2) ? idList.at(1) : QString();
                        d.name = idList.last();
                        d.text = QString(QStringLiteral("<h3>%1</h3>%2")).arg(it.fileInfo().completeBaseName() + QStringLiteral(" - ") + head).arg(body).
                                 remove(QStringLiteral("\u00B6")).
                                 remove(spanRegEx).
                                 remove(QStringLiteral("</span>")).
                                 remove(linkRegEx).
                                 remove(QStringLiteral("</a>")).
                                 remove(classRegEx);

                        if(type == QStringLiteral("class"))
                        {
                            m_classes.append(d);
                            providerFunctions.append(d.name);
                        }
                        else if(type == QStringLiteral("data"))
                        {
                            m_datas.append(d);
                            providerVariables.append(d.name);
                        }
                        else if(type == QStringLiteral("function"))
                        {
                            m_functions.append(d);
                            providerFunctions.append(d.name);
                        }
                        else if(type == QStringLiteral("method"))
                        {
                            m_methods.append(d);
                            providerFunctions.append(d.name);
                        }

                        QRegularExpressionMatch args = argumentRegEx.match(head);

                        if(args.hasMatch())
                        {
                            QStringList list;

                            foreach(const QString &arg, args.captured(1).
                                                        remove(QLatin1String("<span class=\"optional\">[</span>")).
                                                        remove(QLatin1String("<span class=\"optional\">]</span>")).
                                                        remove(QLatin1String("<em>")).
                                                        remove(QLatin1String("</em>")).
                                                        remove(tupleRegEx).
                                                        remove(listRegEx).
                                                        remove(dictionaryRegEx).
                                                        remove(QLatin1Char(' ')).
                                                        split(QLatin1Char(','), QString::SkipEmptyParts))
                            {
                                int equals = arg.indexOf(QLatin1Char('='));
                                QString temp = (equals != -1) ? arg.left(equals) : arg;

                                m_arguments.insert(temp);
                                list.append(temp);
                            }

                            providerFunctionArgs.insert(d.name, list);
                        }
                    }
                }
            }
        }
    }

    connect(TextEditor::Internal::Manager::instance(), &TextEditor::Internal::Manager::highlightingFilesRegistered, this, [this] {
        QString id = TextEditor::Internal::Manager::instance()->definitionIdByName(QStringLiteral("Python"));

        if(!id.isEmpty())
        {
            QSharedPointer<TextEditor::Internal::HighlightDefinition> def = TextEditor::Internal::Manager::instance()->definition(id);

            if(def)
            {
                QSharedPointer<TextEditor::Internal::KeywordList> modulesList = def->keywordList(QStringLiteral("listOpenMVModules"));
                QSharedPointer<TextEditor::Internal::KeywordList> classesList = def->keywordList(QStringLiteral("listOpenMVClasses"));
                QSharedPointer<TextEditor::Internal::KeywordList> datasList = def->keywordList(QStringLiteral("listOpenMVDatas"));
                QSharedPointer<TextEditor::Internal::KeywordList> functionsList = def->keywordList(QStringLiteral("listOpenMVFunctions"));
                QSharedPointer<TextEditor::Internal::KeywordList> methodsList = def->keywordList(QStringLiteral("listOpenMVMethods"));
                QSharedPointer<TextEditor::Internal::KeywordList> argumentsList = def->keywordList(QStringLiteral("listOpenMVArguments"));

                if(modulesList)
                {
                    foreach(const documentation_t &d, m_modules)
                    {
                        modulesList->addKeyword(d.name);
                    }
                }

                if(classesList)
                {
                    foreach(const documentation_t &d, m_classes)
                    {
                        classesList->addKeyword(d.name);
                    }
                }

                if(datasList)
                {
                    foreach(const documentation_t &d, m_datas)
                    {
                        datasList->addKeyword(d.name);
                    }
                }

                if(functionsList)
                {
                    foreach(const documentation_t &d, m_functions)
                    {
                        functionsList->addKeyword(d.name);
                    }
                }

                if(methodsList)
                {
                    foreach(const documentation_t &d, m_methods)
                    {
                        methodsList->addKeyword(d.name);
                    }
                }

                if(argumentsList)
                {
                    foreach(const QString &d, m_arguments.values())
                    {
                        argumentsList->addKeyword(d);
                    }
                }
            }
        }
    });

    OpenMVPluginCompletionAssistProvider *provider = new OpenMVPluginCompletionAssistProvider(providerVariables, providerFunctions, providerFunctionArgs);
    provider->setParent(this);

    connect(Core::EditorManager::instance(), &Core::EditorManager::editorCreated, this, [this, provider] (Core::IEditor *editor, const QString &fileName) {
        TextEditor::BaseTextEditor *textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);

        if(textEditor && fileName.endsWith(QStringLiteral(".py"), Qt::CaseInsensitive))
        {
            textEditor->textDocument()->setCompletionAssistProvider(provider);
            connect(textEditor->editorWidget(), &TextEditor::TextEditorWidget::tooltipOverrideRequested, this, [this] (TextEditor::TextEditorWidget *widget, const QPoint &globalPos, int position, bool *handled) {

                if(handled)
                {
                    *handled = true;
                }

                QTextCursor cursor(widget->textDocument()->document());
                cursor.setPosition(position);
                cursor.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
                QString text = cursor.selectedText().replace(QChar::ParagraphSeparator, QLatin1Char('\n'));

                enum
                {
                    IN_NONE,
                    IN_COMMENT,
                    IN_STRING_0,
                    IN_STRING_1
                }
                in_state = IN_NONE;

                for(int i = 0; i < text.size(); i++)
                {
                    switch(in_state)
                    {
                        case IN_NONE:
                        {
                            if((text.at(i) == QLatin1Char('#')) && ((!i) || (text.at(i-1) != QLatin1Char('\\')))) in_state = IN_COMMENT;
                            if((text.at(i) == QLatin1Char('\'')) && ((!i) || (text.at(i-1) != QLatin1Char('\\')))) in_state = IN_STRING_0;
                            if((text.at(i) == QLatin1Char('\"')) && ((!i) || (text.at(i-1) != QLatin1Char('\\')))) in_state = IN_STRING_1;
                            break;
                        }
                        case IN_COMMENT:
                        {
                            if((text.at(i) == QLatin1Char('\n')) && (text.at(i-1) != QLatin1Char('\\'))) in_state = IN_NONE;
                            break;
                        }
                        case IN_STRING_0:
                        {
                            if((text.at(i) == QLatin1Char('\'')) && (text.at(i-1) != QLatin1Char('\\'))) in_state = IN_NONE;
                            break;
                        }
                        case IN_STRING_1:
                        {
                            if((text.at(i) == QLatin1Char('\"')) && (text.at(i-1) != QLatin1Char('\\'))) in_state = IN_NONE;
                            break;
                        }
                    }
                }

                if(in_state == IN_NONE)
                {
                    cursor.setPosition(position);
                    cursor.select(QTextCursor::WordUnderCursor);
                    text = cursor.selectedText();

                    QStringList list;

                    foreach(const documentation_t &d, m_modules)
                    {
                        if(d.name == text)
                        {
                            list.append(d.text);
                        }
                    }

                    if(widget->textDocument()->document()->characterAt(qMin(cursor.position(), cursor.anchor()) - 1) == QLatin1Char('.'))
                    {
                        foreach(const documentation_t &d, m_datas)
                        {
                            if(d.name == text)
                            {
                                list.append(d.text);
                            }
                        }

                        if(widget->textDocument()->document()->characterAt(qMax(cursor.position(), cursor.anchor())) == QLatin1Char('('))
                        {
                            foreach(const documentation_t &d, m_classes)
                            {
                                if(d.name == text)
                                {
                                    list.append(d.text);
                                }
                            }

                            foreach(const documentation_t &d, m_functions)
                            {
                                if(d.name == text)
                                {
                                    list.append(d.text);
                                }
                            }

                            foreach(const documentation_t &d, m_methods)
                            {
                                if(d.name == text)
                                {
                                    list.append(d.text);
                                }
                            }
                        }
                    }

                    if(!list.isEmpty())
                    {
                        QString string;
                        int i = 0;

                        for(int j = 0, k = qCeil(qSqrt(list.size())); j < k; j++)
                        {
                            string.append(QStringLiteral("<tr>"));

                            for(int l = 0, m = k; l < m; l++)
                            {
                                string.append(QStringLiteral("<td style=\"padding:6px;\">") + list.at(i++) + QStringLiteral("</td>"));

                                if(i >= list.size())
                                {
                                    break;
                                }
                            }

                            string.append(QStringLiteral("</tr>"));

                            if(i >= list.size())
                            {
                                break;
                            }
                        }

                        Utils::ToolTip::show(globalPos, QStringLiteral("<table>") + string + QStringLiteral("</table>"), widget);
                        return;
                    }
                }

                Utils::ToolTip::hide();
            });
        }
    });

    ///////////////////////////////////////////////////////////////////////////

    // TODO: change the default program to something NXTCam specific

    Core::IEditor *editor = Core::EditorManager::currentEditor();
    if(editor ? (editor->document() ? editor->document()->contents().isEmpty() : true) : true)
    {
        QString filePath = Core::ICore::resourcePath() + QStringLiteral("/examples/01-Basics/NXTCam5_default.py");

        QFile file(filePath);

        if(file.open(QIODevice::ReadOnly))
        {
            QByteArray data = file.readAll();

            if((file.error() == QFile::NoError) && (!data.isEmpty()))
            {
                Core::EditorManager::cutForwardNavigationHistory();
                Core::EditorManager::addCurrentPositionToNavigationHistory();

                QString titlePattern = QFileInfo(filePath).baseName().simplified() + QStringLiteral("_$.") + QFileInfo(filePath).completeSuffix();
                TextEditor::BaseTextEditor *editor = qobject_cast<TextEditor::BaseTextEditor *>(Core::EditorManager::openEditorWithContents(Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &titlePattern, data));

                if(editor)
                {
                    editor->editorWidget()->configureGenericHighlighter();
                    Core::EditorManager::activateEditor(editor);
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////

    QLoggingCategory::setFilterRules(QStringLiteral("qt.network.ssl.warning=false")); // http://stackoverflow.com/questions/26361145/qsslsocket-error-when-ssl-is-not-used

    connect(Core::ICore::instance(), &Core::ICore::coreOpened, this, [this] {

        QNetworkAccessManager *manager = new QNetworkAccessManager(this);

        connect(manager, &QNetworkAccessManager::finished, this, [this] (QNetworkReply *reply) {

            QByteArray data = reply->readAll();

            if((reply->error() == QNetworkReply::NoError) && (!data.isEmpty()))
            {
                /*
                QRegularExpressionMatch match = QRegularExpression(QStringLiteral("(\\d+)\\.(\\d+)\\.(\\d+)")).match(QString::fromUtf8(data));

                int major = match.captured(1).toInt();
                int minor = match.captured(2).toInt();
                int patch = match.captured(3).toInt();

                if((NXTCAMVIEW_VERSION_MAJOR < major)
                || ((NXTCAMVIEW_VERSION_MAJOR == major) && (NXTCAMVIEW_VERSION_MINOR < minor))
                || ((NXTCAMVIEW_VERSION_MAJOR == major) &&
                (NXTCAMVIEW_VERSION_MINOR == minor) && (NXTCAMVIEW_VERSION_RELEASE < patch)))
                {
                    QMessageBox box(QMessageBox::Information, tr("Update Available"), tr("A new version of OpenMV Ide (%L1.%L2.%L3) is available for download.").arg(major).arg(minor).arg(patch), QMessageBox::Cancel, Core::ICore::dialogParent(),
                        Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                        (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));
                    QPushButton *button = box.addButton(tr("Download"), QMessageBox::AcceptRole);
                    box.setDefaultButton(button);
                    box.setEscapeButton(QMessageBox::Cancel);
                    box.exec();

                    if(box.clickedButton() == button)
                    {
                        QUrl url = QUrl(QStringLiteral("http://openmv.io/pages/download"));

                        if(!QDesktopServices::openUrl(url))
                        {
                            QMessageBox::critical(Core::ICore::dialogParent(),
                                                  QString(),
                                                  tr("Failed to open: \"%L1\"").arg(url.toString()));
                        }
                    }
                    else
                    {
                        QTimer::singleShot(0, this, &OpenMVPlugin::packageUpdate);
                    }
                }
                else
                {
                    QTimer::singleShot(0, this, &OpenMVPlugin::packageUpdate);
                }
                */
            }
            else
            {
                QTimer::singleShot(0, this, &OpenMVPlugin::packageUpdate);
            }

            reply->deleteLater();
        });

        QNetworkRequest request = QNetworkRequest(QUrl(QStringLiteral("http://upload.openmv.io/openmv-ide-version.txt")));
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
        request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#endif
        QNetworkReply *reply = manager->get(request);

        if(reply)
        {
            connect(reply, &QNetworkReply::sslErrors, reply, static_cast<void (QNetworkReply::*)(void)>(&QNetworkReply::ignoreSslErrors));
        }
        else
        {
            QTimer::singleShot(0, this, &OpenMVPlugin::packageUpdate);
        }
    });
}

ExtensionSystem::IPlugin::ShutdownFlag OpenMVPlugin::aboutToShutdown()
{
    if(!m_connected)
    {
        if(!m_working)
        {
            return ExtensionSystem::IPlugin::SynchronousShutdown;
        }
        else
        {
            connect(this, &OpenMVPlugin::workingDone, this, [this] {disconnectClicked();});
            connect(this, &OpenMVPlugin::disconnectDone, this, &OpenMVPlugin::asynchronousShutdownFinished);
            return ExtensionSystem::IPlugin::AsynchronousShutdown;
        }
    }
    else
    {
        if(!m_working)
        {
            connect(this, &OpenMVPlugin::disconnectDone, this, &OpenMVPlugin::asynchronousShutdownFinished);
            QTimer::singleShot(0, this, [this] {disconnectClicked();});
            return ExtensionSystem::IPlugin::AsynchronousShutdown;
        }
        else
        {
            connect(this, &OpenMVPlugin::workingDone, this, [this] {disconnectClicked();});
            connect(this, &OpenMVPlugin::disconnectDone, this, &OpenMVPlugin::asynchronousShutdownFinished);
            return ExtensionSystem::IPlugin::AsynchronousShutdown;
        }
    }
}

#if 0
static bool removeRecursively(const Utils::FileName &path, QString *error)
{
    return Utils::FileUtils::removeRecursively(path, error);
}

static bool removeRecursivelyWrapper(const Utils::FileName &path, QString *error)
{
    QEventLoop loop;
    QFutureWatcher<bool> watcher;
    QObject::connect(&watcher, &QFutureWatcher<bool>::finished, &loop, &QEventLoop::quit);
    watcher.setFuture(QtConcurrent::run(removeRecursively, path, error));
    loop.exec();
    return watcher.result();
}

static bool extractAll(QByteArray *data, const QString &path)
{
    QBuffer buffer(data);
    QZipReader reader(&buffer);
    return reader.extractAll(path);
}

static bool extractAllWrapper(QByteArray *data, const QString &path)
{
    QEventLoop loop;
    QFutureWatcher<bool> watcher;
    QObject::connect(&watcher, &QFutureWatcher<bool>::finished, &loop, &QEventLoop::quit);
    watcher.setFuture(QtConcurrent::run(extractAll, data, path));
    loop.exec();
    return watcher.result();
}
#endif

void OpenMVPlugin::packageUpdate()
{

#if(0) // DGP
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    connect(manager, &QNetworkAccessManager::finished, this, [this] (QNetworkReply *reply) {

        QByteArray data = reply->readAll();

        if((reply->error() == QNetworkReply::NoError) && (!data.isEmpty()))
        {
            QRegularExpressionMatch match = QRegularExpression(QStringLiteral("(\\d+)\\.(\\d+)\\.(\\d+)")).match(QString::fromUtf8(data));

            int new_major = match.captured(1).toInt();
            int new_minor = match.captured(2).toInt();
            int new_patch = match.captured(3).toInt();

            QSettings *settings = ExtensionSystem::PluginManager::settings();
            settings->beginGroup(QStringLiteral(SETTINGS_GROUP));

            int old_major = settings->value(QStringLiteral(RESOURCES_MAJOR)).toInt();
            int old_minor = settings->value(QStringLiteral(RESOURCES_MINOR)).toInt();
            int old_patch = settings->value(QStringLiteral(RESOURCES_PATCH)).toInt();

            settings->endGroup();

            if((old_major < new_major)
            || ((old_major == new_major) && (old_minor < new_minor))
            || ((old_major == new_major) && (old_minor == new_minor) && (old_patch < new_patch)))
            {
                QMessageBox box(QMessageBox::Information, tr("Update Available"),
                    tr("New NXTCamView5 resources are available (e.g. examples, firmware, documentation, etc.)."),
                    QMessageBox::Cancel, Core::ICore::dialogParent(),
                    Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                    (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));
                QPushButton *button = box.addButton(tr("Install"), QMessageBox::AcceptRole);
                box.setDefaultButton(button);
                box.setEscapeButton(QMessageBox::Cancel);
                box.exec();

                if(box.clickedButton() == button)
                {
                    QProgressDialog *dialog = new QProgressDialog(tr("Installing..."), tr("Cancel"), 0, 0, Core::ICore::dialogParent(),
                        Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint |
                        (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowType(0)));
                    dialog->setWindowModality(Qt::ApplicationModal);
                    dialog->setAttribute(Qt::WA_ShowWithoutActivating);
                    dialog->setCancelButton(Q_NULLPTR);

                    QNetworkAccessManager *manager2 = new QNetworkAccessManager(this);

                    connect(manager2, &QNetworkAccessManager::finished, this, [this, new_major, new_minor, new_patch, dialog] (QNetworkReply *reply2) {

                        QByteArray data2 = reply2->readAll();

                        if((reply2->error() == QNetworkReply::NoError) && (!data2.isEmpty()))
                        {
                            QSettings *settings2 = ExtensionSystem::PluginManager::settings();
                            settings2->beginGroup(QStringLiteral(SETTINGS_GROUP));

                            settings2->setValue(QStringLiteral(RESOURCES_MAJOR), 0);
                            settings2->setValue(QStringLiteral(RESOURCES_MINOR), 0);
                            settings2->setValue(QStringLiteral(RESOURCES_PATCH), 0);
                            settings2->sync();

                            bool ok = true;

                            QString error;

                            if(!removeRecursivelyWrapper(Utils::FileName::fromString(Core::ICore::userResourcePath()), &error))
                            {
                                QMessageBox::critical(Core::ICore::dialogParent(),
                                    QString(),
                                    error + tr("\n\nPlease close any programs that are viewing/editing NXTCamView5's application data and then restart NXTCamView5!"));

                                QApplication::quit();
                                ok = false;
                            }
                            else
                            {
                                if(!extractAllWrapper(&data2, Core::ICore::userResourcePath()))
                                {
                                    QMessageBox::critical(Core::ICore::dialogParent(),
                                        QString(),
                                        tr("Please close any programs that are viewing/editing NXTCamView5's application data and then restart NXTCamView5!"));

                                    QApplication::quit();
                                    ok = false;
                                }
                            }

                            if(ok)
                            {
                                settings2->setValue(QStringLiteral(RESOURCES_MAJOR), new_major);
                                settings2->setValue(QStringLiteral(RESOURCES_MINOR), new_minor);
                                settings2->setValue(QStringLiteral(RESOURCES_PATCH), new_patch);
                                settings2->sync();

                                QMessageBox::information(Core::ICore::dialogParent(),
                                    QString(),
                                    tr("Installation Sucessful! Please restart NXTCamView5."));

                                QApplication::quit();
                            }

                            settings2->endGroup();
                        }
                        else if(reply2->error() != QNetworkReply::NoError)
                        {
                            QMessageBox::critical(Core::ICore::dialogParent(),
                                tr("Package Update"),
                                tr("Error: %L1!").arg(reply2->errorString()));
                        }
                        else
                        {
                            QMessageBox::critical(Core::ICore::dialogParent(),
                                tr("Package Update"),
                                tr("Cannot open the resources file \"%L1\"!").arg(reply2->request().url().toString()));
                        }

                        reply2->deleteLater();

                        delete dialog;
                    });

                    QNetworkRequest request2 = QNetworkRequest(QUrl(QStringLiteral("http://upload.openmv.io/openmv-ide-resources-%L1.%L2.%L3/openmv-ide-resources-%L1.%L2.%L3.zip").arg(new_major).arg(new_minor).arg(new_patch)));
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
                    request2.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#endif
                    QNetworkReply *reply2 = manager2->get(request2);

                    if(reply2)
                    {
                        connect(reply2, &QNetworkReply::sslErrors, reply2, static_cast<void (QNetworkReply::*)(void)>(&QNetworkReply::ignoreSslErrors));
                        dialog->show();
                    }
                    else
                    {
                        QMessageBox::critical(Core::ICore::dialogParent(),
                            tr("Package Update"),
                            tr("Network request failed \"%L1\"!").arg(request2.url().toString()));
                    }
                }
            }
        }

        reply->deleteLater();
    });

    QNetworkRequest request = QNetworkRequest(QUrl(QStringLiteral("http://upload.openmv.io/openmv-ide-resources-version.txt")));
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#endif
    QNetworkReply *reply = manager->get(request);

    if(reply)
    {
        connect(reply, &QNetworkReply::sslErrors, reply, static_cast<void (QNetworkReply::*)(void)>(&QNetworkReply::ignoreSslErrors));
    }
#endif
}

void OpenMVPlugin::bootloaderClicked()
{
    QDialog *dialog = new QDialog(Core::ICore::dialogParent(),
        Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
        (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));
    dialog->setWindowTitle(tr("Bootloader"));
    QFormLayout *layout = new QFormLayout(dialog);
    layout->setVerticalSpacing(0);

    QSettings *settings = ExtensionSystem::PluginManager::settings();
    settings->beginGroup(QStringLiteral(SETTINGS_GROUP));

    Utils::PathChooser *pathChooser = new Utils::PathChooser();
    pathChooser->setExpectedKind(Utils::PathChooser::File);
    pathChooser->setPromptDialogTitle(tr("Firmware Path"));
    pathChooser->setPromptDialogFilter(tr("Firmware Binary (*.bin *.dfu)"));
    pathChooser->setFileName(Utils::FileName::fromString(settings->value(QStringLiteral(LAST_FIRMWARE_PATH), QDir::homePath()).toString()));
    layout->addRow(tr("Firmware Path"), pathChooser);
    layout->addItem(new QSpacerItem(0, 6));

    QCheckBox *checkBox = new QCheckBox(tr("Erase internal file system"));
    checkBox->setChecked(settings->value(QStringLiteral(LAST_FLASH_FS_ERASE_STATE), false).toBool());
    checkBox->setVisible(!pathChooser->path().endsWith(QStringLiteral(".dfu"), Qt::CaseInsensitive));
    layout->addRow(checkBox);
    QCheckBox *checkBox2 = new QCheckBox(tr("Erase internal file system"));
    checkBox2->setChecked(true);
    checkBox2->setEnabled(false);
    checkBox2->setVisible(pathChooser->path().endsWith(QStringLiteral(".dfu"), Qt::CaseInsensitive));
    layout->addRow(checkBox2);
    layout->addItem(new QSpacerItem(0, 6));

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Cancel);
    QPushButton *run = new QPushButton(tr("Run"));
    run->setEnabled(pathChooser->isValid());
    box->addButton(run, QDialogButtonBox::AcceptRole);
    layout->addRow(box);

    connect(box, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(pathChooser, &Utils::PathChooser::validChanged, run, &QPushButton::setEnabled);
    connect(pathChooser, &Utils::PathChooser::pathChanged, this, [this, dialog, checkBox, checkBox2] (const QString &path) {
        if(path.endsWith(QStringLiteral(".dfu"), Qt::CaseInsensitive))
        {
            checkBox->setVisible(false);
            checkBox2->setVisible(true);
        }
        else
        {
            checkBox->setVisible(true);
            checkBox2->setVisible(false);

        }
        dialog->adjustSize();
    });

    if(dialog->exec() == QDialog::Accepted)
    {
        QString forceFirmwarePath = pathChooser->path();
        bool flashFSErase = checkBox->isChecked();

        if(QFileInfo(forceFirmwarePath).isFile())
        {
            settings->setValue(QStringLiteral(LAST_FIRMWARE_PATH), forceFirmwarePath);
            settings->setValue(QStringLiteral(LAST_FLASH_FS_ERASE_STATE), flashFSErase);
            settings->endGroup();
            delete dialog;

            connectClicked(true, forceFirmwarePath, (flashFSErase || forceFirmwarePath.endsWith(QStringLiteral(".dfu"), Qt::CaseInsensitive)) ? QMessageBox::Yes : QMessageBox::No);
        }
        else
        {
            QMessageBox::critical(Core::ICore::dialogParent(),
                tr("Bootloader"),
                tr("\"%L1\" is not a file!").arg(forceFirmwarePath));

            settings->endGroup();
            delete dialog;
        }
    }
    else
    {
        settings->endGroup();
        delete dialog;
    }
}

#define CONNECT_END() \
do { \
    m_working = false; \
    QTimer::singleShot(0, this, &OpenMVPlugin::workingDone); \
    return; \
} while(0)

#define RECONNECT_END() \
do { \
    m_working = false; \
    QTimer::singleShot(0, this, [this] {connectClicked();}); \
    return; \
} while(0)

#define CLOSE_CONNECT_END() \
do { \
    QEventLoop m_loop; \
    connect(m_iodevice, &OpenMVPluginIO::closeResponse, &m_loop, &QEventLoop::quit); \
    m_iodevice->close(); \
    m_loop.exec(); \
    m_working = false; \
    QTimer::singleShot(0, this, &OpenMVPlugin::workingDone); \
    return; \
} while(0)

#define CLOSE_RECONNECT_END() \
do { \
    QEventLoop m_loop; \
    connect(m_iodevice, &OpenMVPluginIO::closeResponse, &m_loop, &QEventLoop::quit); \
    m_iodevice->close(); \
    m_loop.exec(); \
    m_working = false; \
    QTimer::singleShot(0, this, [this] {connectClicked();}); \
    return; \
} while(0)

void OpenMVPlugin::connectClicked(bool forceBootloader, QString forceFirmwarePath, int forceFlashFSErase)
{
    if(!m_working)
    {
        m_working = true;

        QStringList stringList;

        foreach(QSerialPortInfo port, QSerialPortInfo::availablePorts())
        {
            if(port.hasVendorIdentifier() && (port.vendorIdentifier() == OPENMVCAM_VID)
            && port.hasProductIdentifier() && 
            (port.productIdentifier() == OPENMVCAM_PID) || 
            (port.productIdentifier() == NXTCAM_PID))
            {
                stringList.append(port.portName());
            }
        }

        if(Utils::HostOsInfo::isMacHost())
        {
            stringList = stringList.filter(QStringLiteral("cu"), Qt::CaseInsensitive);
        }

        QSettings *settings = ExtensionSystem::PluginManager::settings();
        settings->beginGroup(QStringLiteral(SETTINGS_GROUP));

        QString selectedPort;
        bool forceBootloaderBricked = false;
        QString firmwarePath = forceFirmwarePath;

        if(stringList.isEmpty())
        {
            if(forceBootloader)
            {
                forceBootloaderBricked = true;
            }
            else
            {
                QMessageBox::critical(Core::ICore::dialogParent(),
                    tr("Connect"),
                    tr("No NXTCam5 found!"));

                QFile boards(Core::ICore::userResourcePath() + QStringLiteral("/firmware/boards.txt"));

                if(boards.open(QIODevice::ReadOnly))
                {
                    QMap<QString, QString> mappings;

                    forever
                    {
                        QByteArray data = boards.readLine();

                        if((boards.error() == QFile::NoError) && (!data.isEmpty()))
                        {
                            QRegularExpressionMatch mapping = QRegularExpression(QStringLiteral("(\\S+)\\s+(\\S+)\\s+(\\S+)")).match(QString::fromUtf8(data));
                            mappings.insert(mapping.captured(2).replace(QStringLiteral("_"), QStringLiteral(" ")), mapping.captured(3).replace(QStringLiteral("_"), QStringLiteral(" ")));
                        }
                        else
                        {
                            boards.close();
                            break;
                        }
                    }

                    /*
                    if(!mappings.isEmpty())
                    {
                        if(QMessageBox::question(Core::ICore::dialogParent(),
                            tr("Connect"),
                            tr("Do you have an NXTCam5 connected and is it bricked?"),
                            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes)
                        == QMessageBox::Yes)
                        {
                            int index = mappings.keys().indexOf(settings->value(QStringLiteral(LAST_BOARD_TYPE_STATE)).toString());

                            bool ok;
                            QString temp = QInputDialog::getItem(Core::ICore::dialogParent(),
                                tr("Connect"), tr("Please select the board type"),
                                mappings.keys(), (index != -1) ? index : 0, false, &ok,
                                Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                                (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));

                            if(ok)
                            {
                                settings->setValue(QStringLiteral(LAST_BOARD_TYPE_STATE), temp);

                                int answer = QMessageBox::question(Core::ICore::dialogParent(),
                                    tr("Connect"),
                                    tr("Erase the internal file system?"),
                                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::No);

                                if((answer == QMessageBox::Yes) || (answer == QMessageBox::No))
                                {
                                    forceBootloader = true;
                                    forceFlashFSErase = answer;
                                    forceBootloaderBricked = true;
                                    firmwarePath = Core::ICore::userResourcePath() + QStringLiteral("/firmware/") + mappings.value(temp) + QStringLiteral("/firmware.bin");
                                }
                            }
                        }
                    }*/
                }
            }
        }
        else if(stringList.size() == 1)
        {
            selectedPort = stringList.first();
            settings->setValue(QStringLiteral(LAST_SERIAL_PORT_STATE), selectedPort);
        }
        else
        {
            int index = stringList.indexOf(settings->value(QStringLiteral(LAST_SERIAL_PORT_STATE)).toString());

            bool ok;
            QString temp = QInputDialog::getItem(Core::ICore::dialogParent(),
                tr("Connect"), tr("Please select a serial port"),
                stringList, (index != -1) ? index : 0, false, &ok,
                Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));

            if(ok)
            {
                selectedPort = temp;
                settings->setValue(QStringLiteral(LAST_SERIAL_PORT_STATE), selectedPort);
            }
        }

        settings->endGroup();

        if((!forceBootloaderBricked) && selectedPort.isEmpty())
        {
            CONNECT_END();
        }

        // Open Port //////////////////////////////////////////////////////////

        if(!forceBootloaderBricked)
        {
            QString errorMessage2 = QString();
            QString *errorMessage2Ptr = &errorMessage2;

            QMetaObject::Connection conn = connect(m_ioport, &OpenMVPluginSerialPort::openResult,
                this, [this, errorMessage2Ptr] (const QString &errorMessage) {
                *errorMessage2Ptr = errorMessage;
            });

            QProgressDialog dialog(tr("Connecting..."), tr("Cancel"), 0, 0, Core::ICore::dialogParent(),
               Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint |
               (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowType(0)));
            dialog.setWindowModality(Qt::ApplicationModal);
            dialog.setAttribute(Qt::WA_ShowWithoutActivating);

            forever
            {
                QEventLoop loop;

                connect(m_ioport, &OpenMVPluginSerialPort::openResult,
                        &loop, &QEventLoop::quit);

                m_ioport->open(selectedPort);

                loop.exec();

                if(errorMessage2.isEmpty() || (Utils::HostOsInfo::isLinuxHost() && errorMessage2.contains(QStringLiteral("Permission Denied"), Qt::CaseInsensitive)))
                {
                    break;
                }

                dialog.show();

                QApplication::processEvents();

                if(dialog.wasCanceled())
                {
                    break;
                }
            }

            dialog.close();

            disconnect(conn);

            if(!errorMessage2.isEmpty())
            {
                QMessageBox::critical(Core::ICore::dialogParent(),
                    tr("Connect"),
                    tr("Error: %L1!").arg(errorMessage2));

                if(Utils::HostOsInfo::isLinuxHost() && errorMessage2.contains(QStringLiteral("Permission Denied"), Qt::CaseInsensitive))
                {
                    QMessageBox::information(Core::ICore::dialogParent(),
                        tr("Connect"),
                        tr("Try doing:\n\nsudo adduser %L1 dialout\n\n...in a terminal and then restart your computer.").arg(Utils::Environment::systemEnvironment().userName()));
                }

                CONNECT_END();
            }
        }

        // Get Version ////////////////////////////////////////////////////////

        int major2 = int();
        int minor2 = int();
        int patch2 = int();

        if(!forceBootloaderBricked)
        {
            int *major2Ptr = &major2;
            int *minor2Ptr = &minor2;
            int *patch2Ptr = &patch2;

            QMetaObject::Connection conn = connect(m_iodevice, &OpenMVPluginIO::firmwareVersion,
                this, [this, major2Ptr, minor2Ptr, patch2Ptr] (int major, int minor, int patch) {
                *major2Ptr = major;
                *minor2Ptr = minor;
                *patch2Ptr = patch;
            });

            QEventLoop loop;

            connect(m_iodevice, &OpenMVPluginIO::firmwareVersion,
                    &loop, &QEventLoop::quit);

            m_iodevice->getFirmwareVersion();

            loop.exec();

            disconnect(conn);

            if((!major2) && (!minor2) && (!patch2))
            {
                QMessageBox::critical(Core::ICore::dialogParent(),
                    tr("Connect"),
                    tr("Timeout error while getting firmware version!"));

                //QMessageBox::warning(Core::ICore::dialogParent(),
                    //tr("Connect"),
                    //tr("Do not try to connect while the green light on your OpenMV Cam is on!"));

                if(QMessageBox::question(Core::ICore::dialogParent(),
                    tr("Connect"),
                    tr("Try to connect again?"),
                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes)
                == QMessageBox::Yes)
                {
                    CLOSE_RECONNECT_END();
                }

                CLOSE_CONNECT_END();
            }
            else if((major2 < 0) || (100 < major2) || (minor2 < 0) || (100 < minor2) || (patch2 < 0) || (100 < patch2))
            {
                CLOSE_RECONNECT_END();
            }
        }

        // Bootloader /////////////////////////////////////////////////////////

        if(forceBootloader)
        {
            if(!forceBootloaderBricked)
            {
                if(firmwarePath.isEmpty())
                {
                    if((major2 < OLD_API_MAJOR)
                    || ((major2 == OLD_API_MAJOR) && (minor2 < OLD_API_MINOR))
                    || ((major2 == OLD_API_MAJOR) && (minor2 == OLD_API_MINOR) && (patch2 < OLD_API_PATCH)))
                    {
                        firmwarePath = Core::ICore::userResourcePath() + QStringLiteral("/firmware/") + QStringLiteral(OLD_API_BOARD) + QStringLiteral("/firmware.bin");
                    }
                    else
                    {
                        QString arch2 = QString();
                        QString *arch2Ptr = &arch2;

                        QMetaObject::Connection conn = connect(m_iodevice, &OpenMVPluginIO::archString,
                            this, [this, arch2Ptr] (const QString &arch) {
                            *arch2Ptr = arch;
                        });

                        QEventLoop loop;

                        connect(m_iodevice, &OpenMVPluginIO::archString,
                                &loop, &QEventLoop::quit);

                        m_iodevice->getArchString();

                        loop.exec();

                        disconnect(conn);

                        if(!arch2.isEmpty())
                        {
                            QFile boards(Core::ICore::userResourcePath() + QStringLiteral("/firmware/boards.txt"));

                            if(boards.open(QIODevice::ReadOnly))
                            {
                                QMap<QString, QString> mappings;

                                forever
                                {
                                    QByteArray data = boards.readLine();

                                    if((boards.error() == QFile::NoError) && (!data.isEmpty()))
                                    {
                                        QRegularExpressionMatch mapping = QRegularExpression(QStringLiteral("(\\S+)\\s+(\\S+)\\s+(\\S+)")).match(QString::fromUtf8(data));
                                        mappings.insert(mapping.captured(1).replace(QStringLiteral("_"), QStringLiteral(" ")), mapping.captured(3).replace(QStringLiteral("_"), QStringLiteral(" ")));
                                    }
                                    else
                                    {
                                        boards.close();
                                        break;
                                    }
                                }

                                QString value = mappings.value(arch2.simplified().replace(QStringLiteral("_"), QStringLiteral(" ")));

                                if(!value.isEmpty())
                                {
                                    firmwarePath = Core::ICore::userResourcePath() + QStringLiteral("/firmware/") + value + QStringLiteral("/firmware.bin");
                                }
                                else
                                {
                                    QMessageBox::critical(Core::ICore::dialogParent(),
                                        tr("Connect"),
                                        tr("Unsupported board architecture!"));

                                    CLOSE_CONNECT_END();
                                }
                            }
                            else
                            {
                                QMessageBox::critical(Core::ICore::dialogParent(),
                                    tr("Connect"),
                                    tr("Error: %L1!").arg(boards.errorString()));

                                CLOSE_CONNECT_END();
                            }
                        }
                        else
                        {
                            QMessageBox::critical(Core::ICore::dialogParent(),
                                tr("Connect"),
                                tr("Timeout error while getting board architecture!"));

                            CLOSE_CONNECT_END();
                        }
                    }
                }

                if(firmwarePath.endsWith(QStringLiteral(".dfu"), Qt::CaseInsensitive))
                {
                    QEventLoop loop;

                    connect(m_iodevice, &OpenMVPluginIO::closeResponse,
                            &loop, &QEventLoop::quit);

                    m_iodevice->close();

                    loop.exec();
                }
            }

            // BIN Bootloader /////////////////////////////////////////////////

            while(firmwarePath.endsWith(QStringLiteral(".bin"), Qt::CaseInsensitive))
            {
                QFile file(firmwarePath);

                if(file.open(QIODevice::ReadOnly))
                {
                    QByteArray data = file.readAll();

                    if((file.error() == QFile::NoError) && (!data.isEmpty()))
                    {
                        file.close();

                        QList<QByteArray> dataChunks;

                        for(int i = 0; i < data.size(); i += FLASH_WRITE_CHUNK_SIZE)
                        {
                            dataChunks.append(data.mid(i, qMin(FLASH_WRITE_CHUNK_SIZE, data.size() - i)));
                        }

                        if(dataChunks.last().size() % FLASH_WRITE_CHUNK_SIZE)
                        {
                            dataChunks.last().append(QByteArray(FLASH_WRITE_CHUNK_SIZE - dataChunks.last().size(), 255));
                        }

                        // Start Bootloader ///////////////////////////////////
                        {
                            bool done2 = bool(), loopExit = false, done22 = false;
                            bool *done2Ptr = &done2, *loopExitPtr = &loopExit, *done2Ptr2 = &done22;

                            QMetaObject::Connection conn = connect(m_ioport, &OpenMVPluginSerialPort::bootloaderStartResponse,
                                this, [this, done2Ptr, loopExitPtr] (bool done) {
                                *done2Ptr = done;
                                *loopExitPtr = true;
                            });

                            QMetaObject::Connection conn2 = connect(m_ioport, &OpenMVPluginSerialPort::bootloaderStopResponse,
                                this, [this, done2Ptr2] {
                                *done2Ptr2 = true;
                            });

                            QProgressDialog dialog(forceBootloaderBricked ?  tr("Disconnect your NXTCam5 and then reconnect it...") : tr("Connecting..."), tr("Cancel"), 0, 0, Core::ICore::dialogParent(),
                                Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint |
                                (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowType(0)));
                            dialog.setWindowModality(Qt::ApplicationModal);
                            dialog.setAttribute(Qt::WA_ShowWithoutActivating);
                            dialog.show();

                            connect(&dialog, &QProgressDialog::canceled,
                                    m_ioport, &OpenMVPluginSerialPort::bootloaderStop);

                            QEventLoop loop, loop0, loop1;

                            connect(m_ioport, &OpenMVPluginSerialPort::bootloaderStartResponse,
                                    &loop, &QEventLoop::quit);

                            connect(m_ioport, &OpenMVPluginSerialPort::bootloaderStopResponse,
                                    &loop0, &QEventLoop::quit);

                            connect(m_ioport, &OpenMVPluginSerialPort::bootloaderResetResponse,
                                    &loop1, &QEventLoop::quit);

                            m_ioport->bootloaderStart(selectedPort);

                            // NOT loop.exec();
                            while(!loopExit)
                            {
                                QSerialPortInfo::availablePorts();
                                QApplication::processEvents();
                                // Keep updating the list of available serial
                                // ports for the non-gui serial thread.
                            }

                            dialog.close();

                            if(!done22)
                            {
                                loop0.exec();
                            }

                            m_ioport->bootloaderReset();

                            loop1.exec();

                            disconnect(conn);

                            disconnect(conn2);

                            if(!done2)
                            {
                                QMessageBox::critical(Core::ICore::dialogParent(),
                                    tr("Connect"),
                                    tr("Unable to connect to your NXTCam5's normal bootloader!"));

                                if(forceFirmwarePath.isEmpty() && QMessageBox::question(Core::ICore::dialogParent(),
                                    tr("Connect"),
                                    tr("NXTCamView5 can still try to upgrade your NXTCam5 using your its DFU Bootloader.\n\n"
                                       "Continue?"),
                                    QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok)
                                == QMessageBox::Ok)
                                {
                                    firmwarePath = QFileInfo(firmwarePath).path() + QStringLiteral("/openmv.dfu");
                                    break;
                                }

                                CONNECT_END();
                            }
                        }

                        // Erase Flash ////////////////////////////////////////
                        {
                            int flash_start = forceFlashFSErase ? FLASH_SECTOR_ALL_START : FLASH_SECTOR_START;
                            int flash_end = forceFlashFSErase ? FLASH_SECTOR_ALL_END : FLASH_SECTOR_END;

                            bool ok2 = bool();
                            bool *ok2Ptr = &ok2;

                            QMetaObject::Connection conn2 = connect(m_iodevice, &OpenMVPluginIO::flashEraseDone,
                                this, [this, ok2Ptr] (bool ok) {
                                *ok2Ptr = ok;
                            });

                            QProgressDialog dialog(tr("Erasing..."), tr("Cancel"), flash_start, flash_end, Core::ICore::dialogParent(),
                                Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint |
                                (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowType(0)));
                            dialog.setWindowModality(Qt::ApplicationModal);
                            dialog.setAttribute(Qt::WA_ShowWithoutActivating);
                            dialog.setCancelButton(Q_NULLPTR);
                            dialog.show();

                            for(int i = flash_start; i <= flash_end; i++)
                            {
                                QEventLoop loop0, loop1;

                                connect(m_iodevice, &OpenMVPluginIO::flashEraseDone,
                                        &loop0, &QEventLoop::quit);

                                m_iodevice->flashErase(i);

                                loop0.exec();

                                if(!ok2)
                                {
                                    break;
                                }

                                QTimer::singleShot(FLASH_ERASE_DELAY, &loop1, &QEventLoop::quit);

                                loop1.exec();

                                dialog.setValue(i);
                            }

                            dialog.close();

                            disconnect(conn2);

                            if(!ok2)
                            {
                                QMessageBox::critical(Core::ICore::dialogParent(),
                                    tr("Connect"),
                                    tr("Timeout Error!"));

                                CLOSE_CONNECT_END();
                            }
                        }

                        // Program Flash //////////////////////////////////////
                        {
                            bool ok2 = bool();
                            bool *ok2Ptr = &ok2;

                            QMetaObject::Connection conn2 = connect(m_iodevice, &OpenMVPluginIO::flashWriteDone,
                                this, [this, ok2Ptr] (bool ok) {
                                *ok2Ptr = ok;
                            });

                            QProgressDialog dialog(tr("Programming..."), tr("Cancel"), 0, dataChunks.size() - 1, Core::ICore::dialogParent(),
                                Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint |
                                (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowType(0)));
                            dialog.setWindowModality(Qt::ApplicationModal);
                            dialog.setAttribute(Qt::WA_ShowWithoutActivating);
                            dialog.setCancelButton(Q_NULLPTR);
                            dialog.show();

                            for(int i = 0; i < dataChunks.size(); i++)
                            {
                                QEventLoop loop0, loop1;

                                connect(m_iodevice, &OpenMVPluginIO::flashWriteDone,
                                        &loop0, &QEventLoop::quit);

                                m_iodevice->flashWrite(dataChunks.at(i));

                                loop0.exec();

                                if(!ok2)
                                {
                                    break;
                                }

                                QTimer::singleShot(FLASH_WRITE_DELAY, &loop1, &QEventLoop::quit);

                                loop1.exec();

                                dialog.setValue(i);
                            }

                            dialog.close();

                            disconnect(conn2);

                            if(!ok2)
                            {
                                QMessageBox::critical(Core::ICore::dialogParent(),
                                    tr("Connect"),
                                    tr("Timeout Error!"));

                                CLOSE_CONNECT_END();
                            }
                        }

                        // Reset Bootloader ///////////////////////////////////
                        {
                            QProgressDialog dialog(tr("Programming..."), tr("Cancel"), 0, 0, Core::ICore::dialogParent(),
                                Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint |
                                (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowType(0)));
                            dialog.setWindowModality(Qt::ApplicationModal);
                            dialog.setAttribute(Qt::WA_ShowWithoutActivating);
                            dialog.setCancelButton(Q_NULLPTR);
                            dialog.show();

                            QEventLoop loop;

                            connect(m_iodevice, &OpenMVPluginIO::closeResponse,
                                    &loop, &QEventLoop::quit);

                            m_iodevice->bootloaderReset();
                            m_iodevice->close();

                            loop.exec();

                            dialog.close();

                            QMessageBox::information(Core::ICore::dialogParent(),
                                tr("Connect"),
                                tr("Done upgrading your NXTCam5's firmware!\n\n"
                                   "Click the Ok button after your NXTCam5 has enumerated and finished running its built-in self test (blue led blinking - this takes a while)."));

                            RECONNECT_END();
                        }
                    }
                    else if(file.error() != QFile::NoError)
                    {
                        QMessageBox::critical(Core::ICore::dialogParent(),
                            tr("Connect"),
                            tr("Error: %L1!").arg(file.errorString()));

                        if(forceBootloaderBricked)
                        {
                            CONNECT_END();
                        }
                        else
                        {
                            CLOSE_CONNECT_END();
                        }
                    }
                    else
                    {
                        QMessageBox::critical(Core::ICore::dialogParent(),
                            tr("Connect"),
                            tr("The firmware file is empty!"));

                        if(forceBootloaderBricked)
                        {
                            CONNECT_END();
                        }
                        else
                        {
                            CLOSE_CONNECT_END();
                        }
                    }
                }
                else
                {
                    QMessageBox::critical(Core::ICore::dialogParent(),
                        tr("Connect"),
                        tr("Error: %L1!").arg(file.errorString()));

                    if(forceBootloaderBricked)
                    {
                        CONNECT_END();
                    }
                    else
                    {
                        CLOSE_CONNECT_END();
                    }
                }
            }

            // DFU Bootloader /////////////////////////////////////////////////

            if(firmwarePath.endsWith(QStringLiteral(".dfu"), Qt::CaseInsensitive))
            {
                if(forceFlashFSErase || (QMessageBox::warning(Core::ICore::dialogParent(),
                    tr("Connect"),
                    tr("DFU update erases your NXTCam5's internal flash file system.\n\n"
                       "Backup your data before continuing!"),
                    QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok)
                == QMessageBox::Ok))
                {
                    if(QMessageBox::information(Core::ICore::dialogParent(),
                        tr("Connect"),
                        tr("Disconnect your NXTCam5 from your computer, add a jumper wire between the BOOT and RST pins, and then reconnect your NXTCam5 to your computer.\n\n"
                           "Click the Ok button after your NXTCam5's DFU Bootloader has enumerated."),
                        QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok)
                    == QMessageBox::Ok)
                    {
                        QProgressDialog dialog(tr("Reprogramming...\n\n(may take up to 5 minutes)"), tr("Cancel"), 0, 0, Core::ICore::dialogParent(),
                            Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint |
                            (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowType(0)));
                        dialog.setWindowModality(Qt::ApplicationModal);
                        dialog.setAttribute(Qt::WA_ShowWithoutActivating);
                        dialog.setCancelButton(Q_NULLPTR);
                        dialog.show();

                        QString command;
                        Utils::SynchronousProcess process;
                        Utils::SynchronousProcessResponse response;
                        process.setTimeoutS(300); // 5 minutes...
                        process.setProcessChannelMode(QProcess::MergedChannels);

                        if(Utils::HostOsInfo::isWindowsHost())
                        {
                            for(int i = 0; i < 10; i++) // try multiple times...
                            {
                                command = QDir::cleanPath(QDir::toNativeSeparators(Core::ICore::resourcePath() + QStringLiteral("/dfuse/DfuSeCommand.exe")));
                                response = process.run(command, QStringList()
                                    << QStringLiteral("-c")
                                    << QStringLiteral("-d")
                                    << QStringLiteral("--v")
                                    << QStringLiteral("--o")
                                    << QStringLiteral("--fn")
                                    << QDir::cleanPath(QDir::toNativeSeparators(firmwarePath)));

                                if(response.result == Utils::SynchronousProcessResponse::Finished)
                                {
                                    break;
                                }
                                else
                                {
                                    QApplication::processEvents();
                                }
                            }
                        }
                        else
                        {
                            for(int i = 0; i < 10; i++) // try multiple times...
                            {
                                command = QDir::cleanPath(QDir::toNativeSeparators(Core::ICore::resourcePath() + QStringLiteral("/pydfu/pydfu.py")));
                                response = process.run(QStringLiteral("python"), QStringList()
                                    << command
                                    << QStringLiteral("-u")
                                    << QDir::cleanPath(QDir::toNativeSeparators(firmwarePath)));

                                if(response.result == Utils::SynchronousProcessResponse::Finished)
                                {
                                    break;
                                }
                                else
                                {
                                    QApplication::processEvents();
                                }
                            }
                        }

                        if(response.result == Utils::SynchronousProcessResponse::Finished)
                        {
                            QMessageBox::information(Core::ICore::dialogParent(),
                                tr("Connect"),
                                tr("DFU firmware update complete!\n\n") +
                                (Utils::HostOsInfo::isWindowsHost() ?  tr("Disconnect your NXTCam5 from your computer, remove the jumper wire between the BOOT and RST pins, and then reconnect your NXTCam5 to your computer.\n\n") : QString()) +
                                tr("Click the Ok button after your NXTCam5 has enumerated and finished running its built-in self test (blue led blinking - this takes a while)."));

                            RECONNECT_END();
                        }
                        else
                        {
                            QMessageBox box(QMessageBox::Critical, tr("Connect"), tr("DFU firmware update failed!"), QMessageBox::Ok, Core::ICore::dialogParent(),
                                Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                                (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));
                            box.setDetailedText(response.stdOut);
                            box.setInformativeText(response.exitMessage(command, process.timeoutS()));
                            box.setDefaultButton(QMessageBox::Ok);
                            box.setEscapeButton(QMessageBox::Cancel);
                            box.exec();

                            if(Utils::HostOsInfo::isMacHost())
                            {
                                QMessageBox::information(Core::ICore::dialogParent(),
                                    tr("Connect"),
                                    tr("PyDFU requires the following libraries to be installed:\n\n"
                                       "MacPorts:\n"
                                       "    sudo port install libusb py-pip\n"
                                       "    sudo pip install pyusb\n\n"
                                       "HomeBrew:\n"
                                       "    sudo brew install libusb python\n"
                                       "    sudo pip install pyusb"));
                            }

                            if(Utils::HostOsInfo::isLinuxHost())
                            {
                                QMessageBox::information(Core::ICore::dialogParent(),
                                    tr("Connect"),
                                    tr("PyDFU requires the following libraries to be installed:\n\n"
                                       "    sudo apt-get install libusb-1.0 python-pip\n"
                                       "    sudo pip install pyusb"));
                            }
                        }
                    }
                }

                CONNECT_END();
            }
        }

        // Stopping ///////////////////////////////////////////////////////////

        m_iodevice->scriptStop();

        m_iodevice->jpegEnable(m_jpgCompress->isChecked());
        m_iodevice->fbEnable(!m_disableFrameBuffer->isChecked());

        Core::MessageManager::grayOutOldContent();

        ///////////////////////////////////////////////////////////////////////

        m_iodevice->getTimeout(); // clear

        m_frameSizeDumpTimer.restart();
        m_getScriptRunningTimer.restart();
        m_getTxBufferTimer.restart();

        m_timer.restart();
        m_queue.clear();
        m_connected = true;
        m_running = false;
        m_portName = selectedPort;
        m_portPath = QString();
        m_major = major2;
        m_minor = minor2;
        m_patch = patch2;
        m_errorFilterString = QString();

        m_bootloaderCommand->action()->setEnabled(false);
        m_saveCommand->action()->setEnabled(false);
        m_resetCommand->action()->setEnabled(true);
        m_connectCommand->action()->setEnabled(false);
        m_connectCommand->action()->setVisible(false);
        m_disconnectCommand->action()->setEnabled(true);
        m_disconnectCommand->action()->setVisible(true);
        Core::IEditor *editor = Core::EditorManager::currentEditor();
        m_startCommand->action()->setEnabled(editor ? (editor->document() ? (!editor->document()->contents().isEmpty()) : false) : false);
        m_startCommand->action()->setVisible(true);
        m_stopCommand->action()->setEnabled(false);
        m_stopCommand->action()->setVisible(false);

        m_statusLabel->setEnabled(true);
        statusUpdate(tr("NXTCam5 Connected."));

        m_versionButton->setEnabled(true);
        m_versionButton->setText(tr("Firmware Version: %L1.%L2.%L3").arg(major2).arg(minor2).arg(patch2));
        m_portLabel->setEnabled(true);
        m_portLabel->setText(tr("Serial Port: %L1").arg(m_portName));
        m_pathButton->setEnabled(true);
        m_pathButton->setText(tr("Drive:"));
        m_fpsLabel->setEnabled(true);
        m_fpsLabel->setText(tr("FPS: 0"));

        m_frameBuffer->enableSaveTemplate(false);
        m_frameBuffer->enableSaveDescriptor(false);

        // Check Version //////////////////////////////////////////////////////

        QFile file(Core::ICore::userResourcePath() + QStringLiteral("/firmware/firmware.txt"));

        if(file.open(QIODevice::ReadOnly))
        {
            QByteArray data = file.readAll();

            if((file.error() == QFile::NoError) && (!data.isEmpty()))
            {
                file.close();

                QRegularExpressionMatch match = QRegularExpression(QStringLiteral("(\\d+)\\.(\\d+)\\.(\\d+)")).match(QString::fromUtf8(data));

                /*
                if((major2 < match.captured(1).toInt())
                || ((major2 == match.captured(1).toInt()) && (minor2 < match.captured(2).toInt()))
                || ((major2 == match.captured(1).toInt()) && (minor2 == match.captured(2).toInt()) && (patch2 < match.captured(3).toInt())))
                {
                    m_versionButton->setText(m_versionButton->text().append(tr(" - [ out of date - click here to updgrade ]")));
                }
                else
                {
                    m_versionButton->setText(m_versionButton->text().append(tr(" - [ latest ]")));
                }
                */
                m_versionButton->setDisabled(true);
            }
        }

        // Check ID ///////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////

        QTimer::singleShot(0, this, [this] { OpenMVPlugin::setPortPath(true); });
        QTimer::singleShot(50, this, [this] { OpenMVPlugin::emitLoadColorMap(); });
        QTimer::singleShot(100, this, [this] { OpenMVPlugin::loadFeatureFile(); });

        CONNECT_END();
    }
    else
    {
        QMessageBox::critical(Core::ICore::dialogParent(),
            tr("Connect"),
            tr("Busy... please wait..."));
    }
}

void OpenMVPlugin::restoreDefaults()
{
    // TODO: does it makes sense to run chkdsk before restoring defaults?

    QString drive;
    bool scriptStatus = false;
    int freeSize;

    logLine(QStringLiteral("Restore Defaults clicked...\n"));

    QSettings *settings = ExtensionSystem::PluginManager::settings();
    settings->beginGroup(QLatin1String("NXTCamView5"));
    drive = settings->value(QStringLiteral("CamDrive")).toString();
    settings->endGroup();
    logLine(drive);
    QStorageInfo sd_card(drive);
    if (sd_card.isValid() && sd_card.isReady()) {
        freeSize = sd_card.bytesFree();
        if ( freeSize < 1024*1024*500 ) {
            QMessageBox::critical(Core::ICore::dialogParent(),
                    tr("Not Enough Disk Space"),
                    tr("Make sure SD card is inserted in your NXTCam5."));
            return;
        }
    } else {
        QMessageBox::critical(Core::ICore::dialogParent(),
                tr("Restore"), tr("Is the NXTCam5 connected?"));
        return;
    }

    QString defaultsPath = Core::ICore::resourcePath() + QStringLiteral("/examples/NXTCamv5-defaults/");
    QDir dir(defaultsPath);
    // Check if folder exists, if not, it's a installation error.
    // If the folder exists, copy all files from this folder to NXTCam.
    if ( dir.exists() == false) {
        statusUpdate(tr("NXTCam5-defaults folder not found."));
        QMessageBox::critical(Core::ICore::dialogParent(),
                tr("Restore"), tr("NXTCam5-defaults folder not found."));
        return;
    } else {

        // defaults folder found, copy those files to NXTCam.
        QString f_orig, f_dest;

        scriptStatus = isScriptRunning();
        if ( scriptStatus ) {
            stopClicked();
        }

        QDirIterator it(defaultsPath);
            while(it.hasNext()) {
                it.next();
                f_orig = defaultsPath + QStringLiteral("/") + it.fileName();
                f_dest = drive + QStringLiteral("/") + it.fileName();
                QFileInfo qfo = it.fileInfo();
                if (  qfo.isFile() ) {
                    QFile::remove(f_dest);
                    QFile::copy(f_orig, f_dest);
                }
        }

        //statusUpdate(defaultsPath.toLatin1().data());
        statusUpdate(tr("NXTCam5 defaults restored."));
        QMessageBox::information(Core::ICore::dialogParent(),
                tr("Restore"), tr("NXTCam5 defaults restored.\nDisconnect and re-connect your NXTCam5."));

        // load the colormap again
        emit loadColorMap();

        if ( scriptStatus ) {
            startClicked();
        }
    }
}

void OpenMVPlugin::disconnectClicked(bool reset)
{
    if(m_connected)
    {
        if(!m_working)
        {
            m_working = true;

            // Stopping ///////////////////////////////////////////////////////
            {
                QEventLoop loop;

                connect(m_iodevice, &OpenMVPluginIO::closeResponse,
                        &loop, &QEventLoop::quit);

                if(reset)
                {
                    m_iodevice->sysReset();
                }
                else
                {
                    m_iodevice->scriptStop();
                }

                m_iodevice->close();

                loop.exec();
            }

            ///////////////////////////////////////////////////////////////////

            m_iodevice->getTimeout(); // clear

            m_frameSizeDumpTimer.restart();
            m_getScriptRunningTimer.restart();
            m_getTxBufferTimer.restart();

            m_timer.restart();
            m_queue.clear();
            m_connected = false;
            m_running = false;
            m_major = int();
            m_minor = int();
            m_patch = int();
            m_portName = QString();
            m_portPath = QString();
            m_errorFilterString = QString();

            m_bootloaderCommand->action()->setEnabled(true);
            m_saveCommand->action()->setEnabled(false);
            m_resetCommand->action()->setEnabled(false);
            m_connectCommand->action()->setEnabled(true);
            m_connectCommand->action()->setVisible(true);
            m_disconnectCommand->action()->setVisible(false);
            m_disconnectCommand->action()->setEnabled(false);
            m_startCommand->action()->setEnabled(false);
            m_startCommand->action()->setVisible(true);
            m_stopCommand->action()->setEnabled(false);
            m_stopCommand->action()->setVisible(false);

            m_statusLabel->setDisabled(true);
            statusUpdate(tr("NXTCam5 disconnected."));

            m_versionButton->setDisabled(true);
            m_versionButton->setText(tr("Firmware Version:"));
            m_portLabel->setDisabled(true);
            m_portLabel->setText(tr("Serial Port:"));
            m_pathButton->setDisabled(true);
            m_pathButton->setText(tr("Drive:"));
            m_fpsLabel->setDisabled(true);
            m_fpsLabel->setText(tr("FPS:"));

            m_frameBuffer->enableSaveTemplate(false);
            m_frameBuffer->enableSaveDescriptor(false);

            ///////////////////////////////////////////////////////////////////

            m_working = false;
        }
        else
        {
            QMessageBox::critical(Core::ICore::dialogParent(),
                reset ? tr("Reset") : tr("Disconnect"),
                tr("Busy... please wait..."));
        }
    }

    QTimer::singleShot(0, this, &OpenMVPlugin::disconnectDone);
}

void OpenMVPlugin::startClicked()
{
    if(!m_working)
    {
        m_working = true;

        // Stopping ///////////////////////////////////////////////////////////
        {
            bool running2 = bool();
            bool *running2Ptr = &running2;

            QMetaObject::Connection conn = connect(m_iodevice, &OpenMVPluginIO::scriptRunning,
                this, [this, running2Ptr] (bool running) {
                *running2Ptr = running;
            });

            QEventLoop loop;

            connect(m_iodevice, &OpenMVPluginIO::scriptRunning,
                    &loop, &QEventLoop::quit);

            m_iodevice->getScriptRunning();

            loop.exec();

            disconnect(conn);

            if(running2)
            {
                m_iodevice->scriptStop();
            }
        }

        ///////////////////////////////////////////////////////////////////////

        QString val;
        if (m_feature == QStringLiteral("Object")) {
            val = QStringLiteral("4");
        }
        if (m_feature == QStringLiteral("Line")) {
            val = QStringLiteral("5");
        }
        if (m_feature == QStringLiteral("Face")) {
            val = QStringLiteral("9");
        }
        if (m_feature == QStringLiteral("Eye")) {
            val = QStringLiteral("10");
        }
        if (m_feature == QStringLiteral("QRCode")) {
            val = QStringLiteral("11");
        }
        if (m_feature == QStringLiteral("Motion")) {
            val = QStringLiteral("12");
        }

        QByteArray argString("#\n");
        //argString += "begin_tracking = 1\n";
        argString += "nxtcf = " + val.toLatin1() + "\n";
        QByteArray data = argString + Core::EditorManager::currentEditor()->document()->contents();
        //m_iodevice->scriptExec(Core::EditorManager::currentEditor()->document()->contents());

        m_iodevice->scriptExec(data);

        m_timer.restart();
        m_queue.clear();

        ///////////////////////////////////////////////////////////////////////

        m_working = false;

        QTimer::singleShot(0, this, &OpenMVPlugin::workingDone);
    }
    else
    {
        QMessageBox::critical(Core::ICore::dialogParent(),
            tr("Start"),
            tr("Busy... please wait..."));
    }
}

bool OpenMVPlugin::isScriptRunning()
{
    bool running2 = bool();
    bool *running2Ptr = &running2;

    logLine(QStringLiteral("checking for script's run status..."));
    QMetaObject::Connection conn = connect(m_iodevice, &OpenMVPluginIO::scriptRunning,
        this, [this, running2Ptr] (bool running) {
        *running2Ptr = running;
    });

    QEventLoop loop;

    connect(m_iodevice, &OpenMVPluginIO::scriptRunning,
            &loop, &QEventLoop::quit);

    m_iodevice->getScriptRunning();

    loop.exec();

    disconnect(conn);

    return (running2);
}

bool OpenMVPlugin::script_checkIfRunning()
{
    return( isScriptRunning() );
}

void OpenMVPlugin::restartIfNeeded()
{
    // this function is a slot (receiving emit from colormap menu)
    // if it's already tracking, stop it and restart it.
    // if it not tracking, don't do anything.

    if ( isScriptRunning() ) {
        // stop and restart
        logLine(QStringLiteral("script is running, stopping ..."));
        stopClicked();
        //logLine(QStringLiteral("starting ..."));
        startClicked();
    } else {
        logLine(QStringLiteral("script is not running."));
    }
}

void OpenMVPlugin::stopClicked()
{
    if(!m_working)
    {
        m_working = true;

        // Stopping ///////////////////////////////////////////////////////////
        {
            bool running2 = bool();
            bool *running2Ptr = &running2;

            QMetaObject::Connection conn = connect(m_iodevice, &OpenMVPluginIO::scriptRunning,
                this, [this, running2Ptr] (bool running) {
                *running2Ptr = running;
            });

            QEventLoop loop;

            connect(m_iodevice, &OpenMVPluginIO::scriptRunning,
                    &loop, &QEventLoop::quit);

            m_iodevice->getScriptRunning();

            loop.exec();

            disconnect(conn);

            if(running2)
            {
                m_iodevice->scriptStop();
            }
        }

        ///////////////////////////////////////////////////////////////////////

        m_fpsLabel->setText(tr("FPS: 0"));

        ///////////////////////////////////////////////////////////////////////

        m_working = false;

        QTimer::singleShot(0, this, &OpenMVPlugin::workingDone);
    }
    else
    {
        QMessageBox::critical(Core::ICore::dialogParent(),
            tr("Stop"),
            tr("Busy... please wait..."));
    }
}

void OpenMVPlugin::processEvents()
{
    if((!m_working) && m_connected)
    {
        if(m_iodevice->getTimeout())
        {
            disconnectClicked(true);
        }
        else
        {
            if((!m_disableFrameBuffer->isChecked()) && (!m_iodevice->frameSizeDumpQueued()) && m_frameSizeDumpTimer.hasExpired(FRAME_SIZE_DUMP_SPACING))
            {
                m_frameSizeDumpTimer.restart();
                m_iodevice->frameSizeDump();
            }

            if((!m_iodevice->getScriptRunningQueued()) && m_getScriptRunningTimer.hasExpired(GET_SCRIPT_RUNNING_SPACING))
            {
                m_getScriptRunningTimer.restart();
                m_iodevice->getScriptRunning();

                if(m_portPath.isEmpty())
                {
                    setPortPath(true);
                }
            }

            if((!m_iodevice->getTxBufferQueued()) && m_getTxBufferTimer.hasExpired(GET_TX_BUFFER_SPACING))
            {
                m_getTxBufferTimer.restart();
                m_iodevice->getTxBuffer();
            }

            if(m_timer.hasExpired(FPS_TIMER_EXPIRATION_TIME))
            {
                m_fpsLabel->setText(tr("FPS: 0"));
            }
        }
    }
}

void OpenMVPlugin::errorFilter(const QByteArray &data)
{
    m_errorFilterString.append(Utils::SynchronousProcess::normalizeNewlines(QString::fromUtf8(data)));

    QRegularExpressionMatch match;
    int index = m_errorFilterString.indexOf(m_errorFilterRegex, 0, &match);

    if(index != -1)
    {
        QString fileName = match.captured(1);
        int lineNumber = match.captured(2).toInt();
        QString errorMessage = match.captured(3);

        Core::EditorManager::cutForwardNavigationHistory();
        Core::EditorManager::addCurrentPositionToNavigationHistory();

        TextEditor::BaseTextEditor *editor = Q_NULLPTR;

        if(fileName == QStringLiteral("<stdin>"))
        {
            editor = qobject_cast<TextEditor::BaseTextEditor *>(Core::EditorManager::currentEditor());
        }
        else if(!m_portPath.isEmpty())
        {
            editor = qobject_cast<TextEditor::BaseTextEditor *>(Core::EditorManager::openEditor(
                QDir::cleanPath(QDir::fromNativeSeparators(QString(fileName).prepend(QStringLiteral("/")).prepend(m_portPath)))));
        }

        if(editor)
        {
            Core::EditorManager::addCurrentPositionToNavigationHistory();
            editor->gotoLine(lineNumber);

            QTextCursor cursor = editor->textCursor();

            if(cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor))
            {
                editor->editorWidget()->setBlockSelection(cursor);
            }

            Core::EditorManager::activateEditor(editor);
        }

        QMessageBox *box = new QMessageBox(QMessageBox::Critical, QString(), errorMessage, QMessageBox::Ok, Core::ICore::dialogParent());
        connect(box, &QMessageBox::finished, box, &QMessageBox::deleteLater);
        QTimer::singleShot(0, box, &QMessageBox::exec);

        m_errorFilterString = m_errorFilterString.mid(index + match.capturedLength(0));
    }

    m_errorFilterString = m_errorFilterString.right(ERROR_FILTER_MAX_SIZE);
}

void OpenMVPlugin::saveScript()
{
    if(!m_working)
    {
        int answer = QMessageBox::question(Core::ICore::dialogParent(),
            tr("Save Script"),
            tr("Strip comments and convert spaces to tabs?"),
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);

        if((answer == QMessageBox::Yes) || (answer == QMessageBox::No))
        {
            QFile file(QDir::cleanPath(QDir::fromNativeSeparators(m_portPath) + QStringLiteral("/main.py")));

            if(file.open(QIODevice::WriteOnly))
            {
                QByteArray contents = Core::EditorManager::currentEditor()->document()->contents();

                if(answer == QMessageBox::Yes)
                {
                    QString bytes = QString::fromUtf8(contents);
                    bytes.remove(QRegularExpression(QStringLiteral("^\\s*?\n"), QRegularExpression::MultilineOption));
                    bytes.remove(QRegularExpression(QStringLiteral("^\\s*#.*?\n"), QRegularExpression::MultilineOption));
                    bytes.remove(QRegularExpression(QStringLiteral("\\s*#.*?$"), QRegularExpression::MultilineOption));
                    bytes.replace(QStringLiteral("    "), QStringLiteral("\t"));
                    contents = bytes.toUtf8();
                }

                if(file.write(contents) != contents.size())
                {
                    QMessageBox::critical(Core::ICore::dialogParent(),
                        tr("Save Script"),
                        tr("Error: %L1!").arg(file.errorString()));
                }
            }
            else
            {
                QMessageBox::critical(Core::ICore::dialogParent(),
                    tr("Save Script"),
                    tr("Error: %L1!").arg(file.errorString()));
            }
        }
    }
    else
    {
        QMessageBox::critical(Core::ICore::dialogParent(),
            tr("Save Script"),
            tr("Busy... please wait..."));
    }
}

void OpenMVPlugin::saveImage(const QPixmap &data)
{
    QSettings *settings = ExtensionSystem::PluginManager::settings();
    settings->beginGroup(QStringLiteral(SETTINGS_GROUP));

    QString path =
        QFileDialog::getSaveFileName(Core::ICore::dialogParent(), tr("Save Image"),
            settings->value(QStringLiteral(LAST_SAVE_IMAGE_PATH), QDir::homePath()).toString(),
            tr("Image Files (*.bmp *.jpg *.jpeg *.png *.ppm)"));

    if(!path.isEmpty())
    {
        if(data.save(path))
        {
            settings->setValue(QStringLiteral(LAST_SAVE_IMAGE_PATH), path);
        }
        else
        {
            QMessageBox::critical(Core::ICore::dialogParent(),
                tr("Save Image"),
                tr("Failed to save the image file for an unknown reason!"));
        }
    }

    settings->endGroup();
}

void OpenMVPlugin::saveTemplate(const QRect &rect)
{
    if(!m_working)
    {
        QString drivePath = QDir::cleanPath(QDir::fromNativeSeparators(m_portPath));

        QSettings *settings = ExtensionSystem::PluginManager::settings();
        settings->beginGroup(QStringLiteral(SETTINGS_GROUP));

        QString path =
            QFileDialog::getSaveFileName(Core::ICore::dialogParent(), tr("Save Template"),
                settings->value(QStringLiteral(LAST_SAVE_TEMPLATE_PATH), drivePath).toString(),
                tr("Image Files (*.bmp *.jpg *.jpeg *.pgm *.ppm)"));

        if(!path.isEmpty())
        {
            path = QDir::cleanPath(QDir::fromNativeSeparators(path));

            if((!path.startsWith(drivePath))
            || (!QDir(QFileInfo(path).path()).exists()))
            {
                QMessageBox::critical(Core::ICore::dialogParent(),
                    tr("Save Template"),
                    tr("Please select a valid path on the NXTCam5!"));
            }
            else
            {
                QByteArray sendPath = QString(path).remove(0, drivePath.size()).prepend(QLatin1Char('/')).toUtf8();

                if(sendPath.size() <= DESCRIPTOR_SAVE_PATH_MAX_LEN)
                {
                    m_iodevice->templateSave(rect.x(), rect.y(), rect.width(), rect.height(), sendPath);
                    settings->setValue(QStringLiteral(LAST_SAVE_TEMPLATE_PATH), path);
                }
                else
                {
                    QMessageBox::critical(Core::ICore::dialogParent(),
                        tr("Save Template"),
                        tr("\"%L1\" is longer than a max length of %L2 characters!").arg(QString::fromUtf8(sendPath)).arg(DESCRIPTOR_SAVE_PATH_MAX_LEN));
                }
            }
        }

        settings->endGroup();
    }
    else
    {
        QMessageBox::critical(Core::ICore::dialogParent(),
            tr("Save Template"),
            tr("Busy... please wait..."));
    }
}

void OpenMVPlugin::saveDescriptor(const QRect &rect)
{
    if(!m_working)
    {
        QString drivePath = QDir::cleanPath(QDir::fromNativeSeparators(m_portPath));

        QSettings *settings = ExtensionSystem::PluginManager::settings();
        settings->beginGroup(QStringLiteral(SETTINGS_GROUP));

        QString path =
            QFileDialog::getSaveFileName(Core::ICore::dialogParent(), tr("Save Descriptor"),
                settings->value(QStringLiteral(LAST_SAVE_DESCRIPTOR_PATH), drivePath).toString(),
                tr("Keypoints Files (*.lbp *.orb)"));

        if(!path.isEmpty())
        {
            path = QDir::cleanPath(QDir::fromNativeSeparators(path));

            if((!path.startsWith(drivePath))
            || (!QDir(QFileInfo(path).path()).exists()))
            {
                QMessageBox::critical(Core::ICore::dialogParent(),
                    tr("Save Descriptor"),
                    tr("Please select a valid path on the NXTCam5!"));
            }
            else
            {
                QByteArray sendPath = QString(path).remove(0, drivePath.size()).prepend(QLatin1Char('/')).toUtf8();

                if(sendPath.size() <= DESCRIPTOR_SAVE_PATH_MAX_LEN)
                {
                    m_iodevice->descriptorSave(rect.x(), rect.y(), rect.width(), rect.height(), sendPath);
                    settings->setValue(QStringLiteral(LAST_SAVE_DESCRIPTOR_PATH), path);
                }
                else
                {
                    QMessageBox::critical(Core::ICore::dialogParent(),
                        tr("Save Descriptor"),
                        tr("\"%L1\" is longer than a max length of %L2 characters!").arg(QString::fromUtf8(sendPath)).arg(DESCRIPTOR_SAVE_PATH_MAX_LEN));
                }
            }
        }

        settings->endGroup();
    }
    else
    {
        QMessageBox::critical(Core::ICore::dialogParent(),
            tr("Save Descriptor"),
            tr("Busy... please wait..."));
    }
}

void OpenMVPlugin::updateCam()
{
    if(!m_working)
    {
        QFile file(Core::ICore::userResourcePath() + QStringLiteral("/firmware/firmware.txt"));

        if(file.open(QIODevice::ReadOnly))
        {
            QByteArray data = file.readAll();

            if((file.error() == QFile::NoError) && (!data.isEmpty()))
            {
                file.close();

                QRegularExpressionMatch match = QRegularExpression(QStringLiteral("(\\d+)\\.(\\d+)\\.(\\d+)")).match(QString::fromUtf8(data));

                if((m_major < match.captured(1).toInt())
                || ((m_major == match.captured(1).toInt()) && (m_minor < match.captured(2).toInt()))
                || ((m_major == match.captured(1).toInt()) && (m_minor == match.captured(2).toInt()) && (m_patch < match.captured(3).toInt())))
                {
                    if(QMessageBox::warning(Core::ICore::dialogParent(),
                        tr("Firmware Update"),
                        tr("Update your NXTCam5's firmware to the latest version?"),
                        QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok)
                    == QMessageBox::Ok)
                    {
                        int answer = QMessageBox::question(Core::ICore::dialogParent(),
                            tr("Firmware Update"),
                            tr("Erase the internal file system?"),
                            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::No);

                        if((answer == QMessageBox::Yes) || (answer == QMessageBox::No))
                        {
                            disconnectClicked();

                            if(pluginSpec()->state() != ExtensionSystem::PluginSpec::Stopped)
                            {
                                connectClicked(true, QString(), answer);
                            }
                        }
                    }
                }
                else
                {
                    QMessageBox::information(Core::ICore::dialogParent(),
                        tr("Firmware Update"),
                        tr("Your NXTCam5's firmware is up to date."));

                    if(QMessageBox::question(Core::ICore::dialogParent(),
                        tr("Firmware Update"),
                        tr("Need to reset your NXTCam5's firmware to the release version?"),
                        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes)
                    == QMessageBox::Yes)
                    {
                        int answer = QMessageBox::question(Core::ICore::dialogParent(),
                            tr("Firmware Update"),
                            tr("Erase the internal file system?"),
                            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::No);

                        if((answer == QMessageBox::Yes) || (answer == QMessageBox::No))
                        {
                            disconnectClicked();

                            if(pluginSpec()->state() != ExtensionSystem::PluginSpec::Stopped)
                            {
                                connectClicked(true, QString(), answer);
                            }
                        }
                    }
                }
            }
            else if(file.error() != QFile::NoError)
            {
                QMessageBox::critical(Core::ICore::dialogParent(),
                    tr("Firmware Update"),
                    tr("Error: %L1!").arg(file.errorString()));
            }
            else
            {
                QMessageBox::critical(Core::ICore::dialogParent(),
                    tr("Firmware Update"),
                    tr("Cannot open firmware.txt!"));
            }
        }
        else
        {
            QMessageBox::critical(Core::ICore::dialogParent(),
                tr("Firmware Update"),
                tr("Error: %L1!").arg(file.errorString()));
        }
    }
    else
    {
        QMessageBox::critical(Core::ICore::dialogParent(),
            tr("Firmware Update"),
            tr("Busy... please wait..."));
    }
}

void OpenMVPlugin::setPortPath(bool silent)
{
    int z = 0;
    //char str[200];
    if(!m_working)
    {
        QStringList drives;

        foreach(const QStorageInfo &info, QStorageInfo::mountedVolumes())
        {
            if(info.isValid()
            && info.isReady()
            && (!info.isRoot())
            && (!info.isReadOnly())
            && (QString::fromUtf8(info.fileSystemType()).contains(QStringLiteral("fat"), Qt::CaseInsensitive) || QString::fromUtf8(info.fileSystemType()).contains(QStringLiteral("msdos"), Qt::CaseInsensitive))
            && ((!Utils::HostOsInfo::isMacHost()) || info.rootPath().startsWith(QStringLiteral("/volumes/"), Qt::CaseInsensitive))
            && ((!Utils::HostOsInfo::isLinuxHost()) || info.rootPath().startsWith(QStringLiteral("/media/"), Qt::CaseInsensitive) || info.rootPath().startsWith(QStringLiteral("/mnt/"), Qt::CaseInsensitive) || info.rootPath().startsWith(QStringLiteral("/run/"), Qt::CaseInsensitive)))
            {
                drives.append(info.rootPath());
            }
        }

        z = drives.size();
        /*sprintf(str, ">>00 drives count: z: %d\n", z);
        logLine(str);
        for ( int k = 0; k < z; k++) {
            sprintf (str, ">>00 drive: %s\n", drives.at(k).toLocal8Bit().constData());
            logLine(str);
        }
        */

        QSettings *settings = ExtensionSystem::PluginManager::settings();
        settings->beginGroup(QStringLiteral(SERIAL_PORT_SETTINGS_GROUP));

        if(drives.isEmpty())
        {
            if(!silent)
            {
                QMessageBox::critical(Core::ICore::dialogParent(),
                    tr("Select Drive"),
                    tr("No valid drives were found to associate with your NXTCam5!"));
            }

            m_portPath = QString();
        }
        else if(drives.size() == 1)
        {
            if(m_portPath == drives.first())
            {
                QMessageBox::information(Core::ICore::dialogParent(),
                    tr("Select Drive"),
                    tr("\"%L1\" is the only drive available so it must be your NXTCam5's drive.").arg(drives.first()));
            }
            else
            {
                m_portPath = drives.first();
                settings->setValue(m_portName, m_portPath);
            }
        }
        else
        {
            int index = drives.indexOf(settings->value(m_portName).toString());

            bool ok = silent;
            QString temp;

            if ( silent == true ){
                z = drives.size();
                for ( int k = 0; k < z; k++) {
                    QString fileName =
                        QDir::cleanPath(QDir::fromNativeSeparators(drives.at(k))) + QStringLiteral("/main.py");
                    QFileInfo fi(fileName);
                    if (fi.exists()) {
                        temp = drives.at(k);
                    }
                }
                if (temp.isEmpty()) {
                    statusUpdate(tr("main.py not found."));
                }
            } else {
                temp = QInputDialog::getItem(Core::ICore::dialogParent(),
                tr("Select Drive"), tr("Please associate a drive with your NXTCam5"),
                drives, (index != -1) ? index : 0, false, &ok,
                Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                (Utils::HostOsInfo::isMacHost() ? Qt::WindowType() : Qt::WindowCloseButtonHint));
            }
            if(ok)
            {
                m_portPath = temp;
                settings->setValue(m_portName, m_portPath);
            }
        }

        settings->endGroup();

        settings->beginGroup(QLatin1String("NXTCamView5"));
        settings->setValue(QStringLiteral("CamDrive"), m_portPath);
        settings->endGroup();

        m_pathButton->setText((!m_portPath.isEmpty()) ? tr("Drive: %L1").arg(m_portPath) : tr("Drive:"));

        Core::IEditor *editor = Core::EditorManager::currentEditor();
        m_saveCommand->action()->setEnabled((!m_portPath.isEmpty()) && (editor ? (editor->document() ? (!editor->document()->contents().isEmpty()) : false) : false));

        m_frameBuffer->enableSaveTemplate(!m_portPath.isEmpty());
        m_frameBuffer->enableSaveDescriptor(!m_portPath.isEmpty());
    }
    else
    {
        QMessageBox::critical(Core::ICore::dialogParent(),
            tr("Select Drive"),
            tr("Busy... please wait..."));
    }
}

QMap<QString, QAction *> OpenMVPlugin::aboutToShowExamplesRecursive(const QString &path, QMenu *parent)
{
    QMap<QString, QAction *> actions;
    QDirIterator it(path, QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

    while(it.hasNext())
    {
        QString filePath = it.next();

        if(it.fileInfo().isDir())
        {
            QMenu *menu = new QMenu(it.fileName(), parent);
            QMap<QString, QAction *> menuActions = aboutToShowExamplesRecursive(filePath, menu);
            menu->addActions(menuActions.values());
            menu->setDisabled(menuActions.values().isEmpty());
            actions.insertMulti(it.fileName(), menu->menuAction());
        }
        else
        {
            QAction *action = new QAction(it.fileName(), parent);
            connect(action, &QAction::triggered, this, [this, filePath]
            {
                QFile file(filePath);

                if(file.open(QIODevice::ReadOnly))
                {
                    QByteArray data = file.readAll();

                    if((file.error() == QFile::NoError) && (!data.isEmpty()))
                    {
                        Core::EditorManager::cutForwardNavigationHistory();
                        Core::EditorManager::addCurrentPositionToNavigationHistory();

                        QString titlePattern = QFileInfo(filePath).baseName().simplified() + QStringLiteral("_$.") + QFileInfo(filePath).completeSuffix();
                        TextEditor::BaseTextEditor *editor = qobject_cast<TextEditor::BaseTextEditor *>(Core::EditorManager::openEditorWithContents(Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &titlePattern, data));

                        if(editor)
                        {
                            editor->editorWidget()->configureGenericHighlighter();
                            Core::EditorManager::activateEditor(editor);
                        }
                        else
                        {
                            QMessageBox::critical(Core::ICore::dialogParent(),
                                tr("Open Example"),
                                tr("Cannot open the example file \"%L1\"!").arg(filePath));
                        }
                    }
                    else if(file.error() != QFile::NoError)
                    {
                        QMessageBox::critical(Core::ICore::dialogParent(),
                            tr("Open Example"),
                            tr("Error: %L1!").arg(file.errorString()));
                    }
                    else
                    {
                        QMessageBox::critical(Core::ICore::dialogParent(),
                            tr("Open Example"),
                            tr("Cannot open the example file \"%L1\"!").arg(filePath));
                    }
                }
                else
                {
                    QMessageBox::critical(Core::ICore::dialogParent(),
                        tr("Open Example"),
                        tr("Error: %L1!").arg(file.errorString()));
                }
            });

            actions.insertMulti(it.fileName(), action);
        }
    }

    return actions;
}

const int connectToSerialPortIndex = 0;
const int connectToUDPPortIndex = 1;
const int connectToTCPPortIndex = 2;

void OpenMVPlugin::openTerminalAboutToShow()
{
    m_openTerminalMenu->menu()->clear();
    connect(m_openTerminalMenu->menu()->addAction(tr("New Terminal")), &QAction::triggered, this, [this] {
        QSettings *settings = ExtensionSystem::PluginManager::settings();
        settings->beginGroup(QStringLiteral(SETTINGS_GROUP));

        QStringList optionList = QStringList()
            << tr("Connect to serial port")
            << tr("Connect to UDP port")
            << tr("Connect to TCP port");

        int optionListIndex = optionList.indexOf(settings->value(QStringLiteral(LAST_OPEN_TERMINAL_SELECT)).toString());

        bool optionNameOk;
        QString optionName = QInputDialog::getItem(Core::ICore::dialogParent(),
            tr("New Terminal"), tr("Please select an option"),
            optionList, (optionListIndex != -1) ? optionListIndex : 0, false, &optionNameOk,
            Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
            (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));

        if(optionNameOk)
        {
            switch(optionList.indexOf(optionName))
            {
                case connectToSerialPortIndex:
                {
                    QStringList stringList;

                    foreach(QSerialPortInfo port, QSerialPortInfo::availablePorts())
                    {
                        stringList.append(port.portName());
                    }

                    if(Utils::HostOsInfo::isMacHost())
                    {
                        stringList = stringList.filter(QStringLiteral("cu"), Qt::CaseInsensitive);
                    }

                    int index = stringList.indexOf(settings->value(QStringLiteral(LAST_OPEN_TERMINAL_SERIAL_PORT)).toString());

                    bool portNameValueOk;
                    QString portNameValue = QInputDialog::getItem(Core::ICore::dialogParent(),
                        tr("New Terminal"), tr("Please select a serial port"),
                        stringList, (index != -1) ? index : 0, false, &portNameValueOk,
                        Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                        (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));

                    if(portNameValueOk)
                    {
                        bool baudRateOk;
                        QString baudRate = QInputDialog::getText(Core::ICore::dialogParent(),
                            tr("New Terminal"), tr("Please enter a baud rate"),
                            QLineEdit::Normal, settings->value(QStringLiteral(LAST_OPEN_TERMINAL_SERIAL_PORT_BAUD_RATE), QStringLiteral("115200")).toString(), &baudRateOk,
                            Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                            (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));

                        if(baudRateOk && (!baudRate.isEmpty()))
                        {
                            bool buadRateValueOk;
                            int baudRateValue = baudRate.toInt(&buadRateValueOk);

                            if(buadRateValueOk)
                            {
                                settings->setValue(QStringLiteral(LAST_OPEN_TERMINAL_SELECT), optionName);
                                settings->setValue(QStringLiteral(LAST_OPEN_TERMINAL_SERIAL_PORT), portNameValue);
                                settings->setValue(QStringLiteral(LAST_OPEN_TERMINAL_SERIAL_PORT_BAUD_RATE), baudRateValue);

                                openTerminalMenuData_t data;
                                data.displayName = tr("Serial Port - %L1 - %L2 BPS").arg(portNameValue).arg(baudRateValue);
                                data.optionIndex = connectToSerialPortIndex;
                                data.commandStr = portNameValue;
                                data.commandVal = baudRateValue;

                                if(!openTerminalMenuDataContains(data.displayName))
                                {
                                    m_openTerminalMenuData.append(data);

                                    if(m_openTerminalMenuData.size() > 10)
                                    {
                                        m_openTerminalMenuData.removeFirst();
                                    }
                                }

                                OpenMVTerminal *terminal = new OpenMVTerminal(data.displayName, ExtensionSystem::PluginManager::settings(), Core::Context(Core::Id::fromString(data.displayName)));
                                OpenMVTerminalSerialPort *terminalDevice = new OpenMVTerminalSerialPort(terminal);

                                connect(terminal, &OpenMVTerminal::writeBytes,
                                        terminalDevice, &OpenMVTerminalPort::writeBytes);

                                connect(terminalDevice, &OpenMVTerminalPort::readBytes,
                                        terminal, &OpenMVTerminal::readBytes);

                                QString errorMessage2 = QString();
                                QString *errorMessage2Ptr = &errorMessage2;

                                QMetaObject::Connection conn = connect(terminalDevice, &OpenMVTerminalPort::openResult,
                                    this, [this, errorMessage2Ptr] (const QString &errorMessage) {
                                    *errorMessage2Ptr = errorMessage;
                                });

                                // QProgressDialog scoping...
                                {
                                    QProgressDialog dialog(tr("Connecting... (30 second timeout)"), tr("Cancel"), 0, 0, Core::ICore::dialogParent(),
                                        Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint |
                                        (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowType(0)));
                                    dialog.setWindowModality(Qt::ApplicationModal);
                                    dialog.setAttribute(Qt::WA_ShowWithoutActivating);
                                    dialog.setCancelButton(Q_NULLPTR);
                                    QTimer::singleShot(1000, &dialog, &QWidget::show);

                                    QEventLoop loop;

                                    connect(terminalDevice, &OpenMVTerminalPort::openResult,
                                            &loop, &QEventLoop::quit);

                                    terminalDevice->open(data.commandStr, data.commandVal);

                                    loop.exec();
                                    dialog.close();
                                }

                                disconnect(conn);

                                if(!errorMessage2.isEmpty())
                                {
                                    QMessageBox::critical(Core::ICore::dialogParent(),
                                        tr("New Terminal"),
                                        tr("Error: %L1!").arg(errorMessage2));

                                    if(Utils::HostOsInfo::isLinuxHost() && errorMessage2.contains(QStringLiteral("Permission Denied"), Qt::CaseInsensitive))
                                    {
                                        QMessageBox::information(Core::ICore::dialogParent(),
                                            tr("New Terminal"),
                                            tr("Try doing:\n\nsudo adduser %L1 dialout\n\n...in a terminal and then restart your computer.").arg(Utils::Environment::systemEnvironment().userName()));
                                    }

                                    delete terminalDevice;
                                    delete terminal;
                                }
                                else
                                {
                                    terminal->show();
                                    connect(Core::ICore::instance(), &Core::ICore::coreAboutToClose,
                                            terminal, &OpenMVTerminal::close);
                                }
                            }
                            else
                            {
                                QMessageBox::critical(Core::ICore::dialogParent(),
                                    tr("New Terminal"),
                                    tr("Invalid string: \"%L1\"!").arg(baudRate));
                            }
                        }
                    }

                    break;
                }
                case connectToUDPPortIndex:
                {
                    bool hostNameOk;
                    QString hostName = QInputDialog::getText(Core::ICore::dialogParent(),
                        tr("New Terminal"), tr("Please enter a IP address (or domain name) and port (e.g. xxx.xxx.xxx.xxx:xxxx)"),
                        QLineEdit::Normal, settings->value(QStringLiteral(LAST_OPEN_TERMINAL_UDP_PORT), QStringLiteral("xxx.xxx.xxx.xxx:xxxx")).toString(), &hostNameOk,
                        Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                        (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));

                    if(hostNameOk && (!hostName.isEmpty()))
                    {
                        QStringList hostNameList = hostName.split(QLatin1Char(':'), QString::SkipEmptyParts);

                        if(hostNameList.size() == 2)
                        {
                            bool portValueOk;
                            QString hostNameValue = hostNameList.at(0);
                            int portValue = hostNameList.at(1).toInt(&portValueOk);

                            if(portValueOk)
                            {
                                settings->setValue(QStringLiteral(LAST_OPEN_TERMINAL_SELECT), optionName);
                                settings->setValue(QStringLiteral(LAST_OPEN_TERMINAL_UDP_PORT), hostName);

                                openTerminalMenuData_t data;
                                data.displayName = tr("UDP Port - %L1").arg(hostName);
                                data.optionIndex = connectToUDPPortIndex;
                                data.commandStr = hostNameValue;
                                data.commandVal = portValue;

                                if(!openTerminalMenuDataContains(data.displayName))
                                {
                                    m_openTerminalMenuData.append(data);

                                    if(m_openTerminalMenuData.size() > 10)
                                    {
                                        m_openTerminalMenuData.removeFirst();
                                    }
                                }

                                OpenMVTerminal *terminal = new OpenMVTerminal(data.displayName, ExtensionSystem::PluginManager::settings(), Core::Context(Core::Id::fromString(data.displayName)));
                                OpenMVTerminalUDPPort *terminalDevice = new OpenMVTerminalUDPPort(terminal);

                                connect(terminal, &OpenMVTerminal::writeBytes,
                                        terminalDevice, &OpenMVTerminalPort::writeBytes);

                                connect(terminalDevice, &OpenMVTerminalPort::readBytes,
                                        terminal, &OpenMVTerminal::readBytes);

                                QString errorMessage2 = QString();
                                QString *errorMessage2Ptr = &errorMessage2;

                                QMetaObject::Connection conn = connect(terminalDevice, &OpenMVTerminalPort::openResult,
                                    this, [this, errorMessage2Ptr] (const QString &errorMessage) {
                                    *errorMessage2Ptr = errorMessage;
                                });

                                // QProgressDialog scoping...
                                {
                                    QProgressDialog dialog(tr("Connecting... (30 second timeout)"), tr("Cancel"), 0, 0, Core::ICore::dialogParent(),
                                        Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint |
                                        (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowType(0)));
                                    dialog.setWindowModality(Qt::ApplicationModal);
                                    dialog.setAttribute(Qt::WA_ShowWithoutActivating);
                                    dialog.setCancelButton(Q_NULLPTR);
                                    QTimer::singleShot(1000, &dialog, &QWidget::show);

                                    QEventLoop loop;

                                    connect(terminalDevice, &OpenMVTerminalPort::openResult,
                                            &loop, &QEventLoop::quit);

                                    terminalDevice->open(data.commandStr, data.commandVal);

                                    loop.exec();
                                    dialog.close();
                                }

                                disconnect(conn);

                                if(!errorMessage2.isEmpty())
                                {
                                    QMessageBox::critical(Core::ICore::dialogParent(),
                                        tr("New Terminal"),
                                        tr("Error: %L1!").arg(errorMessage2));

                                    delete terminalDevice;
                                    delete terminal;
                                }
                                else
                                {
                                    terminal->show();
                                    connect(Core::ICore::instance(), &Core::ICore::coreAboutToClose,
                                            terminal, &OpenMVTerminal::close);
                                }
                            }
                            else
                            {
                                QMessageBox::critical(Core::ICore::dialogParent(),
                                    tr("New Terminal"),
                                    tr("Invalid string: \"%L1\"!").arg(hostName));
                            }
                        }
                        else
                        {
                            QMessageBox::critical(Core::ICore::dialogParent(),
                                tr("New Terminal"),
                                tr("Invalid string: \"%L1\"!").arg(hostName));
                        }
                    }

                    break;
                }
                case connectToTCPPortIndex:
                {
                    bool hostNameOk;
                    QString hostName = QInputDialog::getText(Core::ICore::dialogParent(),
                        tr("New Terminal"), tr("Please enter a IP address (or domain name) and port (e.g. xxx.xxx.xxx.xxx:xxxx)"),
                        QLineEdit::Normal, settings->value(QStringLiteral(LAST_OPEN_TERMINAL_TCP_PORT), QStringLiteral("xxx.xxx.xxx.xxx:xxxx")).toString(), &hostNameOk,
                        Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                        (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));

                    if(hostNameOk && (!hostName.isEmpty()))
                    {
                        QStringList hostNameList = hostName.split(QLatin1Char(':'), QString::SkipEmptyParts);

                        if(hostNameList.size() == 2)
                        {
                            bool portValueOk;
                            QString hostNameValue = hostNameList.at(0);
                            int portValue = hostNameList.at(1).toInt(&portValueOk);

                            if(portValueOk)
                            {
                                settings->setValue(QStringLiteral(LAST_OPEN_TERMINAL_SELECT), optionName);
                                settings->setValue(QStringLiteral(LAST_OPEN_TERMINAL_TCP_PORT), hostName);

                                openTerminalMenuData_t data;
                                data.displayName = tr("TCP Port - %L1").arg(hostName);
                                data.optionIndex = connectToTCPPortIndex;
                                data.commandStr = hostNameValue;
                                data.commandVal = portValue;

                                if(!openTerminalMenuDataContains(data.displayName))
                                {
                                    m_openTerminalMenuData.append(data);

                                    if(m_openTerminalMenuData.size() > 10)
                                    {
                                        m_openTerminalMenuData.removeFirst();
                                    }
                                }

                                OpenMVTerminal *terminal = new OpenMVTerminal(data.displayName, ExtensionSystem::PluginManager::settings(), Core::Context(Core::Id::fromString(data.displayName)));
                                OpenMVTerminalTCPPort *terminalDevice = new OpenMVTerminalTCPPort(terminal);

                                connect(terminal, &OpenMVTerminal::writeBytes,
                                        terminalDevice, &OpenMVTerminalPort::writeBytes);

                                connect(terminalDevice, &OpenMVTerminalPort::readBytes,
                                        terminal, &OpenMVTerminal::readBytes);

                                QString errorMessage2 = QString();
                                QString *errorMessage2Ptr = &errorMessage2;

                                QMetaObject::Connection conn = connect(terminalDevice, &OpenMVTerminalPort::openResult,
                                    this, [this, errorMessage2Ptr] (const QString &errorMessage) {
                                    *errorMessage2Ptr = errorMessage;
                                });

                                // QProgressDialog scoping...
                                {
                                    QProgressDialog dialog(tr("Connecting... (30 second timeout)"), tr("Cancel"), 0, 0, Core::ICore::dialogParent(),
                                        Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint |
                                        (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowType(0)));
                                    dialog.setWindowModality(Qt::ApplicationModal);
                                    dialog.setAttribute(Qt::WA_ShowWithoutActivating);
                                    dialog.setCancelButton(Q_NULLPTR);
                                    QTimer::singleShot(1000, &dialog, &QWidget::show);

                                    QEventLoop loop;

                                    connect(terminalDevice, &OpenMVTerminalPort::openResult,
                                            &loop, &QEventLoop::quit);

                                    terminalDevice->open(data.commandStr, data.commandVal);

                                    loop.exec();
                                    dialog.close();
                                }

                                disconnect(conn);

                                if(!errorMessage2.isEmpty())
                                {
                                    QMessageBox::critical(Core::ICore::dialogParent(),
                                        tr("New Terminal"),
                                        tr("Error: %L1!").arg(errorMessage2));

                                    delete terminalDevice;
                                    delete terminal;
                                }
                                else
                                {
                                    terminal->show();
                                    connect(Core::ICore::instance(), &Core::ICore::coreAboutToClose,
                                            terminal, &OpenMVTerminal::close);
                                }
                            }
                            else
                            {
                                QMessageBox::critical(Core::ICore::dialogParent(),
                                    tr("New Terminal"),
                                    tr("Invalid string: \"%L1\"!").arg(hostName));
                            }
                        }
                        else
                        {
                            QMessageBox::critical(Core::ICore::dialogParent(),
                                tr("New Terminal"),
                                tr("Invalid string: \"%L1\"!").arg(hostName));
                        }
                    }

                    break;
                }
            }
        }

        settings->endGroup();
    });

    m_openTerminalMenu->menu()->addSeparator();

    for(int i = 0, j = m_openTerminalMenuData.size(); i < j; i++)
    {
        openTerminalMenuData_t data = m_openTerminalMenuData.at(i);
        connect(m_openTerminalMenu->menu()->addAction(data.displayName), &QAction::triggered, this, [this, data] {
            OpenMVTerminal *terminal = new OpenMVTerminal(data.displayName, ExtensionSystem::PluginManager::settings(), Core::Context(Core::Id::fromString(data.displayName)));
            OpenMVTerminalPort *terminalDevice;

            switch(data.optionIndex)
            {
                case connectToSerialPortIndex:
                {
                    terminalDevice = new OpenMVTerminalSerialPort(terminal);
                    break;
                }
                case connectToUDPPortIndex:
                {
                    terminalDevice = new OpenMVTerminalUDPPort(terminal);
                    break;
                }
                case connectToTCPPortIndex:
                {
                    terminalDevice = new OpenMVTerminalTCPPort(terminal);
                    break;
                }
                default:
                {
                    delete terminal;

                    QMessageBox::critical(Core::ICore::dialogParent(),
                        tr("Open Terminal"),
                        tr("Error: Option Index!"));

                    return;
                }
            }

            connect(terminal, &OpenMVTerminal::writeBytes,
                    terminalDevice, &OpenMVTerminalPort::writeBytes);

            connect(terminalDevice, &OpenMVTerminalPort::readBytes,
                    terminal, &OpenMVTerminal::readBytes);

            QString errorMessage2 = QString();
            QString *errorMessage2Ptr = &errorMessage2;

            QMetaObject::Connection conn = connect(terminalDevice, &OpenMVTerminalPort::openResult,
                this, [this, errorMessage2Ptr] (const QString &errorMessage) {
                *errorMessage2Ptr = errorMessage;
            });

            // QProgressDialog scoping...
            {
                QProgressDialog dialog(tr("Connecting... (30 second timeout)"), tr("Cancel"), 0, 0, Core::ICore::dialogParent(),
                    Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint |
                    (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowType(0)));
                dialog.setWindowModality(Qt::ApplicationModal);
                dialog.setAttribute(Qt::WA_ShowWithoutActivating);
                dialog.setCancelButton(Q_NULLPTR);
                QTimer::singleShot(1000, &dialog, &QWidget::show);

                QEventLoop loop;

                connect(terminalDevice, &OpenMVTerminalPort::openResult,
                        &loop, &QEventLoop::quit);

                terminalDevice->open(data.commandStr, data.commandVal);

                loop.exec();
                dialog.close();
            }

            disconnect(conn);

            if(!errorMessage2.isEmpty())
            {
                QMessageBox::critical(Core::ICore::dialogParent(),
                    tr("Open Terminal"),
                    tr("Error: %L1!").arg(errorMessage2));

                delete terminalDevice;
                delete terminal;
            }
            else
            {
                terminal->show();
                connect(Core::ICore::instance(), &Core::ICore::coreAboutToClose,
                        terminal, &OpenMVTerminal::close);
            }
        });
    }

    if(m_openTerminalMenuData.size())
    {
        m_openTerminalMenu->menu()->addSeparator();
        connect(m_openTerminalMenu->menu()->addAction(tr("Clear Menu")), &QAction::triggered, this, [this] {
            m_openTerminalMenuData.clear();
        });
    }
}

void OpenMVPlugin::openThresholdEditor()
{
    QMessageBox box(QMessageBox::Question, tr("Threshold Editor"), tr("Source image location?"), QMessageBox::Cancel, Core::ICore::dialogParent(),
        Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
        (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));
    QPushButton *button0 = box.addButton(tr(" Frame Buffer "), QMessageBox::AcceptRole);
    QPushButton *button1 = box.addButton(tr(" Image File "), QMessageBox::AcceptRole);
    box.setDefaultButton(button0);
    box.setEscapeButton(QMessageBox::Cancel);
    box.exec();

    QString drivePath = QDir::cleanPath(QDir::fromNativeSeparators(m_portPath));

    QSettings *settings = ExtensionSystem::PluginManager::settings();
    settings->beginGroup(QStringLiteral(SETTINGS_GROUP));

    if(box.clickedButton() == button0)
    {
        if(m_frameBuffer->pixmapValid())
        {
            ThresholdEditor editor(m_frameBuffer->pixmap(), settings->value(QStringLiteral(LAST_THRESHOLD_EDITOR_STATE)).toByteArray(), Core::ICore::dialogParent(),
                Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));

            editor.exec();
            settings->setValue(QStringLiteral(LAST_THRESHOLD_EDITOR_STATE), editor.saveGeometry());
        }
        else
        {
            QMessageBox::critical(Core::ICore::dialogParent(),
                tr("Threshold Editor"),
                tr("No image loaded!"));
        }
    }
    else if(box.clickedButton() == button1)
    {
        QString path =
            QFileDialog::getOpenFileName(Core::ICore::dialogParent(), tr("Image File"),
                settings->value(QStringLiteral(LAST_THRESHOLD_EDITOR_PATH), drivePath.isEmpty() ? QDir::homePath() : drivePath).toString(),
                tr("Image Files (*.bmp *.jpg *.jpeg *.png *.ppm)"));

        if(!path.isEmpty())
        {
            QPixmap pixmap = QPixmap(path);

            ThresholdEditor editor(pixmap, settings->value(QStringLiteral(LAST_THRESHOLD_EDITOR_STATE)).toByteArray(), Core::ICore::dialogParent(),
                Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));

            editor.exec();
            settings->setValue(QStringLiteral(LAST_THRESHOLD_EDITOR_STATE), editor.saveGeometry());
            settings->setValue(QStringLiteral(LAST_THRESHOLD_EDITOR_PATH), path);
        }
    }

    settings->endGroup();
}

void OpenMVPlugin::openKeypointsEditor()
{
    QMessageBox box(QMessageBox::Question, tr("Keypoints Editor"), tr("What would you like to do?"), QMessageBox::Cancel, Core::ICore::dialogParent(),
        Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
        (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));
    QPushButton *button0 = box.addButton(tr(" Edit File "), QMessageBox::AcceptRole);
    QPushButton *button1 = box.addButton(tr(" Merge Files "), QMessageBox::AcceptRole);
    box.setDefaultButton(button0);
    box.setEscapeButton(QMessageBox::Cancel);
    box.exec();

    QString drivePath = QDir::cleanPath(QDir::fromNativeSeparators(m_portPath));

    QSettings *settings = ExtensionSystem::PluginManager::settings();
    settings->beginGroup(QStringLiteral(SETTINGS_GROUP));

    if(box.clickedButton() == button0)
    {
        QString path =
            QFileDialog::getOpenFileName(Core::ICore::dialogParent(), tr("Edit Keypoints"),
                settings->value(QStringLiteral(LAST_EDIT_KEYPOINTS_PATH), drivePath.isEmpty() ? QDir::homePath() : drivePath).toString(),
                tr("Keypoints Files (*.lbp *.orb)"));

        if(!path.isEmpty())
        {
            QScopedPointer<Keypoints> ks(Keypoints::newKeypoints(path));

            if(ks)
            {
                QString name = QFileInfo(path).completeBaseName();
                QStringList list = QDir(QFileInfo(path).path()).entryList(QStringList()
                    << (name + QStringLiteral(".bmp"))
                    << (name + QStringLiteral(".jpg"))
                    << (name + QStringLiteral(".jpeg"))
                    << (name + QStringLiteral(".ppm"))
                    << (name + QStringLiteral(".pgm"))
                    << (name + QStringLiteral(".pbm")),
                    QDir::Files,
                    QDir::Name);

                if(!list.isEmpty())
                {
                    QString pixmapPath = QFileInfo(path).path() + QDir::separator() + list.first();
                    QPixmap pixmap = QPixmap(pixmapPath);

                    KeypointsEditor editor(ks.data(), pixmap, settings->value(QStringLiteral(LAST_EDIT_KEYPOINTS_STATE)).toByteArray(), Core::ICore::dialogParent(),
                        Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                        (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));

                    if(editor.exec() == QDialog::Accepted)
                    {
                        if(QFile::exists(path + QStringLiteral(".bak")))
                        {
                            QFile::remove(path + QStringLiteral(".bak"));
                        }

                        if(QFile::exists(pixmapPath + QStringLiteral(".bak")))
                        {
                            QFile::remove(pixmapPath + QStringLiteral(".bak"));
                        }

                        if(QFile::copy(path, path + QStringLiteral(".bak"))
                        && QFile::copy(pixmapPath, pixmapPath + QStringLiteral(".bak"))
                        && ks->saveKeypoints(path))
                        {
                            settings->setValue(QStringLiteral(LAST_EDIT_KEYPOINTS_STATE), editor.saveGeometry());
                            settings->setValue(QStringLiteral(LAST_EDIT_KEYPOINTS_PATH), path);
                        }
                        else
                        {
                            QMessageBox::critical(Core::ICore::dialogParent(),
                                tr("Save Edited Keypoints"),
                                tr("Failed to save the edited keypoints for an unknown reason!"));
                        }
                    }
                    else
                    {
                        settings->setValue(QStringLiteral(LAST_EDIT_KEYPOINTS_STATE), editor.saveGeometry());
                        settings->setValue(QStringLiteral(LAST_EDIT_KEYPOINTS_PATH), path);
                    }
                }
                else
                {
                    QMessageBox::critical(Core::ICore::dialogParent(),
                        tr("Edit Keypoints"),
                        tr("Failed to find the keypoints image file!"));
                }
            }
            else
            {
                QMessageBox::critical(Core::ICore::dialogParent(),
                    tr("Edit Keypoints"),
                    tr("Failed to load the keypoints file for an unknown reason!"));
            }
        }
    }
    else if(box.clickedButton() == button1)
    {
        QStringList paths =
            QFileDialog::getOpenFileNames(Core::ICore::dialogParent(), tr("Merge Keypoints"),
                settings->value(QStringLiteral(LAST_MERGE_KEYPOINTS_OPEN_PATH), drivePath.isEmpty() ? QDir::homePath() : drivePath).toString(),
                tr("Keypoints Files (*.lbp *.orb)"));

        if(!paths.isEmpty())
        {
            QString first = paths.takeFirst();
            QScopedPointer<Keypoints> ks(Keypoints::newKeypoints(first));

            if(ks)
            {
                foreach(const QString &path, paths)
                {
                    ks->mergeKeypoints(path);
                }

                QString path =
                    QFileDialog::getSaveFileName(Core::ICore::dialogParent(), tr("Save Merged Keypoints"),
                        settings->value(QStringLiteral(LAST_MERGE_KEYPOINTS_SAVE_PATH), drivePath).toString(),
                        tr("Keypoints Files (*.lbp *.orb)"));

                if(!path.isEmpty())
                {
                    if(ks->saveKeypoints(path))
                    {
                        settings->setValue(QStringLiteral(LAST_MERGE_KEYPOINTS_OPEN_PATH), first);
                        settings->setValue(QStringLiteral(LAST_MERGE_KEYPOINTS_SAVE_PATH), path);
                    }
                    else
                    {
                        QMessageBox::critical(Core::ICore::dialogParent(),
                            tr("Save Merged Keypoints"),
                            tr("Failed to save the merged keypoints for an unknown reason!"));
                    }
                }
            }
            else
            {
                QMessageBox::critical(Core::ICore::dialogParent(),
                    tr("Merge Keypoints"),
                    tr("Failed to load the first keypoints file for an unknown reason!"));
            }
        }
    }

    settings->endGroup();
}

void OpenMVPlugin::chooseFeature(QString feature)
{
    logLine(QStringLiteral("into choose Feature.."));
    m_feature = feature;
    showFeatureStatus();
    restartIfNeeded();
}

void OpenMVPlugin::openAprilTagGenerator(apriltag_family_t *family)
{
    QDialog *dialog = new QDialog(Core::ICore::dialogParent(),
        Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
        (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));
    dialog->setWindowTitle(tr("AprilTag Generator"));
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    layout->addWidget(new QLabel(tr("What tag images from the %L1 tag family do you want to generate?").arg(QString::fromUtf8(family->name).toUpper())));

    QSettings *settings = ExtensionSystem::PluginManager::settings();
    settings->beginGroup(QStringLiteral(SETTINGS_GROUP));

    QWidget *temp = new QWidget();
    QHBoxLayout *tempLayout = new QHBoxLayout(temp);
    tempLayout->setMargin(0);

    QWidget *minTemp = new QWidget();
    QFormLayout *minTempLayout = new QFormLayout(minTemp);
    minTempLayout->setMargin(0);
    QSpinBox *minRange = new QSpinBox();
    minRange->setMinimum(0);
    minRange->setMaximum(family->ncodes - 1);
    minRange->setValue(settings->value(QStringLiteral(LAST_APRILTAG_RANGE_MIN), 0).toInt());
    minRange->setAccelerated(true);
    minTempLayout->addRow(tr("Min (%1)").arg(0), minRange); // don't use %L1 here
    tempLayout->addWidget(minTemp);

    QWidget *maxTemp = new QWidget();
    QFormLayout *maxTempLayout = new QFormLayout(maxTemp);
    maxTempLayout->setMargin(0);
    QSpinBox *maxRange = new QSpinBox();
    maxRange->setMinimum(0);
    maxRange->setMaximum(family->ncodes - 1);
    maxRange->setValue(settings->value(QStringLiteral(LAST_APRILTAG_RANGE_MAX), family->ncodes - 1).toInt());
    maxRange->setAccelerated(true);
    maxTempLayout->addRow(tr("Max (%1)").arg(family->ncodes - 1), maxRange); // don't use %L1 here
    tempLayout->addWidget(maxTemp);

    layout->addWidget(temp);

    QCheckBox *checkBox = new QCheckBox(tr("Inlcude tag family and ID number in the image"));
    checkBox->setCheckable(true);
    checkBox->setChecked(settings->value(QStringLiteral(LAST_APRILTAG_INCLUDE), true).toBool());
    layout->addWidget(checkBox);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(box, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    layout->addWidget(box);

    if(dialog->exec() == QDialog::Accepted)
    {
        int min = qMin(minRange->value(), maxRange->value());
        int max = qMax(minRange->value(), maxRange->value());
        int number = max - min + 1;
        bool include = checkBox->isChecked();

        QString path =
            QFileDialog::getExistingDirectory(Core::ICore::dialogParent(), tr("AprilTag Generator - Where do you want to save %n tag image(s) to?", "", number),
                settings->value(QStringLiteral(LAST_APRILTAG_PATH), QDir::homePath()).toString());

        if(!path.isEmpty())
        {
            QProgressDialog progress(tr("Generating images..."), tr("Cancel"), 0, number - 1, Core::ICore::dialogParent(),
                Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint |
                (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowType(0)));
            progress.setWindowModality(Qt::ApplicationModal);

            for(int i = 0; i < number; i++)
            {
                progress.setValue(i);

                QImage image(family->d + 4, family->d + 4, QImage::Format_Grayscale8);

                for(uint32_t y = 0; y < (family->d + 4); y++)
                {
                    for(uint32_t x = 0; x < (family->d + 4); x++)
                    {
                        if((x == 0) || (x == (family->d + 3)) || (y == 0) || (y == (family->d + 3)))
                        {
                            image.setPixel(x, y, -1);
                        }
                        else if((x == 1) || (x == (family->d + 2)) || (y == 1) || (y == (family->d + 2)))
                        {
                            image.setPixel(x, y, family->black_border ? 0 : -1);
                        }
                        else
                        {
                            image.setPixel(x, y, ((family->codes[min + i] >> (((family->d + 1 - y) * family->d) + (family->d + 1 - x))) & 1) ? -1 : 0);
                        }
                    }
                }

                QPixmap pixmap(816, include ? 1056 : 816); // 8" x 11" (96 DPI)
                pixmap.fill();

                QPainter painter;

                if(!painter.begin(&pixmap))
                {
                    QMessageBox::critical(Core::ICore::dialogParent(),
                        tr("AprilTag Generator"),
                        tr("Painting - begin failed!"));

                    progress.cancel();
                    break;
                }

                QFont font = painter.font();
                font.setPointSize(40);
                painter.setFont(font);

                painter.drawImage(8, 8, image.scaled(800, 800, Qt::KeepAspectRatio, Qt::FastTransformation));

                if(include)
                {
                    painter.drawText(0 + 8, 8 + 800 + 8 + 80, 800, 80, Qt::AlignHCenter | Qt::AlignVCenter, QString::fromUtf8(family->name).toUpper() + QString(QStringLiteral(" - %1")).arg(min + i));
                }

                if(!painter.end())
                {
                    QMessageBox::critical(Core::ICore::dialogParent(),
                        tr("AprilTag Generator"),
                        tr("Painting - end failed!"));

                    progress.cancel();
                    break;
                }

                if(!pixmap.save(path + QDir::separator() + QString::fromUtf8(family->name).toLower() + QString(QStringLiteral("_%1.png")).arg(min + i)))
                {
                    QMessageBox::critical(Core::ICore::dialogParent(),
                        tr("AprilTag Generator"),
                        tr("Failed to save the image file for an unknown reason!"));

                    progress.cancel();
                }

                if(progress.wasCanceled())
                {
                    break;
                }
            }

            if(!progress.wasCanceled())
            {
                settings->setValue(QStringLiteral(LAST_APRILTAG_RANGE_MIN), min);
                settings->setValue(QStringLiteral(LAST_APRILTAG_RANGE_MAX), max);
                settings->setValue(QStringLiteral(LAST_APRILTAG_INCLUDE), include);
                settings->setValue(QStringLiteral(LAST_APRILTAG_PATH), path);

                QMessageBox::information(Core::ICore::dialogParent(),
                    tr("AprilTag Generator"),
                    tr("Generation complete!"));
            }
        }
    }

    settings->endGroup();
    delete dialog;
    free(family->name);
    free(family->codes);
    free(family);
}

void OpenMVPlugin::openQRCodeGenerator()
{
    QUrl url = QUrl(QStringLiteral("http://www.google.com/#q=qr+code+generator"));

    if(!QDesktopServices::openUrl(url))
    {
        QMessageBox::critical(Core::ICore::dialogParent(),
                              QString(),
                              tr("Failed to open: \"%L1\"").arg(url.toString()));
    }
}

void OpenMVPlugin::statusUpdate(QString msg)
{
    m_statusLabel->setText(msg);
    showFeatureStatus();
}

void OpenMVPlugin::showFeatureStatus()
{
    m_featureLabel->setText(QStringLiteral("Tracking: ") + m_feature);
}

} // namespace Internal
} // namespace OpenMV
