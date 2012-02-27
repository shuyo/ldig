#!/usr/bin/env python
# -*- coding: utf-8 -*-

# ldig server
# This code is available under the MIT License.
# (c)2011 Nakatani Shuyo / Cybozu Labs Inc.

import os
import BaseHTTPServer
import urlparse
import optparse
import json
import numpy
import ldig

parser = optparse.OptionParser()
parser.add_option("-m", dest="model", help="model directory")
(options, args) = parser.parse_args()
if not options.model: parser.error("need model directory (-m)")


class Detector(object):
    def __init__(self, modeldir):
        self.ldig = ldig.ldig(modeldir)
        self.features = self.ldig.load_features()
        self.trie = self.ldig.load_da()
        self.labels = self.ldig.load_labels()
        self.param = numpy.load(self.ldig.param)

    def detect(self, st):
        label, text, org_text = ldig.normalize_text(st)
        events = self.trie.extract_features(u"\u0001" + text + u"\u0001")
        sum = numpy.zeros(len(self.labels))

        data = []
        for id in sorted(events, key=lambda id:self.features[id][0]):
            phi = self.param[id,]
            sum += phi * events[id]
            data.append({"id":id, "feature":self.features[id][0], "phi":["%0.3f" % x for x in phi]})
        exp_w = numpy.exp(sum - sum.max())
        prob = exp_w / exp_w.sum()
        return {"labels":self.labels, "data":data, "prob":["%0.3f" % x for x in prob]}

basedir = os.path.join(os.path.dirname(__file__), "static")
detector = Detector(options.model)

class LdigServerHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_GET(self):
        url = urlparse.urlparse(self.path)
        path = url.path
        if path.endswith("/"): path += "index.html"
        localpath = basedir + path
        if path == "/detect":
            params = urlparse.parse_qs(url.query)
            json.dump(detector.detect(params['text'][0]), self.wfile)
        elif os.path.exists(localpath):
            self.send_response(200)
            if path.endswith(".html"):
                self.send_header("Content-Type", "text/html; charset=utf-8")
            elif path.endswith(".js"):
                self.send_header("Content-Type", "text/javascript; charset=utf-8")
            self.end_headers()
            with open(localpath, "rb") as f:
                self.wfile.write(f.read())
        else:
            self.send_response(404, "Not Found : " + url.path)
            self.send_header("Expires", "Fri, 31 Dec 2100 00:00:00 GMT")
            self.end_headers()

server = BaseHTTPServer.HTTPServer(('', 48000), LdigServerHandler)
print "ready."
server.serve_forever()
