/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2019 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "theme.h"
#include "ui_theme.h"

#include <QGSettings>
#include <QDebug>
#include <QtDBus/QDBusConnection>
#include <QtConcurrent>

#include "SwitchButton/switchbutton.h"
#include "cursor/xcursortheme.h"
#include "../../../shell/customstyle.h"

// GTK主题
#define THEME_GTK_SCHEMA "org.mate.interface"
#define MODE_GTK_KEY "gtk-theme"

// GTK图标主题
#define ICON_GTK_KEY "icon-theme"

// QT主题
#define THEME_QT_SCHEMA "org.ukui.style"
#define MODE_QT_KEY "style-name"
#define THEME_TRAN_KEY "menu-transparency"
#define PEONY_TRAN_KEY "peony-side-bar-transparency"

// QT图标主题
#define ICON_QT_KEY "icon-theme-name"

// 窗口管理器Marco主题
#define MARCO_SCHEMA "org.gnome.desktop.wm.preferences"
#define MARCO_THEME_KEY "theme"

#define ICONTHEMEPATH "/usr/share/icons/"
#define SYSTHEMEPATH "/usr/share/themes/"
#define CURSORS_THEMES_PATH "/usr/share/icons/"

#define CURSOR_THEME_SCHEMA "org.ukui.peripherals-mouse"
#define CURSOR_THEME_KEY "cursor-theme"

#define ICONWIDGETHEIGH 74

// 透明度设置
#define PERSONALSIE_SCHEMA     "org.ukui.control-center.personalise"
#define PERSONALSIE_TRAN_KEY   "transparency"
#define PERSONALSIE_BLURRY_KEY "blurry"
#define PERSONALSIE_EFFECT_KEY "effect"

const QString defCursor = "DMZ-White";
const QString UbuntuVesionEnhance = "20.10";
const QString kXder = "XRender";

const int transparency = 75;

const QStringList effectList {"blur", "kwin4_effect_translucency", "kwin4_effect_maximize", "zoom"};
const QStringList kIconsList {"blueman", "disk-burner", "firefox", "kylin-video", "kylin-assistant", "kylin-scanner", "kylin-ipmsg"};

namespace {

    // Preview cursors
    const char * const cursor_names[] =
    {
        "left_ptr",
        "left_ptr_watch",
        "wait",
        "pointing_hand",
        "whats_this",
        "ibeam",
        "size_all",
        "size_fdiag",
        "cross",
        "split_h",
        "size_ver",
        "size_hor",
        "size_bdiag",
        "split_v",
    };

    const int numCursors = 9;     // The number of cursors from the above list to be previewed
}

Theme::Theme()
{
    ui = new Ui::Theme;
    pluginWidget = new QWidget;
    pluginWidget->setAttribute(Qt::WA_DeleteOnClose);
    pluginName = tr("Theme");
    pluginType = PERSONALIZED;
    ui->setupUi(pluginWidget);
    ui->titleLabel->setStyleSheet("QLabel{font-size: 18px; color: palette(windowText);}");
    ui->iconLabel->setStyleSheet("QLabel{font-size: 18px; color: palette(windowText);}");
    ui->cursorLabel->setStyleSheet("QLabel{font-size: 18px; color: palette(windowText);}");
    ui->effectLabel->setStyleSheet("QLabel{font-size: 18px; color: palette(windowText);}");

    setupGSettings();
    initSearchText();

    //设置组件
    setupComponent();
    // init kwin settings
    setupSettings();
    initThemeMode();
    initIconTheme();
    initCursorTheme();
    initConnection();
}

Theme::~Theme()
{
    delete ui;
}

QString Theme::get_plugin_name() {
    return pluginName;
}

int Theme::get_plugin_type() {
    return pluginType;
}

QWidget *Theme::get_plugin_ui() {
    return pluginWidget;
}

void Theme::plugin_delay_control(){

}

const QString Theme::name() const {

    return QStringLiteral("theme");
}

