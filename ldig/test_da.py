#!/usr/bin/env python
# -*- coding: utf-8 -*-

import unittest
import da

class TestDoubleArray(unittest.TestCase):
    def test1(self):
        trie = da.DoubleArray(verbose=False)
        trie.initialize(["cat"])
        self.assertEqual(trie.N, 4)
        self.assert_(trie.get("ca") is None)
        self.assert_(trie.get("xxx") is None)
        self.assertEqual(trie.get("cat"), 0)

    def test2(self):
        trie = da.DoubleArray()
        trie.initialize(["cat", "dog"])
        self.assertEqual(trie.N, 7)
        self.assert_(trie.get("ca") is None)
        self.assert_(trie.get("xxx") is None)
        self.assertEqual(trie.get("cat"), 0)
        self.assertEqual(trie.get("dog"), 1)

    def test3(self):
        trie = da.DoubleArray(verbose=False)
        trie.initialize(["ca", "cat", "deer", "dog", "fox", "rat"])
        print trie.base
        print trie.check
        print trie.value
        self.assertEqual(trie.N, 17)
        self.assert_(trie.get("c") is None)
        self.assertEqual(trie.get("ca"), 0)
        self.assertEqual(trie.get("cat"), 1)
        self.assertEqual(trie.get("deer"), 2)
        self.assertEqual(trie.get("dog"), 3)
        self.assert_(trie.get("xxx") is None)

    def test4(self):
        trie = da.DoubleArray()
        self.assertRaises(Exception, trie.initialize, ["cat", "ant"])

    def test5(self):
        trie = da.DoubleArray(verbose=False)
        trie.initialize(["ca", "cat", "deer", "dog", "fox", "rat"])

        r = trie.extract_features("")
        self.assertEqual(len(r), 0)

        r = trie.extract_features("cat")
        self.assertEqual(len(r), 2)
        self.assertEqual(r[0], 1)
        self.assertEqual(r[1], 1)

        r = trie.extract_features("deerat")
        self.assertEqual(len(r), 2)
        self.assertEqual(r[2], 1)
        self.assertEqual(r[5], 1)

unittest.main()

