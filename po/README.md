# Translation Guide for krunner-bazaar

This document explains how to contribute translations to the krunner-bazaar KDE plugin.

## Overview

The krunner-bazaar plugin uses KDE's standard i18n (internationalization) framework. All user-visible strings are wrapped in `i18n()` calls and extracted into `.pot` and `.po` files for translation.

## Currently Translatable Strings

The plugin currently has the following user-visible strings:

1. **"Search for Flatpak applications in Bazaar"** - The syntax description shown in KRunner's help
2. **"Install %1"** - The action text shown for each search result (where %1 is the application name)

## Directory Structure

```
po/
├── plasma_runner_bazaarrunner.pot        # Template file (don't edit directly)
├── es/plasma_runner_bazaarrunner.po      # Spanish translation (reference example)
```

## Contributing a New Translation

### 1. Create a new language file

Copy the template file and rename it to your language code:

```bash
mkdir -p po/[LANG_CODE]
cp po/plasma_runner_bazaarrunner.pot po/[LANG_CODE]/plasma_runner_bazaarrunner.po
```

Where `[LANG_CODE]` is your language's ISO 639-1 code (e.g., `es` for Spanish, `fr` for French, `zh` for Chinese).

### 2. Edit the header

Update the header in your new `.po` file:

```po
"Language-Team: [LANGUAGE] <[EMAIL]>\n"
"Language: [LANG_CODE]\n"
"Last-Translator: [YOUR NAME] <[YOUR EMAIL]>\n"
"PO-Revision-Date: [CURRENT DATE]\n"
```

### 3. Translate the strings

Find each `msgid` and provide a translation in the corresponding `msgstr`:

```po
#: src/bazaarrunner.cpp:28
msgid "Search for Flatpak applications in Bazaar"
msgstr "[YOUR TRANSLATION]"

#: src/bazaarrunner.cpp:50
#, kde-format
msgid "Install %1"
msgstr "[YOUR TRANSLATION]"
```

**Important notes:**
- Keep the `%1` placeholder in the "Install %1" string - this will be replaced with the application name
- The `#, kde-format` comment indicates that KDE's string formatting is used

### 4. Test your translation

Build the project to test your translation:

```bash
just build
just install
```

Then change your system language and test the plugin in KRunner.

## Updating Translations

### Extract new translatable strings

If new translatable strings are added to the code, run:

```bash
./Messages.sh
```

This will update the `.pot` file with any new strings. This will also update existing language files by adding new strings and marking changed strings as "fuzzy" for review.
