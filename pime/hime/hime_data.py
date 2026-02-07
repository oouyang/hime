"""
HIME Data File Parser

This module provides functions to read HIME's binary data files:
- Phonetic tables (.tab2)
- Keyboard mappings (.kbm)
- Generic tables (.gtab)
"""

import struct
import os
from collections import namedtuple

# Constants
CH_SZ = 4  # Maximum UTF-8 character size used in HIME
PHO_PHRASE_ESCAPE = 0x1B

# Data structures
PhoIdx = namedtuple('PhoIdx', ['key', 'start'])
PhoItem = namedtuple('PhoItem', ['ch', 'count'])
KeyMapping = namedtuple('KeyMapping', ['num', 'typ'])  # typ: 0=initial, 1=medial, 2=final, 3=tone

# Phonetic segment bit lengths (from pho.c)
TYP_PHO_LEN = [5, 2, 4, 3]  # Initial, medial, final, tone


class PhoneticTable:
    """Reader for HIME phonetic table files (.tab2)"""

    def __init__(self, filepath):
        self.filepath = filepath
        self.idx_pho = []  # List of PhoIdx
        self.ch_pho = []   # List of PhoItem
        self.phrase_area = b''
        self.idxnum_pho = 0
        self.ch_phoN = 0

        self._load()

    def _load(self):
        """Load phonetic table from file"""
        with open(self.filepath, 'rb') as f:
            # Read header
            # Note: idxnum_pho is read twice (historical quirk)
            self.idxnum_pho = struct.unpack('<H', f.read(2))[0]
            self.idxnum_pho = struct.unpack('<H', f.read(2))[0]
            self.ch_phoN = struct.unpack('<I', f.read(4))[0]
            phrase_area_sz = struct.unpack('<I', f.read(4))[0]

            # Read index array
            # PHO_IDX: phokey_t (u_short) + u_short start = 4 bytes
            for _ in range(self.idxnum_pho):
                key, start = struct.unpack('<HH', f.read(4))
                self.idx_pho.append(PhoIdx(key, start))

            # Add sentinel entry
            self.idx_pho.append(PhoIdx(0xFFFF, self.ch_phoN))

            # Read character array
            # PHO_ITEM: char[CH_SZ] + int count = 4 + 4 = 8 bytes
            for _ in range(self.ch_phoN):
                ch_bytes = f.read(CH_SZ)
                count = struct.unpack('<I', f.read(4))[0]
                # Decode UTF-8 character, strip null bytes
                try:
                    ch = ch_bytes.rstrip(b'\x00').decode('utf-8')
                except UnicodeDecodeError:
                    ch = ch_bytes.rstrip(b'\x00').decode('latin-1')
                self.ch_pho.append(PhoItem(ch, count))

            # Read phrase area
            if phrase_area_sz > 0:
                self.phrase_area = f.read(phrase_area_sz)

    def get_char(self, idx):
        """Get character at index, handling phrase escapes"""
        if idx < 0 or idx >= len(self.ch_pho):
            return ''

        item = self.ch_pho[idx]
        ch = item.ch

        # Check for phrase escape
        if ch and ord(ch[0]) == PHO_PHRASE_ESCAPE:
            # Extract phrase offset from remaining bytes
            raw = self.ch_pho[idx]
            ch_bytes = raw.ch.encode('latin-1') if isinstance(raw.ch, str) else raw.ch
            if len(ch_bytes) >= 4:
                offset = ch_bytes[1] | (ch_bytes[2] << 8) | (ch_bytes[3] << 16)
                # Extract phrase from phrase area
                end = self.phrase_area.find(b'\x00', offset)
                if end == -1:
                    end = len(self.phrase_area)
                try:
                    return self.phrase_area[offset:end].decode('utf-8')
                except:
                    return ''

        return ch

    def lookup(self, phokey):
        """Look up characters for a phonetic key"""
        results = []

        # Binary search for the key in idx_pho
        for i, idx in enumerate(self.idx_pho[:-1]):
            if idx.key == phokey:
                # Found matching key, get all characters
                start = idx.start
                end = self.idx_pho[i + 1].start
                for j in range(start, end):
                    ch = self.get_char(j)
                    if ch:
                        count = self.ch_pho[j].count
                        results.append((ch, count))
                break
            elif idx.key > phokey:
                break

        # Sort by usage count (descending)
        results.sort(key=lambda x: -x[1])
        return [ch for ch, _ in results]


