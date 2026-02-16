"""
HIME Input Engine

Core phonetic (Bopomofo/Zhuyin) input engine for HIME.
"""

import os
from .hime_data import (
    PhoneticTable, KeyboardMapping,
    pho2key, key_typ_pho, phokey_to_str,
    find_data_dir, BOPOMOFO_CHARS, TYP_PHO_LEN
)


# Standard Zhuyin keyboard layout (zo.kbm equivalent)
# Maps ASCII keys to (num, typ) pairs for phonetic components
STANDARD_ZHUYIN_MAP = {
    # Row 1: 1234567890-=
    '1': [(1, 0)],   # ㄅ
    'q': [(2, 0)],   # ㄆ
    'a': [(3, 0)],   # ㄇ
    'z': [(4, 0)],   # ㄈ
    '2': [(5, 0)],   # ㄉ
    'w': [(6, 0)],   # ㄊ
    's': [(7, 0)],   # ㄋ
    'x': [(8, 0)],   # ㄌ
    'e': [(9, 0)],   # ㄍ
    'd': [(10, 0)],  # ㄎ
    'c': [(11, 0)],  # ㄏ
    'r': [(12, 0)],  # ㄐ
    'f': [(13, 0)],  # ㄑ
    'v': [(14, 0)],  # ㄒ
    '5': [(15, 0)],  # ㄓ
    't': [(16, 0)],  # ㄔ
    'g': [(17, 0)],  # ㄕ
    'b': [(18, 0)],  # ㄖ
    'y': [(19, 0)],  # ㄗ
    'h': [(20, 0)],  # ㄘ
    'n': [(21, 0)],  # ㄙ
    # Medials
    'u': [(1, 1)],   # ㄧ
    'j': [(2, 1)],   # ㄨ
    'm': [(3, 1)],   # ㄩ
    # Finals
    '8': [(1, 2)],   # ㄚ
    'i': [(2, 2)],   # ㄛ
    'k': [(3, 2)],   # ㄜ
    ',': [(4, 2)],   # ㄝ
    '9': [(5, 2)],   # ㄞ
    'o': [(6, 2)],   # ㄟ
    'l': [(7, 2)],   # ㄠ
    '.': [(8, 2)],   # ㄡ
    '0': [(9, 2)],   # ㄢ
    'p': [(10, 2)],  # ㄣ
    ';': [(11, 2)],  # ㄤ
    '/': [(12, 2)],  # ㄥ
    '-': [(13, 2)],  # ㄦ
    # Tones
    '3': [(2, 3)],   # ˊ (2nd tone)
    '4': [(3, 3)],   # ˇ (3rd tone)
    '6': [(4, 3)],   # ˋ (4th tone)
    '7': [(5, 3)],   # ˙ (neutral tone)
    ' ': [(1, 3)],   # (1st tone / space to select)
}