void Theme::initSearchText() {
    //~ contents_path /theme/Theme Mode
    ui->titleLabel->setText(tr("Theme Mode"));
    //~ contents_path /theme/Icon theme
    ui->iconLabel->setText(tr("Icon theme"));
    //~ contents_path /theme/Cursor theme
    ui->cursorLabel->setText(tr("Cursor theme"));
    //~ contents_path /theme/Performance mode
    ui->perforLabel->setText(tr("Performance mode"));
    //~ contents_path /theme/Transparency
    ui->transparencyLabel->setText(tr("Transparency"));
}

void Theme::setupSettings() {
    QString filename = QDir::homePath() + "/.config/ukui-kwinrc";
    kwinSettings = new QSettings(filename, QSettings::IniFormat, this);

    QStringList keys = kwinSettings->childGroups();

    kwinSettings->beginGroup("Plugins");
    bool kwin = kwinSettings->value("blurEnabled", kwin).toBool();

    if (!kwinSettings->childKeys().contains("blurEnabled")) {
        kwin = true;
    }

    kwinSettings->endGroup();

    effectSwitchBtn->setChecked(kwin);

    QFileInfo dir(filename);
    if (!dir.isFile()) {
        effectSwitchBtn->setChecked(true);
    }

    if (effectSwitchBtn->isChecked()) {
        ui->transFrame->setVisible(true);
    } else {
        ui->transFrame->setVisible(false);
    }

    if (keys.contains("Compositing")) {
        kwinSettings->beginGroup("Compositing");
        QString xder;
        bool kwinOG;
        bool kwinEN;
        xder = kwinSettings->value("Backend", xder).toString();
        kwinOG = kwinSettings->value("OpenGLIsUnsafe", kwinOG).toBool();
        kwinEN = kwinSettings->value("Enabled", kwinEN).toBool();
        if (xder == kXder || kwinOG || !kwinEN) {
            ui->effectFrame->setVisible(false);
            ui->transFrame->setVisible(false);
            ui->effectLabel->setVisible(false);
            personliseGsettings->set(PERSONALSIE_EFFECT_KEY, false);
        }
        kwinSettings->endGroup();
    }
}

void Theme::setupComponent(){

    ui->lowLabel->setPixmap(QPixmap("://img/plugins/theme/opacitylow.svg"));
    ui->highlabel->setPixmap(QPixmap("://img/plugins/theme/opacityhigh.svg"));

    ui->lightButton->setVisible(getSystemVersion());
    //隐藏现阶段不支持功能
    ui->controlLabel->hide();
    ui->controlWidget->hide();

    ui->defaultButton->setProperty("value", "ukui-default");
    ui->lightButton->setProperty("value", "ukui-light");
    ui->darkButton->setProperty("value", "ukui-dark");

    buildThemeModeBtn(ui->defaultButton, tr("Default"), "default");
    buildThemeModeBtn(ui->lightButton, tr("Light"), "light");
    buildThemeModeBtn(ui->darkButton, tr("Dark"), "dark");

    ui->tranSlider->setRange(35, 100);

    ui->tranSlider->setValue(static_cast<int>(personliseGsettings->get(PERSONALSIE_TRAN_KEY).toDouble() * 100.0));
    connect(ui->tranSlider, &QSlider::sliderReleased, this, [=]() {
        personliseGsettings->set(PERSONALSIE_TRAN_KEY,(static_cast<int>(ui->tranSlider->value()) / 100.0));
        qtSettings->set(THEME_TRAN_KEY, ui->tranSlider->value());
        qtSettings->set(PEONY_TRAN_KEY, ui->tranSlider->value());
    });
    setupControlTheme();
//    ui->effectLabel->hide();
//    ui->effectWidget->hide();

    //构建并填充特效开关按钮
    effectSwitchBtn = new SwitchButton(pluginWidget);
    ui->effectHorLayout->addWidget(effectSwitchBtn);

    ui->kwinFrame->setVisible(false);
    ui->transFrame->setVisible(true);
}

