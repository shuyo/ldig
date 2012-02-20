ldig (Language Detection with Infinity Gram)
======================


This is a prototype of language detection for short message service (twitter).
with 99.1% accuracy for 17 languages


Usage
------

1. Extract model directory
    tar xf models/[select model archive]

2. Detect
    ldig.py -m [model directory] [text data file]


Data format
------

Each tweet is one line in text file as the below format.

  [label]\t[some metadata separated '\t']\t[text without '\t']

[label] is a language name alike en, de, fr and so on.
It is also optional as metadata.


Supported Languages
------

- cs	Czech
- da	Dannish
- de	German
- en	English
- es	Spanish
- fi	Finnish
- fr	French
- id	Indonesian
- it	Italian
- nl	Dutch
- no	Norwegian
- pl	Polish
- pt	Portuguese
- ro	Romanian
- sv	Swedish
- tr	Turkish
- vi	Vietnamese


Documents
------

- [Presentation in Japanese](http://www.slideshare.net/shuyo/gram-10286133)


Copyright & License
-----
- (c)2011 Nakatani Shuyo / Cybozu Labs Inc. All rights reserved.
- MIT License

