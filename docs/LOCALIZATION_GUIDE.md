# Localization guide & handoff — 0.5

English is the source language. Argon owns Korean wording, user guides, terminology, and editorial review; Codex owns Qt integration and C++ source call sites. In `content/ensemble-ko-0.5`, all 572 entries in `translations/ensemble_ko_KR.ts` are translated and verified with `lrelease`. The three Korean strings in `tests/fixtures/localization05.ts` remain isolated **test fixtures** for `reverie-ui05-tests`.

## Files for Argon

- `translations/ensemble_ko_KR.ts`: Korean translation catalog (`572 finished and 0 unfinished`), preserving source/context/locations, placeholders, accelerators, and prefix delimiters.
- `docs/TRANSLATION_KEYS.md`: catalog conventions and technical identifiers.
- `docs/TERMINOLOGY.md`: canonical English–Korean DAW, DSP, plugin, and instrument terminology.
- `docs/TRANSLATION_REVIEW.md`: editorial review, English source copy improvement proposals, missing C++ translation keys, and Korean UI QA results.
- `docs/USER_GUIDE.md` & `docs/USER_GUIDE.ko.md`: English and Korean user guides, onboarding, troubleshooting, and Sound Library instrument descriptions.

Work from the published `codex/ensemble-0.5` branch in a separately coordinated localization branch, for example `codex/ensemble-0.5-ko`. Do not edit C++ widget labels and the Korean TS simultaneously in unrelated branches. Request source changes through review. Keep this existing repository checkout; no new clone/worktree is required. Merge the implementation first or stack the localization PR onto it.

## Tools

```sh
python3 scripts/refresh_translation_keys.py
cmake --build build-05 --target update_translations
cmake --build build-05 --target reverie_translations
```

Qt LinguistTools (`lupdate`, `lrelease`) is required. `refresh_translation_keys.py` generates extraction declarations for dynamic form/category labels and Qt-free core errors. It does not write the TS file. `lupdate` preserves existing translations; review source changes and obsolete entries instead of replacing the whole catalog. Do not run a machine translator over the catalog as part of a code build.

QM files are compiled by CMake and copied to `Contents/Resources/translations/ensemble_ko_KR.qm`. A translator can test an external directory with `REVERIE_TRANSLATIONS=/absolute/path/to/qm-files`. Settings → General → Language supports English, 한국어 and Follow System Language. Language preference is saved immediately and takes effect after restarting; it does not reset the project, engine, plugin state or recorded notes. **Live retranslation of every existing widget is not implemented.** The restart requirement is shown in Settings.

The Korean catalog (`translations/ensemble_ko_KR.ts`) translates all 572 extracted strings in the `"Ensemble"` context; any future unextracted or unfinished strings automatically fall back to English. The independent QM fixture (`tests/fixtures/localization05.ts`) verifies QTranslator, UTF-8 Korean, layout and project/state preservation in automated tests. Plugin-provided parameter/preset names and vendor GUIs belong to the plugin and are not rewritten.

## Review checks

Test 1280×800 and 1440×900, menus, expanded browser, Settings, Library Manager, Export and context menus. Preserve `%1`/`%2`, `\n`, `&` accelerators and file extensions. Keep sample/library/artist names, plugin IDs, JSON fields, paths, MIDI values, manufacturer names and license identifiers unchanged. Include a complete source fallback for unfinished entries. Technical filenames/error details remain verbatim after a translated error prefix.