void Theme::buildThemeModeBtn(QPushButton *button, QString name, QString icon){
    //设置默认按钮

    QVBoxLayout * baseVerLayout = new QVBoxLayout(button);
    baseVerLayout->setSpacing(8);
    baseVerLayout->setMargin(0);
    QLabel * iconLabel = new QLabel(button);
    iconLabel->setFixedSize(QSize(176, 105));
    iconLabel->setScaledContents(true);
    QString fullicon = QString("://img/plugins/theme/%1.png").arg(icon);
    iconLabel->setPixmap(QPixmap(fullicon));

    QHBoxLayout * bottomHorLayout = new QHBoxLayout;
    bottomHorLayout->setSpacing(8);
    bottomHorLayout->setMargin(0);
    QLabel * statusLabel = new QLabel(button);
    statusLabel->setFixedSize(QSize(16, 16));
    statusLabel->setScaledContents(true);

    QLabel * nameLabel = new QLabel(button);
    QSizePolicy nameSizePolicy = nameLabel->sizePolicy();
    nameSizePolicy.setVerticalPolicy(QSizePolicy::Fixed);
    nameSizePolicy.setHorizontalPolicy(QSizePolicy::Fixed);
    nameLabel->setSizePolicy(nameSizePolicy);
    nameLabel->setText(name);

#if QT_VERSION <= QT_VERSION_CHECK(5, 12, 0)
    connect(ui->themeModeBtnGroup, static_cast<void (QButtonGroup::*)(QAbstractButton *)>(&QButtonGroup::buttonClicked), [=](QAbstractButton * eBtn){
#else
    connect(ui->themeModeBtnGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), [=](QAbstractButton * eBtn){
#endif
        if (eBtn == button) {
            nameLabel->setStyleSheet("color: #ffffff;");
            statusLabel->setPixmap(QPixmap("://img/plugins/theme/selected.svg"));
        }
        else {
            nameLabel->setStyleSheet("color: palette(windowText);");
            statusLabel->clear();
        }
    });

    bottomHorLayout->addStretch();
    bottomHorLayout->addWidget(statusLabel);
    bottomHorLayout->addWidget(nameLabel);
    bottomHorLayout->addStretch();

    baseVerLayout->addWidget(iconLabel);
    baseVerLayout->addLayout(bottomHorLayout);

    button->setLayout(baseVerLayout);
}

void Theme::initThemeMode() {
    // 获取当前主题
    QString currentThemeMode = qtSettings->get(MODE_QT_KEY).toString();
    if ("ukui-white" == currentThemeMode || "ukui-default" == currentThemeMode) {
        ui->themeModeBtnGroup->buttonClicked(ui->defaultButton);
    } else if ("ukui-dark" == currentThemeMode){
        ui->themeModeBtnGroup->buttonClicked(ui->darkButton);
    } else {
        ui->themeModeBtnGroup->buttonClicked(ui->lightButton);
    }

    qApp->setStyle(new InternalStyle("ukui"));

    //监听主题改变
    connect(qtSettings, &QGSettings::changed, this, [=](const QString &key) {
        if (key == "styleName") {
            //获取当前主题
            QString currentThemeMode = qtSettings->get(key).toString();
            for (QAbstractButton * button : ui->themeModeBtnGroup->buttons()){
                QVariant valueVariant = button->property("value");
                if ("ukui-black" == currentThemeMode) {
                    currentThemeMode = "ukui-dark";
                } else if("ukui-white" == currentThemeMode) {
                    currentThemeMode = "ukui-default";
                }
                if (valueVariant.isValid() && valueVariant.toString() == currentThemeMode) {
                    disconnect(ui->themeModeBtnGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(themeBtnClickSlot(QAbstractButton*)));
                    button->click();
                    connect(ui->themeModeBtnGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(themeBtnClickSlot(QAbstractButton*)));
                }
            }
            qApp->setStyle(new InternalStyle("ukui"));
        }
    });

    connect(gtkSettings,&QGSettings::changed,this,[=] (const QString &key) {
       if(key == "iconTheme") {
           QString icoName = qtSettings->get(ICON_QT_KEY).toString();
           setCheckStatus(ui->iconThemeVerLayout, icoName, ICON);
       }
    });

    connect(curSettings,&QGSettings::changed,this,[=](const QString &key) {
       if(key == "cursorTheme") {
            QString cursorTheme = curSettings->get(CURSOR_THEME_KEY).toString();
            setCheckStatus(ui->cursorVerLayout, cursorTheme, CURSOR);
       }
    });

    connect(ui->themeModeBtnGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(themeBtnClickSlot(QAbstractButton*)));
}

void Theme::initIconTheme() {
    //获取当前图标主题(以QT为准，后续可以对比GTK两个值)
    QString currentIconTheme = qtSettings->get(ICON_QT_KEY).toString();

    //构建图标主题Widget Group，方便更新选中/非选中状态
    iconThemeWidgetGroup = new WidgetGroup;
    connect(iconThemeWidgetGroup, &WidgetGroup::widgetChanged, [=](ThemeWidget * preWidget, ThemeWidget * curWidget){
        if (preWidget)
            preWidget->setSelectedStatus(false);
        curWidget->setSelectedStatus(true);

        QString value = curWidget->getValue();
        //设置图标主题
        qtSettings->set(ICON_QT_KEY, value);
        gtkSettings->set(ICON_GTK_KEY, value);
    });

    //构建图标主题QDir
    QDir themesDir = QDir(ICONTHEMEPATH);

    foreach (QString themedir, themesDir.entryList(QDir::Dirs)) {
        if (themedir.startsWith("ukui-icon-theme-")){

            QDir appsDir = QDir(ICONTHEMEPATH + themedir + "/48x48/apps/");

            if ("ukui-icon-theme-basic" == themedir) {
                continue;
            }
            appsDir.setFilter(QDir::Files | QDir::NoSymLinks);
            QStringList appIconsList = appsDir.entryList();

            QStringList showIconsList;
            for (int i = 0; i < kIconsList.size(); i++) {
                showIconsList.append(appsDir.path() + "/" + kIconsList.at(i));
            }

            ThemeWidget * widget = new ThemeWidget(QSize(48, 48), dullTranslation(themedir.section("-", -1, -1, QString::SectionSkipEmpty)), showIconsList);
            widget->setValue(themedir);

            // 加入Layout
            ui->iconThemeVerLayout->addWidget(widget);

            // 加入WidgetGround实现获取点击前Widget
            iconThemeWidgetGroup->addWidget(widget);

            if (themedir == currentIconTheme){
                iconThemeWidgetGroup->setCurrentWidget(widget);
                widget->setSelectedStatus(true);
            } else {
                widget->setSelectedStatus(false);
            }
        }
    }
}

void Theme::setupControlTheme(){
    QStringList colorStringList;
    colorStringList << QString("#3D6BE5");
    colorStringList << QString("#FA6C63");
    colorStringList << QString("#6cd472");
    colorStringList << QString("#f9a959");
    colorStringList << QString("#BA7Bd8");
    colorStringList << QString("#F8D15D");
    colorStringList << QString("#E7BBB0");
    colorStringList << QString("#176F57");

    QButtonGroup * colorBtnGroup = new QButtonGroup();

    for (QString color : colorStringList){

        QPushButton * button = new QPushButton(ui->controlWidget);
        button->setFixedSize(QSize(48, 48));
        button->setCheckable(true);
        QString btnStyle = QString("QPushButton{background: %1; border-radius: 4px;}").arg(color);
//        button->setStyleSheet(btnStyle);
        colorBtnGroup->addButton(button, colorStringList.indexOf(color));

        QVBoxLayout * colorVerLayout = new QVBoxLayout();
        colorVerLayout->setSpacing(0);
        colorVerLayout->setMargin(0);

        QHBoxLayout * colorHorLayout = new QHBoxLayout();
        colorHorLayout->setSpacing(0);
        colorHorLayout->setMargin(0);

        QLabel * selectedColorLabel = new QLabel(button);
        QSizePolicy scSizePolicy = selectedColorLabel->sizePolicy();
        scSizePolicy.setHorizontalPolicy(QSizePolicy::Fixed);
        scSizePolicy.setVerticalPolicy(QSizePolicy::Fixed);
        selectedColorLabel->setSizePolicy(scSizePolicy);
        selectedColorLabel->setScaledContents(true);
        selectedColorLabel->setPixmap(QPixmap("://img/plugins/theme/selected.png"));
        //初始化选中图标状态
        selectedColorLabel->setVisible(button->isChecked());
//        connect(colorBtnGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this,[=]{
//            selectedColorLabel->setVisible(button->isChecked());
//            //设置控件主题
//        });

        colorHorLayout->addStretch();
        colorHorLayout->addWidget(selectedColorLabel);
        colorVerLayout->addLayout(colorHorLayout);
        colorVerLayout->addStretch();

        button->setLayout(colorVerLayout);

        ui->controlHorLayout->addWidget(button);
    }
}

void Theme::initCursorTheme(){

    QStringList cursorThemes = _getSystemCursorThemes();

    //获取当前指针主题
    QString currentCursorTheme;
    currentCursorTheme = curSettings->get(CURSOR_THEME_KEY).toString();

    cursorThemeWidgetGroup = new WidgetGroup(this);
    connect(cursorThemeWidgetGroup, &WidgetGroup::widgetChanged, [=](ThemeWidget * preWidget, ThemeWidget * curWidget){
        if (preWidget)
            preWidget->setSelectedStatus(false);
        curWidget->setSelectedStatus(true);

        QString value = curWidget->getValue();
        //设置光标主题
        curSettings->set(CURSOR_THEME_KEY, value);

#if QT_VERSION <= QT_VERSION_CHECK(5,12,0)

#else
        QString filename = QDir::homePath() + "/.config/kcminputrc";
        QSettings *mouseSettings = new QSettings(filename, QSettings::IniFormat);

        mouseSettings->beginGroup("Mouse");
        mouseSettings->setValue("cursorTheme", value);
        mouseSettings->endGroup();

        delete mouseSettings;

        QDBusMessage message = QDBusMessage::createSignal("/KGlobalSettings", "org.kde.KGlobalSettings", "notifyChange");
        QList<QVariant> args;
        args.append(5);
        args.append(0);
        message.setArguments(args);
        QDBusConnection::sessionBus().send(message);
#endif
    });

    for (QString cursor : cursorThemes){

        QList<QPixmap> cursorVec;
        QString path = CURSORS_THEMES_PATH + cursor;
        XCursorTheme *cursorTheme = new XCursorTheme(path);

        for(int i = 0; i < numCursors; i++){
            int size = qApp->devicePixelRatio() * 8;
            QImage image = cursorTheme->loadImage(cursor_names[i],size);
            cursorVec.append(QPixmap::fromImage(image));
        }

        ThemeWidget * widget  = new ThemeWidget(QSize(24, 24), cursor, cursorVec);
        widget->setValue(cursor);

        //加入Layout
        ui->cursorVerLayout->addWidget(widget);

        //加入WidgetGround实现获取点击前Widget
        cursorThemeWidgetGroup->addWidget(widget);

        //初始化指针主题选中界面
        if (currentCursorTheme == cursor || (currentCursorTheme.isEmpty() && cursor == defCursor)){
            cursorThemeWidgetGroup->setCurrentWidget(widget);
            widget->setSelectedStatus(true);
        } else {
            widget->setSelectedStatus(false);
        }
    }
}

void Theme::initEffectSettings() {

}

void Theme::initConnection() {

    connect(ui->resetBtn, &QPushButton::clicked, this, &Theme::resetBtnClickSlot);

    connect(effectSwitchBtn, &SwitchButton::checkedChanged, [this](bool checked) {
        if (!checked) {
            personliseGsettings->set(PERSONALSIE_TRAN_KEY, 0.95);
            qtSettings->set(THEME_TRAN_KEY, 95);
            qtSettings->set(PEONY_TRAN_KEY, 95);
            ui->tranSlider->setValue(95);
        }
        // 提供给外部监听特效接口
        personliseGsettings->set(PERSONALSIE_EFFECT_KEY, checked);
        QString currentThemeMode = qtSettings->get(MODE_QT_KEY).toString();
        ui->transFrame->setVisible(checked);
        writeKwinSettings(checked, currentThemeMode, true);
    });

#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
    ui->transFrame->setVisible(false);
    ui->effectFrame->setVisible(false);
    ui->effectLabel->setVisible(false);
#else

#endif
}

QStringList Theme::_getSystemCursorThemes() {
    QStringList themes;
    QDir themesDir(CURSORS_THEMES_PATH);

    if (themesDir.exists()){
        foreach (QString dirname, themesDir.entryList(QDir::Dirs)){
            if (dirname == "." || dirname == "..")
                continue;
//            QString fullpath(CURSORS_THEMES_PATH + dirname);
            QDir themeDir(CURSORS_THEMES_PATH + dirname + "/cursors/");
            if (themeDir.exists())
                themes.append(dirname);
        }
    }
    return themes;
}

bool Theme::getSystemVersion() {
    QString versionPath = "/etc/os-release";
    QStringList osRes =  readFile(versionPath);
    QString version;

    for (QString str : osRes) {
        if (str.contains("VERSION_ID=")) {
            int index = str.indexOf("VERSION_ID=");
            int startIndex = index + 12;
            int length = str.length() - startIndex - 1;
            version = str.mid(startIndex, length);
        }
    }
    if (UbuntuVesionEnhance == version) {
        return true;
    }
    return false;
}

QStringList Theme::readFile(QString filepath) {
    QStringList fileCont;
    QFile file(filepath);
    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "ReadFile() failed to open" << filepath;
            return QStringList();
        }
        QTextStream textStream(&file);
        while (!textStream.atEnd()) {
            QString line= textStream.readLine();
            line.remove('\n');
            fileCont<<line;
        }
        file.close();
        return fileCont;
    } else {
        qWarning() << filepath << " not found"<<endl;
        return QStringList();
    }
}

