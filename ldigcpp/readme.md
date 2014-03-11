ldigcpp (Language Detection with Infinity Gram for C++)
======================

This is a prototype of language detection for short message service (twitter).
with about 99% accuracy for 50 languages


Build
------

1. Clone ldig project and checkout cpp branch

  ```sh
  git clone https://github.com/shuyo/ldig.git
  cd ldig
  git checkout cpp
  cd cpp
  ```

2. Clone cybozulib

  ```sh
  git clone https://github.com/herumi/cybozulib.git
  ```

3. Make

  ```sh
  cmake .
  make
  ```


Usage
------

1. Extract model archive
    xz -dk lang50.x64.model.xz

2. Detect
    ldig.py -m lang50.x64.model [text data file]


Data format
------

See https://github.com/shuyo/ldig/tree/cpp#data-format



Supported Languages
------

In lang50.x64.model

- ar	Arabic
- ar-bz	Arabizi (Arabic chat alphabet)
- bg	Bulgarian
- bn	Bengali
- ca	Catalan
- cs	Czech
- da	Dannish
- de	German
- dv	Maldivian (Dhivehi)
- el	Greek
- en	English
- es	Spanish
- et	Estonian
- fa	Persian (Farsi)
- fi	Finnish
- fr	French
- gu	Gujarati
- he	Hebrew
- hi	Hindi
- hr	Croatian (Hrvatski)
- hu	Hungrian
- id	Indonesian
- it	Italian
- ja	Japanese
- ko	Korean
- lt	Lithuanian
- lv	Latvian
- mk	Macedonian
- ml	Malayalam
- mn	Mongolian
- nl	Dutch
- no	Norwegian
- pa	Punjabi
- pl	Polish
- pt	Portuguese
- ro	Romanian
- ru	Russian
- si	Sinhala
- sq	Albanian
- sv	Swedish
- ta	Tamil
- te	Telugu
- th	Thai
- tl	Tagalog
- tr	Turkish
- uk	Ukrainian
- ur	Urdu
- vi	Vietnamese
- zh-cn	Simplified Chinese
- zh-tw	Traditional Chinese



Copyright & License
-----
- (c)2013-2014 Nakatani Shuyo / Cybozu Labs Inc. All rights reserved.
- All codes and resources are available under the MIT License.

