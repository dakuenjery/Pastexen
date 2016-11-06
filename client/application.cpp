#include <QIcon>
#include <QKeySequence>
#include <QFile>
#include <QTemporaryFile>
#include <QPixmap>
#include <QDesktopWidget>
#include <QDebug>
#include <QClipboard>
#include <QMessageBox>
#include <QBuffer>
#include <QLocalSocket>
#include <QTimer>
#include <QDir>
#include <QScreen>
#include <QThread>
#include <QPixmap>
#include <QDesktopServices>
#include "application.h"
#include "imageselectwidget.h"
#include "ui_config.h"
#include "languageselectdialog.h"
#include "screenshoteditwidget.h"
#include "utils.h"
#include "application.h"
#include "traywindow.h"

#ifdef Q_OS_LINUX
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <string>
#include <climits>

QString getClipboardText() {
    std::string result;
    Display* d = XOpenDisplay(nullptr);
    if(d) {
        Window wParent = DefaultRootWindow(d);
        Window w = XCreateSimpleWindow(
            d, wParent, 0, 0, 1, 1, 0, CopyFromParent, CopyFromParent);
        Atom target = XInternAtom(d, "UTF8_STRING", False);
        XConvertSelection(d, XA_PRIMARY, target, None, w, CurrentTime);
        XFlush(d);

        for(unsigned long offset = 0;;) {
            XEvent ev;
            XNextEvent(d, &ev);
            if(SelectionNotify == ev.type
               && None != ev.xselection.property
            ) {
                Atom actual_type_return = None;
                int actual_format_return = 0;
                unsigned long nitems_return = 0;
                unsigned long bytes_after_return = 0;
                unsigned char* prop_return = nullptr;
                XGetWindowProperty(
                    d, w, ev.xselection.property, offset, INT_MAX
                    , False, AnyPropertyType, &actual_type_return
                    , &actual_format_return, &nitems_return
                    , &bytes_after_return, &prop_return);
                if(nitems_return) {
                    result.append(reinterpret_cast<const char*>(prop_return)
                                  , nitems_return);
                    offset += nitems_return;
                }
                XDeleteProperty(d, w, ev.xselection.property);
                if(!bytes_after_return) {
                    break;
                }
            }
        }
        XDestroyWindow(d, w);
        XCloseDisplay(d);
    }
    return QString(result.c_str());
}
#endif



Application::Application(int& argc, char *argv[]) :
    QApplication(argc, argv)
    , _autorun(new AutorunManager(this))
    , _configWidget(0)
    , _trayWindow(0)
    , _trayIcon(0)
    , _trayIconMenu(0)
    , _network(new NetworkManager(this))
    , _settings(0)
    , Sharing(false)
    , _timerId(-1)
{
}

Application::~Application()
{
    if (_trayIcon) {
        _trayIcon->hide();
    }
    if (_settings) {
        _settings->Save();
    }
    if (_trayIconMenu) {
        // todo: use smart pointers
        delete _trayIconMenu;
    }
}

//---- Get parameters from app instance and upload ---//
void Application::localRequestReceived()
{
    QString path = _localConnection->readAll();
    uploadFile(path); // upload given file
}

