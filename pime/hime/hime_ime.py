"""
HIME Input Method for PIME

This module implements the PIME TextService interface for HIME,
providing Bopomofo (Zhuyin) phonetic input on Windows.
"""

import os
import sys

# Add parent directory to path for PIME imports
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from input_methods.textService import TextService
from .hime_engine import HIMEInputEngine

# Virtual key codes
VK_BACK = 0x08
VK_TAB = 0x09
VK_RETURN = 0x0D
VK_SHIFT = 0x10
VK_CONTROL = 0x11
VK_MENU = 0x12  # Alt
VK_ESCAPE = 0x1B
VK_SPACE = 0x20
VK_PRIOR = 0x21  # Page Up
VK_NEXT = 0x22   # Page Down
VK_END = 0x23
VK_HOME = 0x24
VK_LEFT = 0x25
VK_UP = 0x26
VK_RIGHT = 0x27
VK_DOWN = 0x28
VK_DELETE = 0x2E


class HIMETextService(TextService):
    """HIME Input Method TextService for PIME"""

    def __init__(self, client):
        super().__init__(client)

        # Find data directory
        self.data_dir = os.path.join(os.path.dirname(__file__), 'data')

        # Initialize HIME engine
        self.engine = HIMEInputEngine(self.data_dir)

        # IME state
        self.show_cands = False
        self.icon_dir = os.path.dirname(__file__)

    def onActivate(self):
        """Called when the input method is activated"""
        super().onActivate()

        # Customize UI
        self.customizeUI(
            candFontSize=20,
            candFontName='Microsoft JhengHei',
            candPerRow=1,
            candUseCursor=True
        )

        # Set selection keys
        self.setSelKeys('1234567890')

        # Add language bar button for mode toggle
        self.addButton(
            'mode_toggle',
            icon=os.path.join(self.icon_dir, 'chi.ico'),
            tooltip='中/英切換 (Shift)',
            commandId=1
        )

        # Reset engine state
        self.engine.reset()

    def onDeactivate(self):
        """Called when the input method is deactivated"""
        self.engine.reset()
        super().onDeactivate()

    def filterKeyDown(self, keyEvent):
        """
        Pre-filter key events.
        Return True to process the key, False to pass through.
        """
        # Always process keys when in Chinese mode
        if self.engine.chinese_mode:
            # Handle modifier keys
            keyCode = keyEvent.keyCode

            # Shift alone toggles Chinese/English mode
            if keyCode == VK_SHIFT and not keyEvent.isKeyDown(VK_CONTROL) and not keyEvent.isKeyDown(VK_MENU):
                return False  # Let onKeyDown handle it

            # Control/Alt combos pass through
            if keyEvent.isKeyDown(VK_CONTROL) or keyEvent.isKeyDown(VK_MENU):
                return False

            # Process printable characters and navigation keys
            if keyEvent.isPrintableChar():
                return True

            # Process special keys
            if keyCode in (VK_BACK, VK_ESCAPE, VK_RETURN, VK_SPACE,
                          VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT,
                          VK_PRIOR, VK_NEXT, VK_HOME, VK_END):
                return True

        return False

    def onKeyDown(self, keyEvent):
        """Process key down event"""
        keyCode = keyEvent.keyCode
        charCode = keyEvent.charCode

        # Shift toggles Chinese/English mode
        if keyCode == VK_SHIFT and keyEvent.repeatCount == 1:
            if not keyEvent.isKeyDown(VK_CONTROL) and not keyEvent.isKeyDown(VK_MENU):
                self.engine.toggle_chinese_mode()
                self._update_mode_button()
                self._clear_composition()
                return True

        # If not in Chinese mode, pass through
        if not self.engine.chinese_mode:
            return False

        # Handle Escape
        if keyCode == VK_ESCAPE:
            if self.isComposing() or self.show_cands:
                self.engine.reset()
                self._clear_composition()
                return True
            return False

        # Handle Backspace
        if keyCode == VK_BACK:
            if self.isComposing():
                handled, commit, preedit, cands, show = self.engine.process_key('\x08')
                self._update_display(preedit, cands, show)
                return True
            return False

        # Handle Enter
        if keyCode == VK_RETURN:
            preedit = self.engine.get_preedit()
            if preedit:
                # Commit preedit as-is
                self.setCommitString(preedit)
                self.engine.reset()
                self._clear_composition()
                return True
            return False

        # Handle Page Up/Down in candidate window
        if keyCode == VK_PRIOR:
            if self.show_cands and self.engine.page_up():
                self._update_candidates()
                return True
            return False

        if keyCode == VK_NEXT:
            if self.show_cands and self.engine.page_down():
                self._update_candidates()
                return True
            return False

        # Handle Up/Down arrows for candidate selection
        if keyCode == VK_UP:
            if self.show_cands and self.engine.page_up():
                self._update_candidates()
                return True
            return False

        if keyCode == VK_DOWN:
            if self.show_cands and self.engine.page_down():
                self._update_candidates()
                return True
            return False

        # Handle character input
        if charCode and keyEvent.isPrintableChar():
            key_char = chr(charCode)
            handled, commit, preedit, cands, show = self.engine.process_key(key_char)

            if commit:
                self.setCommitString(commit)
                self._clear_composition()
            elif handled:
                self._update_display(preedit, cands, show)

            return handled

        return False

    def onKeyUp(self, keyEvent):
        """Process key up event"""
        return False

    def filterKeyUp(self, keyEvent):
        """Pre-filter key up events"""
        return False

    def onCommand(self, commandId, commandType):
        """Handle language bar button clicks"""
        if commandId == 1:
            # Toggle Chinese/English mode
            self.engine.toggle_chinese_mode()
            self._update_mode_button()
            self._clear_composition()

    def onCompositionTerminated(self, forced):
        """Called when composition is forcefully terminated"""
        self.engine.reset()
        self.show_cands = False

    def _update_display(self, preedit, candidates, show_candidates):
        """Update the composition display"""
        self.setCompositionString(preedit)
        self.setCompositionCursor(len(preedit))

        if candidates:
            self.setCandidateList(candidates)
            self.setShowCandidates(True)
            self.show_cands = True
        else:
            self.setShowCandidates(False)
            self.show_cands = False

    def _update_candidates(self):
        """Update candidate list display"""
        candidates = self.engine.get_candidates()
        if candidates:
            self.setCandidateList(candidates)
            self.setShowCandidates(True)
        else:
            self.setShowCandidates(False)

    def _clear_composition(self):
        """Clear composition and candidate display"""
        self.setCompositionString('')
        self.setShowCandidates(False)
        self.show_cands = False

    def _update_mode_button(self):
        """Update the mode toggle button icon"""
        if self.engine.chinese_mode:
            icon = os.path.join(self.icon_dir, 'chi.ico')
            tooltip = '中文模式 (按 Shift 切換英文)'
        else:
            icon = os.path.join(self.icon_dir, 'eng.ico')
            tooltip = '英文模式 (按 Shift 切換中文)'

        self.changeButton(
            'mode_toggle',
            icon=icon,
            tooltip=tooltip
        )
