#!/usr/bin/env python
# -*- coding: utf-8 -*-

# ldig : Language Detector with Infinite-Gram
# This code is available under the MIT License.
# (c)2011 Nakatani Shuyo / Cybozu Labs Inc.

import os, sys, re, codecs, json
import optparse
import numpy
import htmlentitydefs
import subprocess
import da
sys.stdout = codecs.getwriter('utf-8')(sys.stdout)


class ldig(object):
    def __init__(self, model_dir):
        self.features = os.path.join(model_dir, 'features')
        self.labels = os.path.join(model_dir, 'labels.json')
        self.param = os.path.join(model_dir, 'parameters.npy')
        self.doublearray = os.path.join(model_dir, 'doublearray.npz')

    def load_da(self):
        trie = da.DoubleArray()
        trie.load(self.doublearray)
        return trie

    def load_features(self):
        features = []
        with codecs.open(self.features, 'rb',  'utf-8') as f:
            pre_feature = ""
            for n, s in enumerate(f):
                m = re.match(r'(.+)\t([0-9]+)', s)
                if not m:
                    sys.exit("irregular feature : '%s' at %d" % (s, n + 1))
                if pre_feature >= m.groups(1):
                    sys.exit("unordered feature : '%s' at %d" % (s, n + 1))
                pre_feature = m.groups(1)
                features.append(m.groups())
        return features

    def load_labels(self):
        with open(self.labels, 'rb') as f:
            return json.load(f)


    def init(self, temp_path, corpus_list, lbff, ngram_bound):
        """
        Extract features from corpus and generate TRIE(DoubleArray) data
        - load corpus
        - generate temporary file for maxsubst
        - generate double array and save it
        - parameter: lbff = lower bound of feature frequency
        """

        labels = []
        with codecs.open(temp_path, 'wb', 'utf-8') as f:
            for file in corpus_list:
                with codecs.open(file, 'rb', 'utf-8') as g:
                    for i, s in enumerate(g):
                        label, text, org_text =  normalize_text(s)
                        if label is None or label == "":
                            sys.stderr.write("no label data at %d in %s \n" % (i+1, file))
                            continue
                        if label not in labels:
                            labels.append(label)
                        f.write(text)
                        f.write("\n")

        labels.sort()
        print "labels: %d" % len(labels)
        with open(self.labels, 'wb') as f:
            f.write(json.dumps(labels))

        print "generating max-substrings..."
        temp_features = self.features + ".temp"
        maxsubst = options.maxsubst
        if os.name == 'nt': maxsubst += ".exe"
        subprocess.call([maxsubst, temp_path, temp_features])

        # count features
        M = 0
        features = []
        r1 = re.compile(u'.\u0001.')
        r2 = re.compile(u'[A-Za-z\u00a1-\u00a3\u00bf-\u024f\u1e00-\u1eff]')
        with codecs.open(temp_features, 'rb', 'utf-8') as f:
            for line in f:
                i = line.index('\t')
                st = line[0:i]
                c = int(line[i+1:-1])
                if c >= lbff and len(st) <= ngram_bound and (not r1.search(st)) and r2.search(st) and (st[0] != u'\u0001' or st[-1] != u'\u0001'):
                    M += 1
                    features.append((st, line))
        print "# of features = %d" % M

        features.sort()
        with codecs.open(self.features, 'wb', 'utf-8') as f:
            for s in features:
                f.write(s[1])

        generate_doublearray(self.doublearray, [s[0] for s in features])

        numpy.save(self.param, numpy.zeros((M, len(labels))))

    def shrink(self):
        features = self.load_features()
        param = numpy.load(self.param)

        list = (numpy.abs(param).sum(1) > 0.0000001)
        new_param = param[list]
        print "# of features : %d => %d" % (param.shape[0], new_param.shape[0])

        numpy.save(self.param, new_param)
        new_features = []
        with codecs.open(self.features, 'wb',  'utf-8') as f:
            for i, x in enumerate(list):
                if x:
                    f.write("%s\t%s\n" % features[i])
                    new_features.append(features[i][0])

        generate_doublearray(self.doublearray, new_features)

    def debug(self, args):
        features = self.load_features()
        trie = self.load_da()
        labels = self.load_labels()
        param = numpy.load(self.param)

        for st in args:
            label, text, org_text = normalize_text(st)
            events = trie.extract_features(u"\u0001" + text + u"\u0001")
            print "orig: '%s'" % st
            print "norm: '%s'" % text
            sum = numpy.zeros(len(labels))
            print "id\tfeat\tfreq\t%s" % "\t".join(labels)
            for id in sorted(events, key=lambda id:features[id][0]):
                phi = param[id,]
                sum += phi * events[id]
                print "%d\t%s\t%d\t%s" % (id,features[id][0], events[id], "\t".join(["%0.2f" % x for x in phi]))
            exp_w = numpy.exp(sum - sum.max())
            prob = exp_w / exp_w.sum()
            print "\t\t\t%s" % "\t".join(["%0.2f" % x for x in sum])
            print "\t\t\t%s" % "\t".join(["%0.1f%%" % (x * 100) for x in prob])

    def learn(self, options, args):
        trie = self.load_da()
        param = numpy.load(self.param)
        labels = self.load_labels()

        import time
        print "loading corpus... " + time.strftime("%H:%M:%S", time.localtime())
        corpus, idlist = load_corpus(args, labels)
        print "inference... " + time.strftime("%H:%M:%S", time.localtime())
        inference(param, labels, corpus, idlist, trie, options)
        print "finish... " + time.strftime("%H:%M:%S", time.localtime())
        numpy.save(self.param, param)

    def detect(self, options, args):
        trie = self.load_da()
        param = numpy.load(self.param)
        labels = self.load_labels()

        log_likely = likelihood(param, labels, trie, args, options)