//---- Upload file to server ---//
void Application::uploadFile(QString request)
{
    // For get extension of file
    QFileInfo ImgExt;
    ImgExt.setFile(request);

    //----------------------Loading Images-------------------------//
    if ((ImgExt.completeSuffix()=="png")||(ImgExt.completeSuffix()=="jpg")||(ImgExt.completeSuffix()=="gif")){
        if (Sharing) {
            return;
        }

        if (!checkEllapsed()) {
            return;
        }

        Sharing = true;

        _trayWindow->showMessage(tr("Uploading image..."), TMT_None, 60000, true);

        QPixmap pixmap;

        // try load image
        if(! pixmap.load(request)){
            _trayWindow->showMessage(tr("Failed to load image"), TMT_Error);
            Sharing = false;
            return;
        }

        QString imagetype = settings().GetParameter("imagetype", DEFAULT_IMAGE_TYPE);

        QByteArray imageBytes;
        QBuffer buffer(&imageBytes);
        buffer.open(QFile::WriteOnly);
        pixmap.save(&buffer, imagetype.toLocal8Bit().constData());
        buffer.close();
        sending();
        _network->upload("", imageBytes, ImgExt.completeSuffix());
        _trayWindow->showUploadMessage(tr("Uploading image..."));
        Sharing = false;
    }

    //----------------------Loading Text-------------------------//
    else if(_languages.contains(ImgExt.completeSuffix())){

        if (Sharing) {
            return;
        }
        if (!checkEllapsed()) {
            return;
        }

        Sharing = true;
        _trayWindow->showMessage(tr("Uploading code..."), TMT_None, 60000, true);
        QFile source(request);
        if (!source.open(QIODevice::ReadOnly)){
            _trayWindow->showMessage(tr("Failed to open file"), TMT_Error);
            Sharing = false;
            return;
        }

        QTextStream textStream(&source);
        QString text = textStream.readAll();
        sending();
        _network->upload("", text.toUtf8(), ImgExt.completeSuffix());
        _trayWindow->showUploadMessage(tr("Uploading code..."));

        Sharing = false;
    } else {
        _trayWindow->showMessage(tr("Unknown file type"), TMT_Error);
    }
}

bool Application::pxAppInit()
{
    QLocalSocket socket;
    socket.connectToServer(APP_NAME);

    // Check if app instance already run
    if (socket.waitForConnected(500)) {
        // Send args from new instance, and check if args given
        if (QApplication::arguments().count()>1){
            socket.write(QApplication::arguments().at(1).toUtf8());
        }
        qDebug() << "Application already launched!";
        return false;
    }

    qApp->setAttribute(Qt::AA_UseHighDpiPixmaps);

    _lastSended.start();

    _localServer = new QLocalServer(this);

    if (!_localServer->listen(APP_NAME)) {
        QLocalServer::removeServer(APP_NAME);
        _localServer->listen(APP_NAME);
    }

    connect(_localServer, SIGNAL(newConnection()), SLOT(newLocalSocketConnection()));

#ifdef WITH_LIBQTSCREEN
    connect(&FullScreenMaker, &NQtScreen::TScreenShotMaker::OnScreenshotReady, [this]() {
        this->processScreenshotNext(MakingFullScreen, true);
    });
    connect(&FullScreenMaker, &NQtScreen::TScreenShotMaker::OnFailed, [this]() {
        this->processScreenshotNext(MakingFullScreen, false);
    });
#endif

    const QString& settingsFile = QDir::homePath() + "/" + SETTINGS_FILE;
    _settings = new USettings();
    try {
        _settings->Load(settingsFile);
    } catch (...) {
        qDebug() << "Warning: failed to load settings";
    }

    QString uuid = _settings->GetParameter("uuid", "");
    if (uuid.length() != 24 * 2) {
        uuid = GenerateUUID();
        Q_ASSERT(uuid.length() == 24 * 2);
        _settings->SetParameter("uuid", uuid);
        try {
            _settings->Save();
        } catch (...) {
            qDebug() << "Error: failed to save settings";
        }
    }

    initLanguages();

    _configWidget = new ConfigWidget(GetAppName(), _languages);
    connect(_configWidget, SIGNAL(settingsChanged()), SLOT(setupHotkeys()));
    connect(_configWidget, &ConfigWidget::settingsChanged, [this] {
        updateAutostart();
    });
    connect(_configWidget, SIGNAL(hotkeyActivated(size_t)), SLOT(hotkeyPressed(size_t)));

    _configWidget->init();

    _trayIconMenu = new QMenu;
    _trayIconMenu->addAction(tr("About"), this, SLOT(aboutDialog()));
    _trayIconMenu->addAction("", this, SLOT(processCodeShare()));
    _trayIconMenu->addAction("", this, SLOT(processScreenshotFull()));
    _trayIconMenu->addAction("", this, SLOT(processScreenshotPart()));
    //_trayIconMenu->addAction(tr("Configure"), _configWidget, SLOT(show()));
    _trayIconMenu->addAction(tr("Configure"), this, SLOT(showConfigWindow()));
    _trayIconMenu->addAction(tr("My files"), this, SLOT(accountLink()));
    _trayIconMenu->addSeparator();
    _trayIconMenu->addAction(tr("Exit"), this, SLOT(quit()));                   // Tray menu

    setupHotkeys();

    _trayIcon = new QSystemTrayIcon(QIcon(":/icons/icon.png"), this);
    #ifndef Q_OS_MAC
    connect(_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            SLOT(trayIconClicked(QSystemTrayIcon::ActivationReason)));
    #endif
    _trayIcon->setContextMenu(_trayIconMenu);                                   // Tray icon

    _network->init();
    connect(_network, SIGNAL(linkReceived(QString)), SLOT(linkAvaliable(QString)));     // Network

    this->setQuitOnLastWindowClosed(false);

    _trayWindow = new TrayWindow;

    _trayIcon->show();
    QFile file(settingsFile);

    _lastSended = _lastSended.addSecs(5); // for check on first run
    connect(_network, SIGNAL(ready()), SLOT(resolved()));

    if (!file.exists())
        _configWidget->show();

    if (QApplication::arguments().count()>1){
        QString fileName = QApplication::arguments().at(1);
        QMetaObject::invokeMethod(this, "uploadFile", Qt::QueuedConnection, Q_ARG(QString, fileName));
    }

    updateAutostart();

    return true;
}

