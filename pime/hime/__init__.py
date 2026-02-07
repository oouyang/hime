"""
HIME Input Method Module for PIME

This module provides HIME (Huge Input Method Editor) support on Windows
through the PIME framework.

Supported input methods:
- Bopomofo (Zhuyin) phonetic input for Traditional Chinese
"""

from .hime_ime import HIMETextService
from .hime_engine import HIMEInputEngine, PhoneticEngine
from .hime_data import PhoneticTable, KeyboardMapping

__all__ = [
    'HIMETextService',
    'HIMEInputEngine',
    'PhoneticEngine',
    'PhoneticTable',
    'KeyboardMapping',
]

__version__ = '0.1.0'