# from http://www.programming-magic.com/20080820002254/
reference_regex = re.compile(u'&(#x?[0-9a-f]+|[a-z]+);', re.IGNORECASE)
num16_regex = re.compile(u'#x\d+', re.IGNORECASE)
num10_regex = re.compile(u'#\d+', re.IGNORECASE)
def htmlentity2unicode(text):
    result = u''
    i = 0
    while True:
        match = reference_regex.search(text, i)
        if match is None:
            result += text[i:]
            break

        result += text[i:match.start()]
        i = match.end()
        name = match.group(1)

        if name in htmlentitydefs.name2codepoint.keys():
            result += unichr(htmlentitydefs.name2codepoint[name])
        elif num16_regex.match(name):
            result += unichr(int(u'0'+name[1:], 16))
        elif num10_regex.match(name):
            result += unichr(int(name[1:]))
    return result


def normalize_twitter(text):
    """normalization for twitter"""
    text = re.sub(r'(@|#|https?:\/\/)[^ ]+', '', text)
    text = re.sub(r'(^| )[:;][\(\)DOPop]($| )', ' ', text)  # facemark
    text = re.sub(r'(^| )RT[ :]', ' ', text)
    text = re.sub(r'([hj][aieo])\1{2,}', r'\1\1', text, re.IGNORECASE)  # laugh
    text = re.sub(r' via *$', '', text)
    return text