void Theme::setupGSettings() {
    const QByteArray id(THEME_GTK_SCHEMA);
    const QByteArray idd(THEME_QT_SCHEMA);
    const QByteArray iid(CURSOR_THEME_SCHEMA);
    const QByteArray iiid(PERSONALSIE_SCHEMA);
    gtkSettings = new QGSettings(id, QByteArray(), this);
    qtSettings = new QGSettings(idd, QByteArray(), this);
    curSettings = new QGSettings(iid, QByteArray(), this);
    personliseGsettings = new QGSettings(iiid, QByteArray(), this);
}

QString Theme::dullTranslation(QString str) {
    if (!QString::compare(str, "basic")){
        return QObject::tr("basic");
    } else if (!QString::compare(str, "classical")){
        return QObject::tr("classical");
    } else if (!QString::compare(str, "default")){
        return QObject::tr("default");
    } else
        return QObject::tr("Unknown");
}

// reset all of themes, include cursor, icon,and etc...
void Theme::resetBtnClickSlot() {

    emit ui->themeModeBtnGroup->buttonClicked(ui->defaultButton);

    // reset cursor default theme
    QString cursorTheme = "DMZ-White";
    curSettings->set(CURSOR_THEME_KEY,cursorTheme);

    //reset icon default theme
    qtSettings->reset(ICON_QT_KEY);

    if (ui->effectFrame->isVisible() && effectSwitchBtn->isChecked()) {
        qtSettings->reset(THEME_TRAN_KEY);
        qtSettings->reset(PEONY_TRAN_KEY);
        gtkSettings->reset(ICON_GTK_KEY);
        personliseGsettings->reset(PERSONALSIE_TRAN_KEY);
        ui->tranSlider->setValue(transparency);
    }

    QString icoName = qtSettings->get(ICON_QT_KEY).toString();

    setCheckStatus(ui->iconThemeVerLayout, icoName, ICON);
    setCheckStatus(ui->cursorVerLayout, cursorTheme, CURSOR);
}

