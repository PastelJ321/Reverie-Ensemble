#include <QCoreApplication>
// SPDX-License-Identifier: Apache-2.0
#include "ui/Editor.h"
#include "ui/ProductionControls.h"
#include <QtWidgets>
namespace reverie {
static QCheckBox* flag(QFormLayout* form, const char* name, bool value) {
    auto* w = new QCheckBox(QCoreApplication::translate("Ensemble", name));
    w->setChecked(value);
    form->addRow(w);
    return w;
}
static QDoubleSpinBox* number(QFormLayout* form, const char* name, double lo, double hi,
                              double value) {
    auto* w = new QDoubleSpinBox;
    w->setRange(lo, hi);
    w->setDecimals(3);
    w->setValue(value);
    form->addRow(QCoreApplication::translate("Ensemble", name), w);
    return w;
}
void showSynthControls(Editor& e) {
    if (e.selectedTrack < 0)
        return;
    auto preset = e.project.tracks[e.selectedTrack].preset;
    if (preset.kind != InstrumentKind::Synth)
        return;
    QDialog d(&e);
    d.setWindowTitle(QCoreApplication::translate("Ensemble", "Synth controls"));
    auto* f = new QFormLayout(&d);
    auto* modern = flag(f, "Extended synthesis", preset.modern);
    auto* mono = flag(f, "Monophonic", preset.mono);
    auto* legato = flag(f, "Legato", preset.legato);
    auto* mix = number(f, "Oscillator blend", 0, 1, preset.oscMix);
    auto* detune = number(f, "Detune (cents)", -100, 100, preset.detune);
    auto* glide = number(f, "Portamento (s)", 0, 4, preset.glide);
    auto* env = number(f, "Filter envelope (octaves)", 0, 8, preset.filterEnvelope);
    auto* rate = number(f, "LFO (Hz)", .01, 30, preset.lfoRate);
    auto* depth = number(f, "Vibrato (semitones)", 0, 12, preset.vibrato);
    auto* fm = number(f, "FM index", 0, 10, preset.fm);
    auto* b = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    f->addRow(b);
    QObject::connect(b, &QDialogButtonBox::accepted, &d, &QDialog::accept);
    QObject::connect(b, &QDialogButtonBox::rejected, &d, &QDialog::reject);
    if (d.exec() == QDialog::Accepted)
        e.edit([&] {
            auto& p = e.project.tracks[e.selectedTrack].preset;
            p.modern = modern->isChecked();
            p.mono = mono->isChecked();
            p.legato = legato->isChecked();
            p.oscMix = mix->value();
            p.detune = detune->value();
            p.glide = glide->value();
            p.filterEnvelope = env->value();
            p.lfoRate = rate->value();
            p.vibrato = depth->value();
            p.fm = fm->value();
        });
}
void showChannelProcessing(Editor& e, int track) {
    if (track < 0 || track >= int(e.project.tracks.size()))
        return;
    auto settings = e.project.tracks[track].effects;
    QDialog d(&e);
    d.setObjectName("channelProcessing");
    d.setWindowTitle(QCoreApplication::translate("Ensemble", "Channel processing"));
    auto* f = new QFormLayout(&d);
    auto* enabled = flag(f, "Enabled", settings.enabled);
    auto* comp = flag(f, "Compressor", settings.compressor);
    auto* threshold = number(f, "Threshold (dB)", -60, 0, settings.thresholdDb);
    auto* ratio = number(f, "Ratio", 1, 40, settings.ratio);
    auto* attack = number(f, "Attack (ms)", .1, 200, settings.attackMs);
    auto* release = number(f, "Release (ms)", 1, 3000, settings.releaseMs);
    auto* makeup = number(f, "Makeup (dB)", -24, 24, settings.makeupDb);
    auto* eq = flag(f, "Parametric EQ", settings.peq);
    auto* hz = number(f, "Frequency (Hz)", 20, 20000, settings.eqHz);
    auto* db = number(f, "Gain (dB)", -24, 24, settings.eqDb);
    auto* q = number(f, "Q", .1, 20, settings.eqQ);
    auto* limit = flag(f, "Peak limiter", settings.limiter);
    auto* input = number(f, "Input gain (dB)", -24, 24, settings.inputDb);
    auto* output = number(f, "Output gain (dB)", -24, 24, settings.outputDb);
    auto* b = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    f->addRow(b);
    QObject::connect(b, &QDialogButtonBox::accepted, &d, &QDialog::accept);
    QObject::connect(b, &QDialogButtonBox::rejected, &d, &QDialog::reject);
    if (d.exec() == QDialog::Accepted)
        e.edit([&] {
            auto& s = e.project.tracks[track].effects;
            s.enabled = enabled->isChecked();
            s.compressor = comp->isChecked();
            s.thresholdDb = threshold->value();
            s.ratio = ratio->value();
            s.attackMs = attack->value();
            s.releaseMs = release->value();
            s.makeupDb = makeup->value();
            s.peq = eq->isChecked();
            s.eqHz = hz->value();
            s.eqDb = db->value();
            s.eqQ = q->value();
            s.limiter = limit->isChecked();
            s.inputDb = input->value();
            s.outputDb = output->value();
        });
}
} // namespace reverie
