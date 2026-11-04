# Localization guide & handoff — 0.5.1

English is the source language. In `release/ensemble-0.5.1`, all 636 entries in `translations/ensemble_ko_KR.ts` are translated and verified with `lrelease`, including all previously unextracted UI labels, status strings, and runtime engine/plugin exception messages (`translateEngineMessage()`). The three Korean strings in `tests/fixtures/localization05.ts` remain isolated **test fixtures** for `reverie-ui05-tests`.

## Key Localization Files

- `translations/ensemble_ko_KR.ts`: Korean translation catalog (`636 finished and 0 unfinished`), preserving source/context/locations, placeholders, accelerators, and prefix delimiters.
- `translations/catalog_keys.cpp` & `translations/engine_prefixes.h`: generated extraction declarations for dynamic form/category labels and Qt-free core/platform/UI error strings.
- `docs/TRANSLATION_KEYS.md`: catalog conventions and technical identifiers.
- `docs/TERMINOLOGY.md`: canonical English–Korean DAW, DSP, plugin, and instrument terminology.
- `docs/TRANSLATION_REVIEW.md`: editorial review, resolved C++ localization bugs, and Korean UI QA results.
- `docs/USER_GUIDE.md` & `docs/USER_GUIDE.ko.md`: English and Korean user guides, onboarding, troubleshooting, and Sound Library instrument descriptions.

## Tools

```sh
python3 scripts/refresh_translation_keys.py
cmake --build build-release --target update_translations
cmake --build build-release --target reverie_translations
```

Qt LinguistTools (`lupdate`, `lrelease`) is required. `refresh_translation_keys.py` generates extraction declarations for dynamic form/category labels and Qt-free core/UI errors. It does not write the TS file. `lupdate` preserves existing translations; review source changes and obsolete entries instead of replacing the whole catalog. Do not run a machine translator over the catalog as part of a code build.

QM files are compiled by CMake and copied to `Contents/Resources/translations/ensemble_ko_KR.qm`. A translator can test an external directory with `REVERIE_TRANSLATIONS=/absolute/path/to/qm-files`. Settings → General → Language supports English, 한국어 and Follow System Language. Language preference is saved immediately and takes effect after restarting; it does not reset the project, engine, plugin state or recorded notes. **Live retranslation of every existing widget is not implemented.** The restart requirement is shown in Settings (`Language changes apply after restarting Ensemble. Save your project before restarting.`).

The Korean catalog (`translations/ensemble_ko_KR.ts`) translates all 636 extracted strings in the `"Ensemble"` context; any future unextracted or unfinished strings automatically fall back to English. Both the independent QM fixture (`tests/fixtures/localization05.ts`) and the production Korean QM (`ensemble_ko_KR.qm`) are verified in `reverie-ui05-tests`. Plugin-provided parameter/preset names and vendor GUIs belong to the plugin and are not rewritten.

## Review checks

Test 1280×800 and 1440×900, menus, expanded browser, Settings, Library Manager, Export and context menus. Preserve `%1`/`%2`, `\n`, `&` accelerators and file extensions. Keep sample/library/artist names, plugin IDs, JSON fields, paths, MIDI values, manufacturer names and license identifiers unchanged. Include a complete source fallback for unfinished entries. Technical filenames/error details remain verbatim after a translated error prefix.
