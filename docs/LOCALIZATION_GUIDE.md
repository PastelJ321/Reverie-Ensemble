# Localization handoff — 0.5

English is the source language. Argon owns Korean wording; Codex owns Qt integration and source call sites. The production Korean catalog is intentionally unfinished. The three Korean strings in `tests/fixtures/localization05.ts` are **test fixtures**, not a shipping translation.

## Files for Argon

- `translations/ensemble_ko_KR.ts`: translate `<translation type="unfinished">`, preserve source/context/locations, placeholders, accelerators and numerus forms.
- `docs/TRANSLATION_KEYS.md`: catalog conventions and technical identifiers.
- Documentation/copy changes may be reviewed in the same localization PR when explicitly in scope.

Work from the published `codex/ensemble-0.5` branch in a separately coordinated localization branch, for example `codex/ensemble-0.5-ko`. Do not edit C++ widget labels and the Korean TS simultaneously in unrelated branches. Request source changes through review. Keep this existing repository checkout; no new clone/worktree is required. Merge the implementation first or stack the localization PR onto it.

## Tools

```sh
python3 scripts/refresh_translation_keys.py
cmake --build build-05 --target update_translations
cmake --build build-05 --target reverie_translations
```

Qt LinguistTools (`lupdate`, `lrelease`) is required. `refresh_translation_keys.py` generates extraction declarations for dynamic form/category labels and Qt-free core errors. It does not write the TS file. `lupdate` preserves existing translations; review source changes and obsolete entries instead of replacing the whole catalog. Do not run a machine translator over the catalog as part of a code build.

QM files are compiled by CMake and copied to `Contents/Resources/translations/ensemble_ko_KR.qm`. A translator can test an external directory with `REVERIE_TRANSLATIONS=/absolute/path/to/qm-files`. Settings → General → Language supports English, 한국어 and Follow System Language. Language preference is saved immediately and takes effect after restarting; it does not reset the project, engine, plugin state or recorded notes. **Live retranslation of every existing widget is not implemented.** The restart requirement is shown in Settings.

The unfinished Korean catalog currently falls back to English. Do not call this a completed Korean localization. The independent QM fixture verifies QTranslator, UTF-8 Korean, layout and project/state preservation. Plugin-provided parameter/preset names and vendor GUIs belong to the plugin and are not rewritten.

## Review checks

Test 1280×800 and 1440×900, menus, expanded browser, Settings, Library Manager, Export and context menus. Preserve `%1`/`%2`, `\n`, `&` accelerators and file extensions. Keep sample/library/artist names, plugin IDs, JSON fields, paths, MIDI values, manufacturer names and license identifiers unchanged. Include a complete source fallback for unfinished entries. Technical filenames/error details remain verbatim after a translated error prefix.
