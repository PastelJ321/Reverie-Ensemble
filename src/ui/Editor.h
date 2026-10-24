// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#pragma once
#include "core/Engine.h"
#include "core/Model.h"
#include "core/Recorder.h"
#include "platform/AudioDevice.h"
#include <QMainWindow>
#include <QSet>
#include <QTimer>
#include <functional>
class QLineEdit;
class QLabel;
class QDoubleSpinBox;
class QSpinBox;
class QComboBox;
class QSlider;
class QPushButton;
class QCheckBox;
class QScrollArea;
class QListWidget;
class QStackedWidget;
namespace reverie {
class AudioEditor;
class InspectorPanel;
class SoundBrowser;
class MixerView;
class Timeline;
class PianoRoll;
class Editor : public QMainWindow {
    friend class SettingsDialog;

  public:
    explicit Editor(bool audio = true);
    ~Editor() override;
    Project project;
    AudioEngine engine;
    int selectedTrack = 0;
    Id selectedClip = 0;
    bool snap = true;
    double grid = .25;
    double pixelsPerBeat = 64;
    int trackHeight = 82;
    bool followPlayhead = false;
    bool timeDisplaySeconds = false;
    void auditionNote(int key, float velocity = .7f, double duration = .3);
    void applyInstrument(const Preset&, bool newTrack = false);
    void showMixer();
    void showSettings();
    std::string audioDeviceReport() const { return device_.deviceReport(); }
    void showExport();
    void beginEdit();
    void endEdit();
    void edit(const std::function<void()>&);
    void refresh();
    void select(int track, Id clip = 0);
    void addTrack(TrackKind);
    void deleteSelection();
    void duplicateClip();
    void moveTrack(int delta);
    void undo();
    void redo();
    void play();
    void stop();
    void setTempo(double);
    void openPiano(Id);
    void importWav(const QString& path = {}, double beat = -1);
    bool saveProject(const QString& path = {});
    bool openProject(const QString&);
    bool exportProject(const QString&);
    Timeline* timeline() const {
        return timeline_;
    }
    PianoRoll* piano() const {
        return piano_;
    }
    double snapped(double beat) const {
        return snap ? std::round(beat / grid) * grid : beat;
    }
    QString currentPath() const {
        return currentPath_;
    }
    void message(const QString&);

  protected:
    void closeEvent(QCloseEvent*) override;

  private:
    QStackedWidget* bottomStack_ = nullptr;
    AudioEditor* audioEditor_ = nullptr;
    InspectorPanel* selectionInspector_ = nullptr;
    SoundBrowser* browser_ = nullptr;
    MixerView* mixer_ = nullptr;
    QStackedWidget* workspace_ = nullptr;
    AudioDeviceManager device_;
    AudioRecorder recorder_;
    std::vector<Project> undo_, redo_;
    bool transaction_ = false, refreshing_ = false, dirty_ = false, recording_ = false,
         audioEnabled_ = true;
    Id recordingTrack_ = 0;
    double recordingBeat_ = 0;
    std::filesystem::path recordingPath_;
    QString currentPath_;
    Timeline* timeline_ = nullptr;
    PianoRoll* piano_ = nullptr;
    QScrollArea* timelineScroll_ = nullptr;
    QScrollArea* pianoScroll_ = nullptr;
    QLabel* projectLabel_ = nullptr;
    QLineEdit* position_ = nullptr;
    QLabel* output_ = nullptr;
    QLabel* trackTitle_ = nullptr;
    QLabel* instrumentLabel_ = nullptr;
    QDoubleSpinBox* bpm_ = nullptr;
    QDoubleSpinBox* volume_ = nullptr;
    QDoubleSpinBox* pan_ = nullptr;
    QDoubleSpinBox* master_ = nullptr;
    QDoubleSpinBox *attack_ = nullptr, *decay_ = nullptr, *sustain_ = nullptr, *release_ = nullptr,
                   *cutoff_ = nullptr;
    QComboBox* wave_ = nullptr;
    QComboBox* preset_ = nullptr;
    QComboBox* inputs_ = nullptr;
    QCheckBox* mute_ = nullptr;
    QCheckBox* solo_ = nullptr;
    QPushButton* playButton_ = nullptr;
    QPushButton* recordButton_ = nullptr;
    QTimer timer_;
    void buildUi();
    void buildMenus();
    void tick();
    void toggleRecord();
    void finishRecord();
    void loadPreset();
    void savePreset();
    void loadSampler();
    bool confirmUnsaved();
    void newProject();
};
} // namespace reverie