class KeyboardMapping:
    """Reader for HIME keyboard mapping files (.kbm)"""

    def __init__(self, filepath):
        self.filepath = filepath
        self.selkeyN = 0
        # phokbm[key][slot] = (num, typ)
        # key: 0-127 (ASCII)
        # slot: 0-2 (up to 3 mappings per key)
        self.phokbm = [[None, None, None] for _ in range(128)]

        self._load()

    def _load(self):
        """Load keyboard mapping from file"""
        with open(self.filepath, 'rb') as f:
            # PHOKBM structure: char selkeyN + phokbm[128][3] of {char num, char typ}
            self.selkeyN = struct.unpack('<B', f.read(1))[0]

            # Read the mapping array
            for key in range(128):
                for slot in range(3):
                    num = struct.unpack('<b', f.read(1))[0]
                    typ = struct.unpack('<b', f.read(1))[0]
                    if num != 0:
                        self.phokbm[key][slot] = KeyMapping(num, typ)

    def get_mapping(self, key):
        """Get phonetic mapping for a key (ASCII code)"""
        if 0 <= key < 128:
            return [m for m in self.phokbm[key] if m is not None]
        return []


class GenericTable:
    """Reader for HIME generic table files (.gtab)"""

    def __init__(self, filepath):
        self.filepath = filepath
        self.cname = ''  # Display name
        self.selkey = '1234567890'  # Selection keys
        self.KeyS = 0    # Number of keys
        self.MaxPress = 0  # Max keystroke length
        self.DefChars = 0  # Total characters
        self.keyname = []  # Key symbols
        self.items = []    # List of (key_sequence, character)
        self.key64 = False
        self.keybits = 5

        self._load()

    def _load(self):
        """Load generic table from file"""
        with open(self.filepath, 'rb') as f:
            # Read TableHead structure
            version = struct.unpack('<I', f.read(4))[0]
            flag = struct.unpack('<I', f.read(4))[0]
            cname_bytes = f.read(32)
            self.cname = cname_bytes.rstrip(b'\x00').decode('utf-8', errors='replace')
            selkey_bytes = f.read(12)
            self.selkey = selkey_bytes.rstrip(b'\x00').decode('ascii', errors='replace')
            space_style = struct.unpack('<I', f.read(4))[0]
            self.KeyS = struct.unpack('<I', f.read(4))[0]
            self.MaxPress = struct.unpack('<I', f.read(4))[0]
            M_DUP_SEL = struct.unpack('<I', f.read(4))[0]
            self.DefChars = struct.unpack('<I', f.read(4))[0]

            # Skip QUICK_KEYS (large array)
            # quick1: 46 * 10 * CH_SZ = 1840 bytes
            # quick2: 46 * 46 * 10 * CH_SZ = 84640 bytes
            quick_keys_size = 46 * 10 * CH_SZ + 46 * 46 * 10 * CH_SZ
            f.seek(quick_keys_size, 1)

            # Read endkey and keybits from union
            endkey_bytes = f.read(99)
            self.keybits = struct.unpack('<B', f.read(1))[0]
            if self.keybits == 0:
                self.keybits = 5  # Default
            selkey2_bytes = f.read(10)

            # Padding to 128 bytes for union
            f.read(128 - 99 - 1 - 10)

            # Determine if 64-bit keys
            self.key64 = (self.keybits * self.MaxPress > 32)

            # Read keyname table (KeyS entries, each CH_SZ bytes)
            # But first we need to know kmask
            self.kmask = (1 << self.keybits) - 1
            max_keys = 1 << self.keybits

            # Skip to table data - the format varies
            # For simplicity, we'll just read what we can
            # The actual keyname and table reading is complex

            # Read ITEM or ITEM64 array
            item_size = 8 if not self.key64 else 12  # key[4/8] + ch[4]

            for _ in range(self.DefChars):
                if self.key64:
                    key_bytes = f.read(8)
                    key = struct.unpack('<Q', key_bytes)[0]
                else:
                    key_bytes = f.read(4)
                    key = struct.unpack('<I', key_bytes)[0]

                ch_bytes = f.read(CH_SZ)
                try:
                    ch = ch_bytes.rstrip(b'\x00').decode('utf-8')
                except:
                    ch = ''

                if ch:
                    self.items.append((key, ch))

    def lookup(self, key_seq):
        """Look up characters for a key sequence"""
        results = []
        for key, ch in self.items:
            if key == key_seq:
                results.append(ch)
        return results

    def lookup_prefix(self, key_seq, num_keys):
        """Look up characters that start with the given key prefix"""
        results = []
        # Mask to extract the prefix
        shift = (self.MaxPress - num_keys) * self.keybits
        mask = ((1 << (num_keys * self.keybits)) - 1) << shift

        for key, ch in self.items:
            if (key & mask) == (key_seq & mask):
                results.append((key, ch))

        return results


