#!/usr/bin/env python3.2

# TODO: make input not line-buffered

import sys, struct
from itertools import count,zip_longest,chain
from operator import itemgetter

concat_channels = False

encoding = sys.argv[1]
if encoding[0] == "u":
    signed = False
elif encoding[0] == "s":
    signed = True
else:
    raise ValueError("Invalid signed-ness")

bits = int(encoding[1:3])
if bits % 8 != 0:
    raise ValueError("Invalid bits-per-sample")

if encoding[3:5] == "le":
    little_endian = True
elif encoding[3:5] == "be":
    little_endian = False
else:
    raise ValueError("Invalid endian-ness")

num_channels = int(sys.argv[2])

def int_to_bytes(n, blocksize=0):
    """long_to_bytes(n:int, blocksize:int) : bytes
    Convert an integer to big-endian bytes.

    If optional blocksize is given and greater than zero, pad the front of the
    byte string with binary zeros so that the length is a multiple of
    blocksize.
    """
    # after much testing, this algorithm was deemed to be the fastest
    s = b''
    pack = struct.pack
    while n > 0:
        s = pack('>I', n & 0xffffffff) + s
        n = n >> 32
    # strip off leading zeros
    for i in range(len(s)):
        if s[i] != b'\000':
            break
    else:
        # only happens when n == 0
        s = b'\000'
        i = 0
    s = s[i:]
    # add back some pad bytes.  this could be done more efficiently w.r.t. the
    # de-padding being done above, but sigh...
    if blocksize > 0 and len(s) % blocksize:
        s = (blocksize - len(s) % blocksize) * b'\000' + s
    return s

def bytes_to_int(s,signed=False):
    """bytes_to_int(bytes, signed:bool) : int
    Convert big-endian bytes to an integer.

    This is (essentially) the inverse of int_to_bytes(). The optional signed
    is whether the bytes should be interpreted as a signed or unsigned int.
    """
    acc = 0
    unpack = struct.unpack
    length = len(s)
    numbits = length*8
    if length % 4:
        extra = (4 - length % 4)
        s = b'\000' * extra + s
        length = length + extra
    for i in range(0, length, 4):
        acc = (acc << 32) + unpack('>I', s[i:i+4])[0]
    return ( acc - 2**(numbits) 
             if signed and acc >= 2**(numbits-1)
             else acc )

def grouper(n, iterable, fillvalue=None):
    "grouper(3, 'ABCDEFG', 'x') --> ABC DEF Gxx"
    args = [iter(iterable)] * n
    return zip_longest(*args, fillvalue=fillvalue)

def flatten(listOfLists):
    "Flatten one level of nesting"
    return chain.from_iterable(listOfLists)

class Wrapper(object):
    def __init__(self,original):
        self.underlying = original

    def __getattribute__(self,name):
        try:
            return object.__getattribute__(self,name)
        except AttributeError:
            return getattr(object.__getattribute__(self,"underlying"),name)

class ByteIterWrapper(Wrapper):
    def __iter__(self):
        return self
    def __next__(self):
        retval = self.underlying.read(1)
        if len(retval) == 0:
            raise StopIteration
        else:
            return retval

sys.stdin = sys.stdin.detach()
sys.stdout = sys.stdout.detach()

data = ByteIterWrapper(sys.stdin)

if little_endian:
    sample_bytes = map(lambda x: bytes(x)[::-1],
                       map(flatten,
                           grouper(bits//8,data)))
else:
    sample_bytes = map(bytes,map(flatten,grouper(bits//8,data)))

sample_ints = map(lambda x: bytes_to_int(x,signed=signed),
                  sample_bytes)


if concat_channels:
    output = ( ( ( sum( ( s % 2 ) << i
                        for (i,s) in zip(count(),samples) )
                   for samples in grouper(8,channel,fillvalue=0) )
                 for channel in zip(*grouper(num_channels,sample_ints)) ) )
    output = flatten(output)
else:
    output = ( ( sum( ( s % 2 ) << i 
                      for (i,s) in zip(count(),samples) )
                 for samples in grouper(8,sample_ints,fillvalue=0) ) )
# eager and shitty
sys.stdout.write(bytes(output))

print("Done",file=sys.stderr)
