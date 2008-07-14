#
# python version of sampler data structure
#
# does not deal with precision issues.
# also, is too slow.
#

from math import log, ceil, floor
import random

class increasing_sampler:
    class sampler_item:
        def __init__(self, index=None, weight=1):
            assert(weight >= 1)
            self.index = index
            self.weight = weight
            self.bucket_index = self._bucket_index()

        def _bucket_index(self):
            return int(ceil(log(self.weight,2)))

        def _max_weight(self):
            return 2**self._bucket_index()

        def raise_weight(self, new_weight):
            assert new_weight >= self.weight
            old_bucket_index = self.bucket_index
            self.weight = new_weight
            self.bucket_index = self._bucket_index()

        def __str__(self):
            return "(i=%d,w=%g,b=%d)" % (self.index, self.weight, self.bucket_index)
            
    class bucket:
        def __init__(self, max_weight):
            self.items = []
            self.max_weight = max_weight

        def _total_max_weight(self):
            return len(self.items)*self.max_weight

        def append(self, item):
            self.items.append(item)

        def remove(self, item):
            self.items.remove(item)

        def sample(self):
            assert self.items
            item = random.sample(self.items, 1)[0]
            if item.weight >= random.random() * self.max_weight:
                return item
            else:
                return None

        def __str__(self):
            return "\n".join(["<bucket total_max_weight=%g>" % self._total_max_weight()]
                             + [str(i) for i in self.items]
                             + ["</bucket>"])

    def __init__(self):
        self.items = []
        self.buckets = {}
        self.total_max_weight = 0
        self.max_bucket = -1

    def __str__(self):
        return "\n".join(["<increasing_sampler total_max_weight=%g>" % self.total_max_weight]
                         + ["%d:%s" % (i, str(self.buckets[i])) for i in self.buckets]
                         + ["</sampler>"])

    def get_weight(self, index):
        return self.items[index].weight

    def raise_weight(self, index, new_weight):
        item = self.items[index]
        old_bucket_index = item.bucket_index
        item.raise_weight(new_weight)
        if old_bucket_index != item.bucket_index:
            self.buckets[old_bucket_index].remove(item)
            if not (item.bucket_index in self.buckets):
                self.buckets[item.bucket_index] = self.bucket(item._max_weight())
            self.buckets[item.bucket_index].append(item)
            self.max_bucket = max(self.max_bucket, item.bucket_index)
            self.total_max_weight += (self.buckets[item.bucket_index].max_weight
                                      - self.buckets[old_bucket_index].max_weight)

    def append(self, weight):
        item = self.sampler_item(len(self.items), weight)
        self.items.append(item)
        if not (item.bucket_index in self.buckets):
            self.buckets[item.bucket_index] = self.bucket(item._max_weight())
        self.buckets[item.bucket_index].append(item)
        self.max_bucket = max(self.max_bucket, item.bucket_index)
        self.total_max_weight += self.buckets[item.bucket_index].max_weight
        return item.index

    # after freeze, item can be accessed but not returned by sample()
    def freeze(self, index):
        item = self.items[index]
        bucket = self.buckets[item.bucket_index]
        self.total_max_weight -= bucket.max_weight
        bucket.remove(item)

    def sample(self):
        assert self.total_max_weight > 0
        item = None
        while not item:
            threshold = int(floor(random.random()*self.total_max_weight))
            #print "sample threshold:", threshold,
            bucket_index = self.max_bucket
            while True:
                threshold -= self.buckets[bucket_index]._total_max_weight()
                #print threshold,
                if threshold < 0: break
                bucket_index -= 1
                while bucket_index not in self.buckets:
                    bucket_index -= 1
                    assert bucket_index >= 0
            item = self.buckets[bucket_index].sample()
            #print "b=", bucket_index, "; i=", item
        return item.index

if __name__ == "__main__":
    s = increasing_sampler()
    for i in range(1,10):
        s.append(i)
    for i in range(10):
        print s.sample(),
    print
    for i in range(1,10):
        s.raise_weight(i-1,i*i)
    for i in range(10):
        print s.sample(),
    print