void Theme::writeKwinSettings(bool change, QString theme, bool effect) {
    Q_UNUSED(theme);
    Q_UNUSED(effect);

    if (!change) {
        kwinSettings->beginGroup("Plugins");
        kwinSettings->setValue("blurEnabled", false);
        kwinSettings->setValue("kwin4_effect_maximizeEnabled", false);
        kwinSettings->setValue("kwin4_effect_translucencyEnabled", false);
        kwinSettings->setValue("zoomEnabled", false);
        kwinSettings->endGroup();
#if QT_VERSION <= QT_VERSION_CHECK(5, 12, 0)

#else
        for (int i = 0; i < effectList.length(); i++) {
            QDBusMessage message = QDBusMessage::createMethodCall("org.ukui.KWin",
                                                       "/Effects",
                                                       "org.ukui.kwin.Effects",
                                                       "unloadEffect");
            message << effectList.at(i);
            QDBusConnection::sessionBus().send(message);
        }
#endif
    } else {
        kwinSettings->beginGroup("Plugins");
        kwinSettings->setValue("blurEnabled", true);
        kwinSettings->setValue("kwin4_effect_maximizeEnabled", true);
        kwinSettings->setValue("kwin4_effect_translucencyEnabled", true);
        kwinSettings->setValue("zoomEnabled", true);
        kwinSettings->endGroup();
#if QT_VERSION <= QT_VERSION_CHECK(5, 12, 0)

#else
        // 开启模糊特效：
        for (int i = 0; i < effectList.length(); i++) {

            QDBusMessage message = QDBusMessage::createMethodCall("org.ukui.KWin",
                                                                  "/Effects",
                                                                  "org.ukui.kwin.Effects",
                                                                  "loadEffect");
            message << effectList.at(i);
            QDBusConnection::sessionBus().send(message);
        }
#endif

    }
    kwinSettings->sync();
}