def find_data_dir():
    """Find HIME data directory"""
    # Check common locations
    candidates = [
        os.path.join(os.path.dirname(__file__), 'data'),
        os.path.expanduser('~/.hime'),
        '/usr/share/hime/table',
        '/usr/local/share/hime/table',
    ]

    for path in candidates:
        if os.path.isdir(path):
            return path

    return os.path.join(os.path.dirname(__file__), 'data')


# Phonetic key encoding/decoding functions
def pho2key(typ_pho):
    """Convert 4-segment phonetic array to phokey_t"""
    if typ_pho[0] == 24:  # BACK_QUOTE_NO
        return (24 << 9) | typ_pho[1]

    key = typ_pho[0]
    for i in range(1, 4):
        key = typ_pho[i] | (key << TYP_PHO_LEN[i])

    return key


def key_typ_pho(phokey):
    """Convert phokey_t to 4-segment phonetic array"""
    rtyp_pho = [0, 0, 0, 0]
    rtyp_pho[3] = phokey & 7
    phokey >>= 3
    rtyp_pho[2] = phokey & 0xF
    phokey >>= 4
    rtyp_pho[1] = phokey & 0x3
    phokey >>= 2
    rtyp_pho[0] = phokey
    return rtyp_pho


# Bopomofo display characters
BOPOMOFO_CHARS = [
    # Initials (consonants) - index 1-21
    ['', 'ㄅ', 'ㄆ', 'ㄇ', 'ㄈ', 'ㄉ', 'ㄊ', 'ㄋ', 'ㄌ',
     'ㄍ', 'ㄎ', 'ㄏ', 'ㄐ', 'ㄑ', 'ㄒ', 'ㄓ', 'ㄔ',
     'ㄕ', 'ㄖ', 'ㄗ', 'ㄘ', 'ㄙ', '[', ']', '`'],
    # Medials - index 1-3
    ['', 'ㄧ', 'ㄨ', 'ㄩ'],
    # Finals (vowels) - index 1-13
    ['', 'ㄚ', 'ㄛ', 'ㄜ', 'ㄝ', 'ㄞ', 'ㄟ', 'ㄠ', 'ㄡ',
     'ㄢ', 'ㄣ', 'ㄤ', 'ㄥ', 'ㄦ'],
    # Tones - index 1-5
    ['', '', 'ˊ', 'ˇ', 'ˋ', '˙'],
]


def phokey_to_str(phokey):
    """Convert phonetic key to Bopomofo display string"""
    typ_pho = key_typ_pho(phokey)
    result = ''

    for i in range(4):
        idx = typ_pho[i]
        if idx > 0 and idx < len(BOPOMOFO_CHARS[i]):
            result += BOPOMOFO_CHARS[i][idx]

    return result