re_ignore_i = re.compile(r'[^I]')
vietnamese_norm = {
	u'\u0041\u0300':u'\u00C0', u'\u0045\u0300':u'\u00C8', u'\u0049\u0300':u'\u00CC', u'\u004F\u0300':u'\u00D2', 
	u'\u0055\u0300':u'\u00D9', u'\u0059\u0300':u'\u1EF2', u'\u0061\u0300':u'\u00E0', u'\u0065\u0300':u'\u00E8', 
	u'\u0069\u0300':u'\u00EC', u'\u006F\u0300':u'\u00F2', u'\u0075\u0300':u'\u00F9', u'\u0079\u0300':u'\u1EF3', 
	u'\u00C2\u0300':u'\u1EA6', u'\u00CA\u0300':u'\u1EC0', u'\u00D4\u0300':u'\u1ED2', u'\u00E2\u0300':u'\u1EA7', 
	u'\u00EA\u0300':u'\u1EC1', u'\u00F4\u0300':u'\u1ED3', u'\u0102\u0300':u'\u1EB0', u'\u0103\u0300':u'\u1EB1', 
	u'\u01A0\u0300':u'\u1EDC', u'\u01A1\u0300':u'\u1EDD', u'\u01AF\u0300':u'\u1EEA', u'\u01B0\u0300':u'\u1EEB', 

	u'\u0041\u0301':u'\u00C1', u'\u0045\u0301':u'\u00C9', u'\u0049\u0301':u'\u00CD', u'\u004F\u0301':u'\u00D3', 
	u'\u0055\u0301':u'\u00DA', u'\u0059\u0301':u'\u00DD', u'\u0061\u0301':u'\u00E1', u'\u0065\u0301':u'\u00E9', 
	u'\u0069\u0301':u'\u00ED', u'\u006F\u0301':u'\u00F3', u'\u0075\u0301':u'\u00FA', u'\u0079\u0301':u'\u00FD', 
	u'\u00C2\u0301':u'\u1EA4', u'\u00CA\u0301':u'\u1EBE', u'\u00D4\u0301':u'\u1ED0', u'\u00E2\u0301':u'\u1EA5', 
	u'\u00EA\u0301':u'\u1EBF', u'\u00F4\u0301':u'\u1ED1', u'\u0102\u0301':u'\u1EAE', u'\u0103\u0301':u'\u1EAF', 
	u'\u01A0\u0301':u'\u1EDA', u'\u01A1\u0301':u'\u1EDB', u'\u01AF\u0301':u'\u1EE8', u'\u01B0\u0301':u'\u1EE9', 

	u'\u0041\u0303':u'\u00C3', u'\u0045\u0303':u'\u1EBC', u'\u0049\u0303':u'\u0128', u'\u004F\u0303':u'\u00D5', 
	u'\u0055\u0303':u'\u0168', u'\u0059\u0303':u'\u1EF8', u'\u0061\u0303':u'\u00E3', u'\u0065\u0303':u'\u1EBD', 
	u'\u0069\u0303':u'\u0129', u'\u006F\u0303':u'\u00F5', u'\u0075\u0303':u'\u0169', u'\u0079\u0303':u'\u1EF9', 
	u'\u00C2\u0303':u'\u1EAA', u'\u00CA\u0303':u'\u1EC4', u'\u00D4\u0303':u'\u1ED6', u'\u00E2\u0303':u'\u1EAB', 
	u'\u00EA\u0303':u'\u1EC5', u'\u00F4\u0303':u'\u1ED7', u'\u0102\u0303':u'\u1EB4', u'\u0103\u0303':u'\u1EB5', 
	u'\u01A0\u0303':u'\u1EE0', u'\u01A1\u0303':u'\u1EE1', u'\u01AF\u0303':u'\u1EEE', u'\u01B0\u0303':u'\u1EEF', 

	u'\u0041\u0309':u'\u1EA2', u'\u0045\u0309':u'\u1EBA', u'\u0049\u0309':u'\u1EC8', u'\u004F\u0309':u'\u1ECE', 
	u'\u0055\u0309':u'\u1EE6', u'\u0059\u0309':u'\u1EF6', u'\u0061\u0309':u'\u1EA3', u'\u0065\u0309':u'\u1EBB', 
	u'\u0069\u0309':u'\u1EC9', u'\u006F\u0309':u'\u1ECF', u'\u0075\u0309':u'\u1EE7', u'\u0079\u0309':u'\u1EF7', 
	u'\u00C2\u0309':u'\u1EA8', u'\u00CA\u0309':u'\u1EC2', u'\u00D4\u0309':u'\u1ED4', u'\u00E2\u0309':u'\u1EA9', 
	u'\u00EA\u0309':u'\u1EC3', u'\u00F4\u0309':u'\u1ED5', u'\u0102\u0309':u'\u1EB2', u'\u0103\u0309':u'\u1EB3', 
	u'\u01A0\u0309':u'\u1EDE', u'\u01A1\u0309':u'\u1EDF', u'\u01AF\u0309':u'\u1EEC', u'\u01B0\u0309':u'\u1EED', 

	u'\u0041\u0323':u'\u1EA0', u'\u0045\u0323':u'\u1EB8', u'\u0049\u0323':u'\u1ECA', u'\u004F\u0323':u'\u1ECC', 
	u'\u0055\u0323':u'\u1EE4', u'\u0059\u0323':u'\u1EF4', u'\u0061\u0323':u'\u1EA1', u'\u0065\u0323':u'\u1EB9', 
	u'\u0069\u0323':u'\u1ECB', u'\u006F\u0323':u'\u1ECD', u'\u0075\u0323':u'\u1EE5', u'\u0079\u0323':u'\u1EF5', 
	u'\u00C2\u0323':u'\u1EAC', u'\u00CA\u0323':u'\u1EC6', u'\u00D4\u0323':u'\u1ED8', u'\u00E2\u0323':u'\u1EAD', 
	u'\u00EA\u0323':u'\u1EC7', u'\u00F4\u0323':u'\u1ED9', u'\u0102\u0323':u'\u1EB6', u'\u0103\u0323':u'\u1EB7', 
	u'\u01A0\u0323':u'\u1EE2', u'\u01A1\u0323':u'\u1EE3', u'\u01AF\u0323':u'\u1EF0', u'\u01B0\u0323':u'\u1EF1', 
}
re_vietnamese = re.compile(u'[AEIOUYaeiouy\u00C2\u00CA\u00D4\u00E2\u00EA\u00F4\u0102\u0103\u01A0\u01A1\u01AF\u01B0][\u0300\u0301\u0303\u0309\u0323]')
re_latin_cont = re.compile(u'([a-z\u00e0-\u00ff])\\1{2,}')
re_symbol_cont = re.compile(u'([^a-z\u00e0-\u00ff])\\1{1,}')
def normalize_text(org):
    m = re.match(r'([-A-Za-z]+)\t(.+)', org)
    if m:
        label, org = m.groups()
    else:
        label = ""
    s = htmlentity2unicode(org)
    s = re.sub(u'[\u2010-\u2015]', '-', s)
    s = re.sub(u'[0-9]+', '0', s)
    s = re.sub(u'[^\u0020-\u007e\u00a1-\u024f\u0300-\u036f\u1e00-\u1eff]+', ' ', s)
    s = re.sub(u'  +', ' ', s).strip()

    s = normalize_twitter(s)

    s = re_vietnamese.sub(lambda x:vietnamese_norm[x.group(0)], s)
    s = re_ignore_i.sub(lambda x:x.group(0).lower(), s)
    s = s.replace(u'\u0219', u'\u015f').replace(u'\u021b', u'\u0163')
    s = re_latin_cont.sub(r'\1\1', s)
    s = re_symbol_cont.sub(r'\1', s)
    return label, s, org