void Theme::themeBtnClickSlot(QAbstractButton *button) {
    // 设置主题
     QString themeMode = button->property("value").toString();
     QString currentThemeMode = qtSettings->get(MODE_QT_KEY).toString();

     qApp->setStyle(new InternalStyle(themeMode));
     if (QString::compare(currentThemeMode, themeMode)){
         QString tmpMode;
         if ("ukui-dark" == themeMode) {
             tmpMode = "ukui-black";
         } else {
             tmpMode = "ukui-white";
         }
         gtkSettings->set(MODE_GTK_KEY, tmpMode);

         QtConcurrent::run([=](){
             qtSettings->set(MODE_QT_KEY, themeMode);
         });
     }
}

void Theme::setCheckStatus(QLayout *mlayout, QString checkName, ThemeType type) {
    QLayoutItem *item;
    if (mlayout->layout() != NULL ) {
        int size = mlayout->layout()->count();
        for (int i = 0; i < size; i++) {
            item = mlayout->layout()->itemAt(i);
            ThemeWidget *themeWdt = static_cast<ThemeWidget *>(item->widget());
            themeWdt->setSelectedStatus(false);
            if (themeWdt->getValue() == checkName) {
                themeWdt->setSelectedStatus(true);
                if (type == ICON) {
                    iconThemeWidgetGroup->setCurrentWidget(themeWdt);
                } else {
                    cursorThemeWidgetGroup->setCurrentWidget(themeWdt);
                }
            }
        }
    }
}

double Theme::convertToTran(const int value) {
    switch (value) {
    case 1:
        return 0.2;
        break;
    case 2:
        return 0.4;
        break;
    case 3:
        return 0.6;
        break;
    case 4:
        return 0.8;
        break;
    case 5:
        return 1.0;
        break;
    default:
        return 1.0;
        break;
    }
}

int Theme::tranConvertToSlider(const double value) {
    if (0.2 ==  value) {
        return 1;
    } else if (0.4 ==  value) {
        return 2;
    } else if (0.6 ==  value) {
        return 3;
    } else if (0.8 ==  value) {
        return 4;
    } else if (1.0 ==  value) {
        return 5;
    } else {
        return 5;
    }
}