void Application::showConfigWindow()
{
    _configWidget->show();
    #if defined(Q_OS_MAC)
    _configWidget->activateWindow();
    _configWidget->raise();
    #endif
}

void Application::resolved()
{
    // If program have parameter upload file
    if (QApplication::arguments().count()>1){
        uploadFile(QApplication::arguments().at(1).toUtf8());
    }
}

void Application::newLocalSocketConnection()
{
    _localConnection = _localServer->nextPendingConnection();
    connect(_localConnection, SIGNAL(readyRead()), SLOT(localRequestReceived()));
}

void Application::trayMessage(const QString& text, ETrayMessageType type)
{
    _trayWindow->showMessage(text, type);
}

void Application::processScreenshot(bool isFullScreen) {
    if (Sharing) {
        return;
    }
    if (!checkEllapsed()) {
        return;
    }
    Sharing = true;

#ifdef WITH_LIBQTSCREEN
    MakingFullScreen = isFullScreen;
    FullScreenMaker.MakeScreenshot();
#else
    processScreenshotNext(isFullScreen, false);
#endif
}

void Application::processScreenshotNext(bool isFullScreen, bool useFullScreenMaker)
{
    QPixmap pixmap;

#ifdef WITH_LIBQTSCREEN
    if (useFullScreenMaker) {
        pixmap = QPixmap::fromImage(FullScreenMaker.GetLastScreenshot());
    } else {
        pixmap = QGuiApplication::primaryScreen()->grabWindow(0);
        pixmap.setDevicePixelRatio(qApp->devicePixelRatio());
    }
#else
    pixmap = QGuiApplication::primaryScreen()->grabWindow(0);
    pixmap.setDevicePixelRatio(qApp->devicePixelRatio());
#endif

    if (!isFullScreen) {
        ImageSelectWidget imageSelectDialog(&pixmap);
        imageSelectDialog.setWindowState(Qt::WindowFullScreen);
        if (!imageSelectDialog.exec()) {
            Sharing = false;
            return;
        }
    }

    bool editScreenshot = settings().GetParameter("showeditscreenshot", ToString(DEFAULT_SHOW_EDIT_SCREENSHOT));

    if (editScreenshot && !isFullScreen) {
        ScreenshotEditWidget editDialog(&pixmap);
        if (!editDialog.exec()) {
            Sharing = false;
            return;
        }
    }

    _trayWindow->showMessage(tr("Uploading image..."), TMT_None, 60000, true);

    bool scaleScreenshot = Application::settings().GetParameter("scalescreenshot", ToString(DEFAULT_SCALE_SCREENSHOT));

    if (scaleScreenshot) {
        qreal scaleRatio = qApp->devicePixelRatio();
        pixmap = pixmap.scaled(qreal(pixmap.width()) / scaleRatio,
                               qreal(pixmap.height()) / scaleRatio,
                               Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    QString imagetype = settings().GetParameter("imagetype", DEFAULT_IMAGE_TYPE);

    QByteArray imageBytes;
    QBuffer buffer(&imageBytes);
    buffer.open(QFile::WriteOnly);
    pixmap.save(&buffer, imagetype.toLocal8Bit().constData(), 90);
    buffer.close();
    try {
        sending();
        _network->upload("", imageBytes, imagetype);
        _trayWindow->showUploadMessage(tr("Uploading image..."));
    } catch (UException &e) {
        emit trayMessage(e.what(), TMT_Error);
        return;
    }
    Sharing = false;
}

void Application::processCodeShare()
{
    if (!checkEllapsed()) {
        return;
    }

    sending();

#ifdef Q_OS_WIN
    QApplication::processEvents();

    INPUT input[4];

    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = VK_CONTROL;
    input[0].ki.wScan = 0;
    input[0].ki.dwFlags = 0;
    input[0].ki.time = 0;
    input[0].ki.dwExtraInfo = 0;

    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = 'C';
    input[1].ki.wScan = 0;
    input[1].ki.dwFlags = 0;
    input[1].ki.time = 10;
    input[1].ki.dwExtraInfo = 0;

    input[2].type = INPUT_KEYBOARD;
    input[2].ki.wVk = 'C';
    input[2].ki.wScan = 0;
    input[2].ki.dwFlags = KEYEVENTF_KEYUP;
    input[2].ki.time = 20;
    input[2].ki.dwExtraInfo = 0;

    input[3].type = INPUT_KEYBOARD;
    input[3].ki.wVk = VK_CONTROL;
    input[3].ki.wScan = 0;
    input[3].ki.dwFlags = KEYEVENTF_KEYUP;
    input[3].ki.time = 30;
    input[3].ki.dwExtraInfo = 0;

    SendInput(4, input, sizeof(INPUT));

    #elif defined(Q_OS_MAC)

    CGEventSourceRef src = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
    CGEventRef event2, event3;
    event2 = CGEventCreateKeyboardEvent(src, (CGKeyCode)kVK_ANSI_C, true);
    event3 = CGEventCreateKeyboardEvent(src, (CGKeyCode)kVK_ANSI_C, false);

    CGEventSetFlags(event2, kCGEventFlagMaskCommand);
    CGEventSetFlags(event3, kCGEventFlagMaskCommand);

    CGEventTapLocation loc = kCGHIDEventTap;
    CGEventPost(loc, event2);
    CGEventPost(loc, event3);

    CFRelease(event2);
    CFRelease(event3);

    CFRelease(src);

    #endif

    _trayWindow->showMessage(tr("Uploading code..."), TMT_None, 60000, true);
    _timerId = this->startTimer(200);
}

void Application::accountLink()
{
    QDesktopServices::openUrl(QUrl(Application::GetAccountUrl()));
}

void Application::trayIconClicked(const QSystemTrayIcon::ActivationReason &button)
{
    switch (button)
    {
    case QSystemTrayIcon::Trigger:
        _configWidget->show();
        break;
    case QSystemTrayIcon::MiddleClick:
        this->quit();
        break;
    default:
        break;
    }
}

void Application::linkAvaliable(const QString &link)
{
    QApplication::clipboard()->setText(link);
}

void Application::aboutDialog()
{
    QMessageBox::information(NULL, APP_NAME,
                             tr("Utility for easy screenshoting and code sharing<br> We live at <a href=\"http://pastexen.com/\">pastexen.com</a>"));
}

void Application::setupHotkeys()
{
    QString fullHotkey = settings().GetParameter("fullhotkey", DEFAULT_HOTKEY_FULL);
    QString partHotkey = settings().GetParameter("parthotkey", DEFAULT_HOTKEY_PART);
    QString codeHotkey = settings().GetParameter("texthotkey", DEFAULT_HOTKEY_CODE);

    QList<QAction*> actsList = _trayIconMenu->actions();
    actsList[1]->setText(tr("Text share (%1)").arg(codeHotkey));
    actsList[2]->setText(tr("Full s-shot (%1)").arg(fullHotkey));
    actsList[3]->setText(tr("Half s-shot (%1)").arg(partHotkey));
}

void Application::initLanguages()
{
    _languages.insert("auto", tr("Auto detection"));
    _languages.insert("txt", tr("Plain text"));
    _languages.insert("c", "C");
    _languages.insert("cpp", "C++ ");
    _languages.insert("h", "C header");
    _languages.insert("hpp", "C header");
    _languages.insert("cs", "C#");
    _languages.insert("java", "Java");
    _languages.insert("php", "PHP");
    _languages.insert("py", "Python");
    _languages.insert("pl", "Perl");
    _languages.insert("pas", "Pascal");
    _languages.insert("m", "ObjectiveC");
    _languages.insert("xml", "XML");
    _languages.insert("html", "HTML");
    _languages.insert("js", "Javascript");
    _languages.insert("css", "CSS");
    _languages.insert("json", "Json");
    _languages.insert("as", "ActionScript");
    _languages.insert("vb", "VBscript");
    _languages.insert("d", "D");
    _languages.insert("sql", "SQL");
    _languages.insert("st", "Smalltalk");
    _languages.insert("lisp", "LISP");
    _languages.insert("ini", "ini");
    _languages.insert("conf", "Apache");
    _languages.insert("sh", "BASH");
    _languages.insert("bat", "Dos (bat)");
    _languages.insert("cmake", "CMake");
    _languages.insert("hs", "Haskell");
}

bool Application::checkEllapsed()
{
    if (_lastSended.elapsed() < 3000) {
        return false;
    }
    return true;
}

void Application::sending()
{
    _lastSended.restart();
}

void Application::updateAutostart()
{
    bool autostart = _settings->GetParameter("autostart", ToString(DEFAULT_AUTOSTART));

    if ((autostart && _autorun->isInstalled()) || (!autostart && !_autorun->isInstalled()))
        return;

    else if (autostart && !_autorun->isInstalled())
        _autorun->install();

    else if (!autostart && _autorun->isInstalled())
        _autorun->uninstall();
}

void Application::timerEvent(QTimerEvent *) {
    if (_timerId == -1) { // Fix for unknown timerEvent-s, #108
        return;
    }
    this->killTimer(_timerId);
    _timerId = -1;

    QString text;

#ifdef Q_OS_LINUX
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) qDebug() << "DPU ERROR!";
    XTestFakeKeyEvent(dpy, KEYCODE_LCONTROL, KEY_DOWN, CurrentTime);
    XTestFakeKeyEvent(dpy, KEYCODE_C, KEY_DOWN, 0);
    QThread::msleep(200);
    XTestFakeKeyEvent(dpy, KEYCODE_LCONTROL, KEY_UP, CurrentTime);
    XTestFakeKeyEvent(dpy, KEYCODE_C, KEY_UP, 0);
    XCloseDisplay(dpy);
    text = getClipboardText();
#elif defined(Q_OS_WIN)
    text = QApplication::clipboard()->text();
#elif defined(Q_OS_MAC)
    text = QApplication::clipboard()->text();
#endif

    if (text.count() == 0) {
        trayMessage(tr("No text found in clipboard"), TMT_Error);
        return;
    }

    bool showsourcedialog = settings().GetParameter("showsourcedialog", ToString(DEFAULT_SHOW_SOURCES_CONF_DIALOG));
    if (showsourcedialog) {
        LanguageSelectDialog dialog(_languages);
        if (!dialog.exec()) {
            return;
        }
    }

    QString sourcestype = settings().GetParameter("sourcetype", DEFAULT_SOURCES_TYPE);

    try {
        _network->upload("", text.toUtf8(), sourcestype);
        _trayWindow->showUploadMessage(tr("Uploading code..."));
    } catch(UException &e) {
        emit trayMessage(e.what(), TMT_Error);
        return;
    }
}
