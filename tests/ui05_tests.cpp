// SPDX-License-Identifier: Apache-2.0
#include "core/Plugin.h"
#include "ui/Editor.h"
#include "ui/ExportDialog.h"
#include "ui/MixerView.h"
#include "ui/PerformanceKeyboard.h"
#include "ui/PluginBrowser.h"
#include "ui/ProductionControls.h"
#include "ui/SettingsDialog.h"
#include "ui/SoundBrowser.h"
#include "ui/Theme.h"
#include <QTest>
#include <QTranslator>
#include <QtWidgets>
#include <fstream>
#include <iostream>
using namespace reverie;
void check(bool b, const char* m) {
    if (!b)
        throw std::runtime_error(m);
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setOrganizationName("QuadraJ-QA05");
    app.setApplicationName("Ensemble-" + QFileInfo(app.applicationDirPath()).fileName() +
                           (app.arguments().contains("--native") ? "-native" : "-offscreen"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       app.applicationDirPath() + "/qa-settings");
    QSettings().clear();
    bool native = app.arguments().contains("--native");
    try {
        QProcess scanner;
        scanner.start(QString::fromUtf8(REVERIE_SCANNER),
                      {QString::fromUtf8(REVERIE_CRASH_FIXTURE)});
        check(scanner.waitForFinished(10000) && scanner.exitStatus() == QProcess::CrashExit,
              "scanner child crash isolated");
        auto root = std::filesystem::path(REVERIE_TEST_ROOT);
        auto out =
            root /
            (native ? "out/native05"
                    : "out/ui05/" + QFileInfo(app.applicationDirPath()).fileName().toStdString());
        std::filesystem::create_directories(out);
        Editor e(native);
        check(e.project.tracks[0].preset.instrumentId == "quadraj.cc0.upright" &&
                  e.project.tracks[1].preset.instrumentId == "quadraj.cc0.strings",
              "starter instruments resolve by stable ID after factory expansion");
        e.resize(1280, 800);
        e.show();
        auto* browser = e.findChild<SoundBrowser*>("soundBrowser");
        browser->setExpanded(true);
        QTest::qWait(200);
        browser->parentWidget()->grab().save(
            QString::fromStdString((out / "factory-browser.png").string()));
        browser->setExpanded(false);
        auto infos = scanClap(root / "out/surge05/Surge XT.clap");
        Preset p;
        p.kind = InstrumentKind::Clap;
        p.name = infos[0].name;
        p.pluginId = infos[0].id;
        p.backendPath = infos[0].path;
        p.gain = .3;
        p.renderTailSeconds = 4;
        {
            PluginInstance editor(p, 48000);
            auto params = editor.parameters();
            for (auto& v : params)
                if (!v.readOnly && v.maximum > v.minimum) {
                    editor.setParameter(v.id, v.minimum + (v.maximum - v.minimum) * .4);
                    break;
                }
            p.pluginState = editor.saveState();
        }
        e.edit([&] {
            e.project.tracks.clear();
            e.selectedTrack = -1;
            e.selectedClip = 0;
            e.project.name = "Plugin Performance";
        });
        e.applyInstrument(p, true);
        e.armTrack(e.selectedTrack, true);
        e.showKeyboard();
        auto* keys = e.findChild<PerformanceKeyboard*>("performanceKeyboard");
        e.findChild<QComboBox*>("countIn")->setCurrentIndex(0);
        keys->setFocus();
        e.toggleRecord();
        auto pump = [&](int ms) {
            if (native)
                QTest::qWait(ms);
            else {
                std::array<float, 512> pcm{};
                for (int i = 0; i < ms / 5; ++i)
                    e.engine.process(pcm.data(), 256);
                QApplication::processEvents();
            }
        };
        pump(100);
        keys->setFocus();
        QTest::keyPress(keys, Qt::Key_A);
        QTest::keyPress(keys, Qt::Key_D);
        QTest::keyPress(keys, Qt::Key_G);
        pump(300);
        float peak = e.engine.masterPeak();
        e.grab().save(QString::fromStdString((out / "plugin-midi-recording.png").string()));
        QTest::keyPress(keys, Qt::Key_Tab);
        QTest::keyRelease(keys, Qt::Key_A);
        QTest::keyRelease(keys, Qt::Key_D);
        QTest::keyRelease(keys, Qt::Key_G);
        pump(100);
        QTest::keyRelease(keys, Qt::Key_Tab);
        pump(50);
        e.stop();
        auto& t = e.project.tracks[e.selectedTrack];
        check(t.clips.back().notes.size() == 3 && peak > 0,
              "real Surge typing/chord/MIDI capture/audio");
        auto id = t.id;
        auto state = t.preset.pluginState;
        check(e.saveProject(QString::fromStdString((out / "Plugin Performance.reverie").string())),
              "save");
        check(e.openProject(QString::fromStdString((out / "Plugin Performance.reverie").string())),
              "reopen");
        check(findTrack(e.project, id)->preset.pluginState == state, "plugin state restored");
        check(e.exportProject(QString::fromStdString((out / "Plugin Performance.wav").string())),
              "WAV");
        QTranslator fixture;
        check(fixture.load(QString::fromUtf8(REVERIE_TEST_QM)), "load Korean QM fixture");
        app.installTranslator(&fixture);
        SettingsDialog settings(e);
        check(settings.windowTitle() == QString::fromUtf8("앙상블 설정"),
              "QTranslator/UTF-8 Korean UI");
        settings.show();
        if (auto* tabs = settings.findChild<QTabWidget*>())
            tabs->setCurrentIndex(tabs->count() - 1);
        QTest::qWait(20);
        auto* lang = settings.findChild<QComboBox*>("languageChoice");
        check(lang != nullptr, "language choice");
        check(settings.findChild<QComboBox*>("settingsOutput")->itemData(0).toString() ==
                  "System default",
              "device identity remains untranslated");
        lang->setCurrentIndex(lang->findData("ko_KR"));
        check(QSettings().value("interface/language").toString() == "ko_KR",
              "language persistence");
        settings.grab().save(QString::fromStdString((out / "language-fixture.png").string()));
        settings.close();
        app.removeTranslator(&fixture);
        check(findTrack(e.project, id)->preset.pluginState == state,
              "language does not reset project");
        QTimer::singleShot(200, &e, [&] {
            for (auto* w : QApplication::topLevelWidgets())
                if (w->objectName() == "pluginParameters") {
                    w->grab().save(
                        QString::fromStdString((out / "plugin-parameters.png").string()));
                    qobject_cast<QDialog*>(w)->reject();
                }
        });
        bool accepted;
        auto patch = p;
        editPluginParameters(e, patch, accepted);
        QTimer::singleShot(100, &e, [&] {
            for (auto* w : QApplication::topLevelWidgets())
                if (w->objectName() == "channelProcessing") {
                    w->grab().save(
                        QString::fromStdString((out / "channel-processing.png").string()));
                    qobject_cast<QDialog*>(w)->reject();
                }
        });
        showChannelProcessing(e, e.selectedTrack);
        QSettings().setValue("plugins/paths",
                             QStringList{QString::fromStdString((root / "out/surge05").string())});
        QTimer::singleShot(100, &e, [&] {
            for (auto* w : QApplication::topLevelWidgets())
                if (w->objectName() == "pluginBrowser") {
                    for (auto* b : w->findChildren<QPushButton*>())
                        if (b->text() == "Rescan")
                            b->click();
                    w->grab().save(
                        QString::fromStdString((out / "installed-plugins.png").string()));
                    qobject_cast<QDialog*>(w)->reject();
                }
        });
        showPlugins(e);
        auto pack = readSoundPack(root / "out/factory05", false);
        auto meta = std::find_if(pack.instruments.begin(), pack.instruments.end(),
                                 [](auto& i) { return i.name == "Meta Piano"; });
        check(meta != pack.instruments.end(), "offline factory Meta piano");
        SampleCache cache;
        auto piano = loadLibraryPreset(*meta, pack, cache);
        e.edit([&] {
            e.project.name = "Meta Piano Performance";
            e.project.tracks.clear();
            e.selectedTrack = -1;
            e.selectedClip = 0;
        });
        e.applyInstrument(piano, true);
        e.armTrack(0, true);
        e.findChild<QComboBox*>("countIn")->setCurrentIndex(0);
        e.toggleRecord();
        keys->setFocus();
        QTest::keyPress(keys, Qt::Key_A);
        QTest::keyPress(keys, Qt::Key_G);
        pump(350);
        float pianoPeak = e.engine.masterPeak();
        QTest::keyRelease(keys, Qt::Key_A);
        QTest::keyRelease(keys, Qt::Key_G);
        pump(100);
        e.stop();
        check(pianoPeak > 0 && !e.project.tracks.empty() && !e.project.tracks[0].clips.empty() &&
                  e.project.tracks[0].clips.back().notes.size() == 2,
              "offline Meta typing/MIDI capture");
        check(e.saveProject(
                  QString::fromStdString((out / "Meta Piano Performance.reverie").string())),
              "save Meta take");
        check(e.openProject(
                  QString::fromStdString((out / "Meta Piano Performance.reverie").string())),
              "reopen Meta take");
        check(
            e.exportProject(QString::fromStdString((out / "Meta Piano Performance.wav").string())),
            "Meta WAV");
        e.openPiano(e.project.tracks[0].clips.back().id);
        check(!e.findChild<QWidget*>("instrumentArtwork")->isVisible(),
              "compact browser must not reveal detail artwork");
        e.grab().save(QString::fromStdString((out / "meta-piano-recording.png").string()));

        // === 0.5.1 Stabilization & Korean Localization Regression Suite ===
        auto midiFixture = out / "import-fixture.mid";
        const unsigned char midiBytes[] = {'M',  'T',  'h', 'd',  0,    0,    0,    6,    0,
                                           0,    0,    1,   1,    0xe0, 'M',  'T',  'r',  'k',
                                           0,    0,    0,   13,   0,    0x90, 60,   100,  0x83,
                                           0x60, 0x80, 60,  0,    0,    0xff, 0x2f, 0};
        {
            std::ofstream mf(midiFixture, std::ios::binary);
            mf.write(reinterpret_cast<const char*>(midiBytes), sizeof midiBytes);
        }

        // Verify English compact SoundBrowser category list height at 1280x800 and 1440x900
        for (auto sz : {QSize(1280, 800), QSize(1440, 900)}) {
            e.resize(sz);
            QApplication::processEvents();
            auto* catList = browser->findChild<QListWidget*>("soundCategory");
            check(catList != nullptr && catList->count() >= 2, "English soundCategory exists");
            check(catList->height() >= 62 &&
                      catList->visualItemRect(catList->item(1)).bottom() <=
                          catList->viewport()->height(),
                  "English compact category row 2 is not clipped");
        }

        // Verify English MIDI import via Editor::importMidi
        auto tracksBeforeEn = e.project.tracks.size();
        e.importMidi(QString::fromStdString(midiFixture.string()));
        check(e.project.tracks.size() == tracksBeforeEn + 1 &&
                  !e.project.tracks.back().clips.empty() &&
                  e.project.tracks.back().clips[0].notes.size() == 1,
              "English MIDI import adds track and clip");

        // Load production Korean QM via loadUiLanguage()
        qputenv("REVERIE_TRANSLATIONS",
                QFileInfo(QString::fromUtf8(REVERIE_KO_QM)).absolutePath().toUtf8());
        QSettings().setValue("interface/language", "ko_KR");
        loadUiLanguage();
        check(QCoreApplication::translate("Ensemble", "Export Audio") ==
                  QString::fromUtf8("오디오 내보내기"),
              "production Korean QM loaded via loadUiLanguage");
        check(QCoreApplication::translate("Ensemble", "Projects support up to 64 tracks.") ==
                  QString::fromUtf8("프로젝트당 최대 64개의 트랙을 사용할 수 있습니다."),
              "updated 0.5.1 track limit string translated");
        check(translateEngineMessage("Cannot decode WAV: /tmp/bad.wav") ==
                      QString::fromUtf8("WAV 파일을 디코딩할 수 없습니다(파일 형식과 경로를 확인하세요): /tmp/bad.wav") &&
                  translateEngineMessage("Cannot create export file") ==
                      QString::fromUtf8("내보낼 파일을 생성할 수 없습니다. 폴더 권한과 경로를 확인하세요."),
              "translateEngineMessage translates engine error prefix and exact message");

        {
            Editor koEditor(false);
            for (auto sz : {QSize(1280, 800), QSize(1440, 900)}) {
                koEditor.resize(sz);
                koEditor.show();
                QApplication::processEvents();
                auto* koBrowser = koEditor.findChild<SoundBrowser*>("soundBrowser");
                auto* koCatList = koBrowser->findChild<QListWidget*>("soundCategory");
                check(koCatList != nullptr && koCatList->count() >= 2,
                      "Korean soundCategory exists");
                check(koCatList->height() >= 62 &&
                          koCatList->visualItemRect(koCatList->item(1)).bottom() <=
                              koCatList->viewport()->height(),
                      "Korean compact category row 2 is not clipped");
            }

            // Bug B: Korean MIDI import via actionImportMidi / importMidi
            auto* importAct = koEditor.findChild<QAction*>("actionImportMidi");
            check(importAct != nullptr &&
                      importAct->text() == QString::fromUtf8("MIDI 파일 가져오기…"),
                  "Korean actionImportMidi objectName and label");
            auto tracksBeforeKo = koEditor.project.tracks.size();
            koEditor.importMidi(QString::fromStdString(midiFixture.string()));
            check(koEditor.project.tracks.size() == tracksBeforeKo + 1 &&
                      !koEditor.project.tracks.back().clips.empty(),
                  "Korean MIDI import works without English text comparison");

            // Bug C: MixerView CSS stylesheet not corrupted by Korean translation
            MixerView koMixer(koEditor);
            koMixer.refresh();
            bool foundStripCss = false;
            for (auto* w : koMixer.findChildren<QWidget*>()) {
                if (w->styleSheet().contains("border-right:1px solid #777065;"))
                    foundStripCss = true;
            }
            check(foundStripCss, "MixerView CSS stylesheet preserved in Korean UI");

            // Bug A: ExportDialog cancel button identified by objectName("exportCancel") in Korean
            ExportDialog koExport(koEditor.project, 0, 4, &koEditor);
            check(koExport.windowTitle() == QString::fromUtf8("오디오 내보내기"),
                  "Korean ExportDialog title");
            auto* cancelBtn = koExport.findChild<QPushButton*>("exportCancel");
            check(cancelBtn != nullptr && cancelBtn->text() == QString::fromUtf8("취소") &&
                      cancelBtn->isEnabled(),
                  "Korean ExportDialog cancel button has objectName exportCancel");

            // Bug E & F: SettingsDialog version 0.5.1 and truthful restart notice
            SettingsDialog koSettings(koEditor);
            bool foundVersion051 = false;
            bool foundRestartNote = false;
            for (auto* lbl : koSettings.findChildren<QLabel*>()) {
                if (lbl->text().contains("0.5.1"))
                    foundVersion051 = true;
                if (lbl->text().contains(QString::fromUtf8("다시 시작하기 전에 프로젝트를 저장하세요")))
                    foundRestartNote = true;
            }
            check(foundVersion051 && foundRestartNote,
                  "Korean SettingsDialog shows 0.5.1 and truthful restart notice");
        }

        // Verify fallback to English when interface/language is set to "en"
        QSettings().setValue("interface/language", "en");
        loadUiLanguage();
        check(QCoreApplication::translate("Ensemble", "Export Audio") == "Export Audio",
              "English fallback restores source strings");

        shutdownPluginModules();
        std::cout << "PASS " << (native ? "native device" : "offscreen")
                  << " Surge XT Musical Typing -> 3-note MIDI clip -> parameter/state -> "
                     "save/reopen -> WAV; also offline Meta Piano MIDI/save/reopen/export; peak="
                  << peak << " metaPeak=" << pianoPeak << '\n';
        if (native)
            std::cout << e.audioDeviceReport() << '\n';
    } catch (const std::exception& ex) {
        std::cerr << "FAIL " << ex.what() << '\n';
        return 1;
    }
    return 0;
}
