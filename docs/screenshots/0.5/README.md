# 0.5 captures

Native Cocoa widgets at 1280×800 logical workspace (Retina PNGs): factory-browser (expanded browser region), plugin-midi-recording, meta-piano-recording (compact), installed-plugins, plugin-parameters, channel-processing and language-fixture. The Korean screenshot uses three QA strings, not a completed shipping translation.

`meta-piano-https-install-offscreen.png` is an actual offscreen Qt installer capture from the complete Meta Piano HTTPS/integrity/render test. It is labeled separately from native captures; later factory-inclusion labels differ.

No image here is a mockup. Instrument illustrations are retained 0.4 assets; missing specific artwork falls back to small family icons without a misleading large image. The browser count can include a separately scanned user plugin; factory inventory excludes it.

`release-bundle-starter.png` is the actual Release application launched from `/private/tmp` with the bundled factory component and no audio device opened. It confirms the two recorded starter instruments; the floating expanded browser is captured separately in `factory-browser.png`.

`ko/` contains runtime Korean UI QA captures rendered with `ensemble_ko_KR.qm` (`572/572` finished entries) across Main Window (`1440×900` and `1280×800`), File Menu, Expanded Instrument Browser, Sound Library Manager, Plugin Browser, Virtual Keyboard, MIDI Recording, Piano Roll, Mixer & Channel Processing, Export Audio, Settings (Audio and General/Language tabs), and Error Dialogs.
