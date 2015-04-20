import numpy as np
from itertools import izip
from operator import itemgetter
import ldig


class LdigDetector(object):
    """Standalone detector, based on `server.py:Detector`.

    If your text is already normalized, it might be slightly faster to
    initialize with `normalize=False` prior to use the `detect` method, or just
    use the `_detect_normalize` or `_detect` methods directly.

    """
    def __init__(self, modeldir, normalize=True):
        self.ldig = ldig.ldig(modeldir)
        self.features = self.ldig.load_features()
        self.trie = self.ldig.load_da()
        self.labels = self.ldig.load_labels()
        self.param = np.load(self.ldig.param)
        if normalize:
            self.detect = self._detect_normalize
        else:
            self.detect = self._detect

    def _detect_normalize(self, text):
        _, text, _ = ldig.normalize_text(text)
        return self._detect(text)

    def _detect(self, text):
        events = self.trie.extract_features(u"\u0001" + text + u"\u0001")
        _sum = np.zeros(len(self.labels))

        for id in sorted(events, key=lambda id: self.features[id][0]):
            phi = self.param[id, ]
            _sum += phi * events[id]
        exp_w = np.exp(_sum - _sum.max())
        prob = exp_w / exp_w.sum()

        r = sorted(izip(self.labels, prob), key=itemgetter(1), reverse=True)
        return r