class PhoneticEngine:
    """Phonetic (Bopomofo/Zhuyin) input engine"""

    def __init__(self, data_dir=None):
        self.data_dir = data_dir or find_data_dir()
        self.pho_table = None
        self.kbm = None

        # Current input state
        self.typ_pho = [0, 0, 0, 0]  # [initial, medial, final, tone]
        self.inph = [0, 0, 0, 0]     # Input keys for each position
        self.candidates = []
        self.candidate_page = 0
        self.candidates_per_page = 10

        # Selection keys
        self.sel_keys = '1234567890'

        self._load_data()

    def _load_data(self):
        """Load phonetic table and keyboard mapping"""
        # Try to load phonetic table
        pho_files = ['pho.tab2', 's-pho.tab2', 'pho-huge.tab2']
        for fname in pho_files:
            path = os.path.join(self.data_dir, fname)
            if os.path.exists(path):
                try:
                    self.pho_table = PhoneticTable(path)
                    break
                except Exception as e:
                    print(f"Failed to load {path}: {e}")

        # Try to load keyboard mapping
        kbm_files = ['zo.kbm', 'et.kbm']
        for fname in kbm_files:
            path = os.path.join(self.data_dir, fname)
            if os.path.exists(path):
                try:
                    self.kbm = KeyboardMapping(path)
                    break
                except Exception as e:
                    print(f"Failed to load {path}: {e}")

    def reset(self):
        """Reset input state"""
        self.typ_pho = [0, 0, 0, 0]
        self.inph = [0, 0, 0, 0]
        self.candidates = []
        self.candidate_page = 0

    def is_empty(self):
        """Check if no phonetic input has been entered"""
        return all(p == 0 for p in self.typ_pho)

    def get_preedit(self):
        """Get current preedit string (Bopomofo display)"""
        if self.is_empty():
            return ''

        result = ''
        for i in range(4):
            idx = self.typ_pho[i]
            if idx > 0 and idx < len(BOPOMOFO_CHARS[i]):
                result += BOPOMOFO_CHARS[i][idx]

        return result

    def process_key(self, key_char):
        """
        Process a key input.

        Args:
            key_char: Single character key pressed

        Returns:
            tuple: (handled, commit_string, show_candidates)
                - handled: True if key was consumed
                - commit_string: String to commit (if any)
                - show_candidates: True if candidate window should be shown
        """
        key = key_char.lower()

        # Check for candidate selection
        if self.candidates and key in self.sel_keys:
            idx = self.sel_keys.index(key)
            page_start = self.candidate_page * self.candidates_per_page
            actual_idx = page_start + idx
            if actual_idx < len(self.candidates):
                commit = self.candidates[actual_idx]
                self.reset()
                return (True, commit, False)

        # Check for standard Zhuyin mapping
        if key in STANDARD_ZHUYIN_MAP:
            mappings = STANDARD_ZHUYIN_MAP[key]
            for num, typ in mappings:
                self._insert_phonetic(num, typ, key)

            # Check if we have a complete syllable (has tone or space pressed)
            if self.typ_pho[3] != 0 or key == ' ':
                self._lookup_candidates()
                if len(self.candidates) == 1:
                    # Auto-select if only one candidate
                    commit = self.candidates[0]
                    self.reset()
                    return (True, commit, False)
                elif len(self.candidates) > 0:
                    return (True, '', True)

            return (True, '', False)

        # Handle Escape to cancel
        if key_char == '\x1b':  # Escape
            self.reset()
            return (True, '', False)

        # Handle Backspace
        if key_char == '\x08':  # Backspace
            if not self.is_empty():
                self._delete_last()
                return (True, '', False)
            return (False, '', False)

        return (False, '', False)

    def _insert_phonetic(self, num, typ, key):
        """Insert a phonetic component"""
        # Find the max filled position
        max_in_idx = -1
        for i in range(3, -1, -1):
            if self.typ_pho[i]:
                max_in_idx = i
                break

        # Try insert mode (fill empty slots in order)
        if typ > max_in_idx:
            self.typ_pho[typ] = num
            self.inph[typ] = ord(key)
        else:
            # Overwrite mode
            self.typ_pho[typ] = num
            self.inph[typ] = ord(key)

    def _delete_last(self):
        """Delete the last entered phonetic component"""
        # Find and clear the last non-zero component
        for i in range(3, -1, -1):
            if self.typ_pho[i]:
                self.typ_pho[i] = 0
                self.inph[i] = 0
                break

        self.candidates = []
        self.candidate_page = 0

    def _lookup_candidates(self):
        """Look up candidates for current phonetic input"""
        self.candidates = []
        self.candidate_page = 0

        if self.is_empty():
            return

        # Generate phonetic key
        phokey = pho2key(self.typ_pho)

        if self.pho_table:
            self.candidates = self.pho_table.lookup(phokey)

    def get_candidates(self):
        """Get current page of candidates"""
        if not self.candidates:
            return []

        start = self.candidate_page * self.candidates_per_page
        end = start + self.candidates_per_page
        return self.candidates[start:end]

    def page_up(self):
        """Move to previous candidate page"""
        if self.candidate_page > 0:
            self.candidate_page -= 1
            return True
        return False

    def page_down(self):
        """Move to next candidate page"""
        max_page = (len(self.candidates) - 1) // self.candidates_per_page
        if self.candidate_page < max_page:
            self.candidate_page += 1
            return True
        return False

    def select_candidate(self, index):
        """Select candidate by index (0-9 for current page)"""
        page_start = self.candidate_page * self.candidates_per_page
        actual_idx = page_start + index
        if actual_idx < len(self.candidates):
            commit = self.candidates[actual_idx]
            self.reset()
            return commit
        return None


class HIMEInputEngine:
    """Main HIME input engine supporting multiple input methods"""

    def __init__(self, data_dir=None):
        self.data_dir = data_dir or find_data_dir()
        self.phonetic_engine = PhoneticEngine(self.data_dir)
        self.current_mode = 'phonetic'  # 'phonetic', 'table'
        self.chinese_mode = True  # True = Chinese, False = English

    def reset(self):
        """Reset all input state"""
        self.phonetic_engine.reset()

    def toggle_chinese_mode(self):
        """Toggle between Chinese and English mode"""
        self.chinese_mode = not self.chinese_mode
        self.reset()
        return self.chinese_mode

    def process_key(self, key_char):
        """
        Process a key input.

        Returns:
            tuple: (handled, commit_string, preedit_string, candidates, show_candidates)
        """
        if not self.chinese_mode:
            # English mode - pass through
            return (False, '', '', [], False)

        if self.current_mode == 'phonetic':
            handled, commit, show_cands = self.phonetic_engine.process_key(key_char)
            preedit = self.phonetic_engine.get_preedit()
            candidates = self.phonetic_engine.get_candidates() if show_cands else []
            return (handled, commit, preedit, candidates, show_cands)

        return (False, '', '', [], False)

    def get_preedit(self):
        """Get current preedit string"""
        if self.current_mode == 'phonetic':
            return self.phonetic_engine.get_preedit()
        return ''

    def get_candidates(self):
        """Get current candidates"""
        if self.current_mode == 'phonetic':
            return self.phonetic_engine.get_candidates()
        return []

    def select_candidate(self, index):
        """Select a candidate"""
        if self.current_mode == 'phonetic':
            return self.phonetic_engine.select_candidate(index)
        return None

    def page_up(self):
        """Page up in candidate list"""
        if self.current_mode == 'phonetic':
            return self.phonetic_engine.page_up()
        return False

    def page_down(self):
        """Page down in candidate list"""
        if self.current_mode == 'phonetic':
            return self.phonetic_engine.page_down()
        return False
