# Translation keys

`Ensemble` is the stable context for application UI and Qt-free engine errors. Source English is the key. Prefer `QCoreApplication::translate("Ensemble", "...")` with explicit literals and placeholders. Do not concatenate translated fragments to build ordinary sentences. Error prefixes are a boundary exception: the translated prefix is followed by an unchanged technical path/identifier.

`translations/catalog_keys.cpp` and `engine_prefixes.h` are generated from dynamic category/form labels and core error literals by `scripts/refresh_translation_keys.py`. Edit source labels, not those generated files. Runtime category selection stores English IDs in `Qt::UserRole`; translating the visible category must not change filtering. Internal preset names from plugins/libraries, serialized keys, file paths, note/channel numbers and plugin identifiers are data, not translation keys.

The Korean TS contains source entries and unfinished translations for Argon. Do not bulk-generate Korean prose in parallel with that work. The test fixture is separate from the shipping catalog and must never be copied over it. Standard Qt dialog buttons use Qt's own contexts; application wording belongs in `Ensemble`.
