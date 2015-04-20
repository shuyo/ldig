#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Testcase of ldig
# This code is available under the MIT License.
# (c)2012 Nakatani Shuyo / Cybozu Labs Inc.

import unittest
import ldig

class TestNormalization(unittest.TestCase):
    """Normalization test"""

    def setUp(self):
        pass

    def assertNormalize(self, org, norm):
        self.assertEqual(ldig.normalize_text(org), ("", norm, org))

    def testNormalizeRT(self):
        self.assertNormalize(u"RT RT RT RT RT I'm a Superwoman", u"I'm a superwoman")

    def testNormalizeLaugh(self):
        self.assertNormalize(u"ahahahah", u"ahahah")
        self.assertNormalize(u"hahha", u"haha")
        self.assertNormalize(u"hahaa", u"haha")
        self.assertNormalize(u"ahahahahhahahhahahaaaa", u"ahaha")

    def testLowerCaseWithTurkish(self):
        self.assertNormalize(u"I", u"I")
        self.assertNormalize(u"İ", u"i")
        self.assertNormalize(u"i", u"i")
        self.assertNormalize(u"ı", u"ı")

        self.assertNormalize(u"Iİ", u"Ii")
        self.assertNormalize(u"Iı", u"Iı")

if __name__ == '__main__':
    import sys, codecs
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout)
    unittest.main()