# load courpus
def load_corpus(filelist, labels):
    idlist = dict((x, []) for x in labels)
    corpus = []
    for filename in filelist:
        f = codecs.open(filename, 'rb',  'utf-8')
        for i, s in enumerate(f):
            label, text, org_text = normalize_text(s)
            if label not in labels:
                sys.exit("unknown label '%s' at %d in %s " % (label, i+1, filename))
            idlist[label].append(len(corpus))
            corpus.append((label, text, org_text))
        f.close()
    return corpus, idlist


def shuffle(idlist):
    n = max(len(idlist[lang]) for lang in idlist)
    list = []
    for lang in idlist:
        text_ids = idlist[lang]
        n_text = len(text_ids)
        list += text_ids * (n / n_text)
        numpy.random.shuffle(text_ids)
        list += text_ids[:n % n_text]
    numpy.random.shuffle(list)
    return list



# prediction probability
def predict(param, events):
    sum_w = numpy.dot(param[events.keys(),].T, events.values())
    exp_w = numpy.exp(sum_w - sum_w.max())
    return exp_w / exp_w.sum()


# inference and learning
def inference(param, labels, corpus, idlist, trie, options):
    K = len(labels)
    M = param.shape[0]

    list = shuffle(idlist)
    N = len(list)
    WHOLE_REG_INT = (N / options.n_whole_reg) + 1

    # learning rate
    eta = options.eta
    if options.reg_const:
        penalties = numpy.zeros_like(param)
        alpha = pow(0.9, -1.0 / N)
        uk = 0

    corrects = numpy.zeros(K, dtype=int)
    counts = numpy.zeros(K, dtype=int)
    for m, target in enumerate(list):
        label, text, org_text = corpus[target]
        events = trie.extract_features(u"\u0001" + text + u"\u0001")
        label_k = labels.index(label)

        y = predict(param, events)
        predict_k = y.argmax()
        counts[label_k] += 1
        if label_k == predict_k:
            corrects[label_k] += 1

        # learning
        if options.reg_const:
            eta *= alpha
            uk += options.reg_const * eta / N
        y[label_k] -= 1
        y *= eta

        if options.reg_const:
            indexes = events
            if (N - m) % WHOLE_REG_INT == 1:
                print "full regularization: %d / %d" % (m, N)
                indexes = xrange(M)
            for id in indexes:
                if id in events: param[id,] -= y * events[id]

                for j in xrange(K):
                    w = param[id, j]
                    if w > 0:
                        w1 = w - uk - penalties[id, j]
                        if w1 > 0:
                            param[id, j] = w1
                            penalties[id, j] += w1 - w
                        else:
                            param[id, j] = 0
                            penalties[id, j] -= w
                    elif w < 0:
                        w1 = w + uk - penalties[id, j]
                        if w1 < 0:
                            param[id, j] = w1
                            penalties[id, j] += w1 - w
                        else:
                            param[id, j] = 0
                            penalties[id, j] -= w
        else:
            for id, freq in events.iteritems():
                param[id,] -= y * freq

    for lbl, crct, cnt in zip(labels, corrects, counts):
        if cnt > 0:
            print ">    %s = %d / %d = %.2f" % (lbl, crct, cnt, 100.0 * crct / cnt)
    print "> total = %d / %d = %.2f" % (corrects.sum(), N, 100.0 * corrects.sum() / N)
    list = (numpy.abs(param).sum(1) > 0.0000001)
    print "> # of relevant features = %d / %d" % (list.sum(), M)


def likelihood(param, labels, trie, filelist, options):
    K = len(labels)
    corrects = numpy.zeros(K, dtype=int)
    counts = numpy.zeros(K, dtype=int)

    label_map = dict()
    for i, label in enumerate(labels):
        label_map[label] = i

    n_available_data = 0
    log_likely = 0.0
    for filename in filelist:
        f = codecs.open(filename, 'rb',  'utf-8')
        for i, s in enumerate(f):
            label, text, org_text = normalize_text(s)

            if label not in label_map:
                sys.stderr.write("WARNING : unknown label '%s' at %d in %s (ignore the later same labels)\n" % (label, i+1, filename))
                label_map[label] = -1
            label_k = label_map[label]

            events = trie.extract_features(u"\u0001" + text + u"\u0001")
            y = predict(param, events)
            predict_k = y.argmax()

            if label_k >= 0:
                log_likely -= numpy.log(y[label_k])
                n_available_data += 1
                counts[label_k] += 1
                if label_k == predict_k:
                    corrects[predict_k] += 1

            predict_lang = labels[predict_k]
            if y[predict_k] < 0.6: predict_lang = ""
            print "%s\t%s\t%s" % (label, predict_lang, org_text)
        f.close()

    if n_available_data > 0:
        log_likely /= n_available_data

        for lbl, crct, cnt in zip(labels, corrects, counts):
            if cnt > 0:
                print ">    %s = %d / %d = %.2f" % (lbl, crct, cnt, 100.0 * crct / cnt)
        print "> total = %d / %d = %.2f" % (corrects.sum(), n_available_data, 100.0 * corrects.sum() / n_available_data)
        print "> average negative log likelihood = %.3f" % log_likely

    return log_likely


def generate_doublearray(file, features):
    trie = da.DoubleArray()
    trie.initialize(features)
    trie.save(file)




if __name__ == '__main__':
    parser = optparse.OptionParser()
    parser.add_option("-m", dest="model", help="model directory")
    parser.add_option("--init", dest="init", help="initialize model", action="store_true")
    parser.add_option("--learning", dest="learning", help="learn model", action="store_true")
    parser.add_option("--shrink", dest="shrink", help="remove irrevant features", action="store_true")
    parser.add_option("--debug", dest="debug", help="detect command line text for debug", action="store_true")

    # for initialization
    parser.add_option("--ff", dest="bound_feature_freq", help="threshold of feature frequency (for initialization)", type="int", default=8)
    parser.add_option("-n", dest="ngram_bound", help="n-gram upper bound (for initialization)", type="int", default=99999)
    parser.add_option("-x", dest="maxsubst", help="max substring extractor", default="./maxsubst")

    # for learning
    parser.add_option("-e", "--eta", dest="eta", help="learning rate", type="float", default=0.1)
    parser.add_option("-r", "--regularity", dest="reg_const", help="regularization constant", type="float")
    parser.add_option("--wr", dest="n_whole_reg", help="number of whole regularizations", type="int", default=2)

    (options, args) = parser.parse_args()
    if not options.model: parser.error("need model directory (-m)")


    detector = ldig(options.model)
    if options.init:
        if not os.path.exists(options.model):
            os.mkdir(options.model)
        if len(args) == 0:
            parser.error("need corpus")
    else:
        if not os.path.exists(detector.features):
            parser.error("features file doesn't exist")
        if not os.path.exists(detector.labels):
            parser.error("labels file doesn't exist")
        if not os.path.exists(detector.param):
            parser.error("parameters file doesn't exist")


    if options.init:
        temp_path = os.path.join(options.model, 'temp')
        detector.init(temp_path, args, options.bound_feature_freq, options.ngram_bound)

    elif options.debug:
        detector.debug(args)

    elif options.shrink:
        detector.shrink()

    elif options.learning:
        detector.learn(options, args)

    else:
        detector.detect(options, args)

