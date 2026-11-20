"""Sawit/S++ -> C/C++ transpiler (port alternatif dari implementasi Go utama).

Implementasi referensi adalah main.go. Modul ini menjaga paritas perilaku:
keyword & fungsi identik, output transpile byte-identik, serta perintah
transpile/build/run/init/audit yang sama.
Aset runtime/boot ditanam di skrip agar tetap bisa dipakai bila skrip
disalin ke proyek contoh terpisah.
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
import base64 as _base64

_EMBEDDED_ASSETS = {
    "sawit_runtime.h": (
    "I2lmbmRlZiBTQVdJVF9SVU5USU1FX0gKI2RlZmluZSBTQVdJVF9SVU5USU1FX0gKCi8qIER1YSBtb2RlOgogKiAg"
    "LSBob3N0ZWQgKGRlZmF1bHQpOiBtZW1ha2FpIGxpYmMsIGNvY29rIHVudHVrIGFwbGlrYXNpICYgc2hlbGwgU2F3"
    "aXQuCiAqICAtIGZyZWVzdGFuZGluZyAoI2RlZmluZSBTQVdJVF9GUkVFU1RBTkRJTkcpOiBtZW1ha2FpIHNhd2l0"
    "X2tlcm5lbC5oLAogKiAgICB0YW5wYSBsaWJjLCB1bnR1ayBrZXJuZWwgeWFuZyBkaS1idWlsZCBkZW5nYW4gLS1m"
    "cmVlc3RhbmRpbmcuCiAqLwojaWZkZWYgU0FXSVRfRlJFRVNUQU5ESU5HCiNpbmNsdWRlICJzYXdpdF9rZXJuZWwu"
    "aCIKI2Vsc2UKCiNpbmNsdWRlIDxzdGRhcmcuaD4KI2luY2x1ZGUgPHN0ZGRlZi5oPgojaW5jbHVkZSA8c3RkaW8u"
    "aD4KI2luY2x1ZGUgPHN0ZGxpYi5oPgojaW5jbHVkZSA8c3RyaW5nLmg+CiNpbmNsdWRlIDxzdGRib29sLmg+CiNp"
    "bmNsdWRlIDxzdGRpbnQuaD4KCnN0YXRpYyBpbnQgc2F3aXRfc3Rkb3V0X2J1bmdrYW0gPSAwOwpzdGF0aWMgaW50"
    "IHNhd2l0X3N0ZGVycl9idW5na2FtID0gMDsKc3RhdGljIGludCBzYXdpdF9zdGRpbl9idW5na2FtID0gMDsKCnN0"
    "YXRpYyBpbmxpbmUgdm9pZCBzYXdpdF9idW5na2FtKEZJTEUgKnN0cmVhbSkgewogICAgaWYgKHN0cmVhbSA9PSBz"
    "dGRvdXQpIHNhd2l0X3N0ZG91dF9idW5na2FtID0gMTsKICAgIGlmIChzdHJlYW0gPT0gc3RkZXJyKSBzYXdpdF9z"
    "dGRlcnJfYnVuZ2thbSA9IDE7Cn0Kc3RhdGljIGlubGluZSB2b2lkIHNhd2l0X2J1a2Ffc3VhcmEoRklMRSAqc3Ry"
    "ZWFtKSB7CiAgICBpZiAoc3RyZWFtID09IHN0ZG91dCkgc2F3aXRfc3Rkb3V0X2J1bmdrYW0gPSAwOwogICAgaWYg"
    "KHN0cmVhbSA9PSBzdGRlcnIpIHNhd2l0X3N0ZGVycl9idW5na2FtID0gMDsKfQpzdGF0aWMgaW5saW5lIHZvaWQg"
    "c2F3aXRfdHV0dXBfdGVsaW5nYSh2b2lkKSB7IHNhd2l0X3N0ZGluX2J1bmdrYW0gPSAxOyB9CnN0YXRpYyBpbmxp"
    "bmUgdm9pZCBzYXdpdF9idWthX3RlbGluZ2Eodm9pZCkgIHsgc2F3aXRfc3RkaW5fYnVuZ2thbSA9IDA7IH0KCnN0"
    "YXRpYyBpbmxpbmUgdm9pZCBzYXdpdF9idXp6ZXIoY29uc3QgY2hhciAqZm9ybWF0LCAuLi4pIHsKICAgIGlmIChz"
    "YXdpdF9zdGRvdXRfYnVuZ2thbSkgcmV0dXJuOwogICAgdmFfbGlzdCBhcmdzOwogICAgdmFfc3RhcnQoYXJncywg"
    "Zm9ybWF0KTsKICAgIHZmcHJpbnRmKHN0ZG91dCwgZm9ybWF0LCBhcmdzKTsKICAgIHZhX2VuZChhcmdzKTsKICAg"
    "IGZmbHVzaChzdGRvdXQpOwp9CgpzdGF0aWMgaW5saW5lIHZvaWQgc2F3aXRfZGVtb19tYXN5YXJha2F0KGNvbnN0"
    "IGNoYXIgKmZvcm1hdCwgLi4uKSB7CiAgICBpZiAoc2F3aXRfc3RkZXJyX2J1bmdrYW0pIHJldHVybjsKICAgIHZh"
    "X2xpc3QgYXJnczsKICAgIHZhX3N0YXJ0KGFyZ3MsIGZvcm1hdCk7CiAgICBmcHV0cygiW0RFTU8gTUFTWUFSQUtB"
    "VF0gIiwgc3RkZXJyKTsKICAgIHZmcHJpbnRmKHN0ZGVyciwgZm9ybWF0LCBhcmdzKTsKICAgIGZwdXRjKCdcbics"
    "IHN0ZGVycik7CiAgICB2YV9lbmQoYXJncyk7CiAgICBmZmx1c2goc3RkZXJyKTsKfQoKc3RhdGljIGlubGluZSB2"
    "b2lkIHNhd2l0X2JlcnNpaGthbl9sYXlhcih2b2lkKSB7CiNpZmRlZiBfV0lOMzIKICAgICh2b2lkKXN5c3RlbSgi"
    "Y2xzIik7CiNlbHNlCiAgICBmcHV0cygiXDAzM1sySlwwMzNbSCIsIHN0ZG91dCk7CiAgICBmZmx1c2goc3Rkb3V0"
    "KTsKI2VuZGlmCn0KCi8qIC0tLS0tLS0tLS0gSW5wdXQ6IHNhZGFwKCkgbWVueWFkYXAgc3RkaW4gLS0tLS0tLS0t"
    "LSAqLwpzdGF0aWMgaW5saW5lIGludCBzYXdpdF9zYWRhcChjb25zdCBjaGFyICpmb3JtYXQsIC4uLikgewogICAg"
    "aWYgKHNhd2l0X3N0ZGluX2J1bmdrYW0pIHJldHVybiAtMTsKICAgIHZhX2xpc3QgYXJnczsKICAgIHZhX3N0YXJ0"
    "KGFyZ3MsIGZvcm1hdCk7CiAgICBpbnQgcmVzdWx0ID0gdnNjYW5mKGZvcm1hdCwgYXJncyk7CiAgICB2YV9lbmQo"
    "YXJncyk7CiAgICByZXR1cm4gcmVzdWx0Owp9CgovKiBzYWRhcEJhcmlzOiBmZ2V0cyArIGJ1YW5nIFxuL1xyIGRp"
    "IGFraGlyLiByZXR1cm4gMCAoTlVMTCkgYmlsYSBFT0YuICovCnN0YXRpYyBpbmxpbmUgY2hhciAqc2F3aXRfc2Fk"
    "YXBfYmFyaXMoY2hhciAqYnVmZmVyLCBzaXplX3Qgc2l6ZSkgewogICAgaWYgKHNhd2l0X3N0ZGluX2J1bmdrYW0g"
    "fHwgIWJ1ZmZlciB8fCBzaXplID09IDApIHJldHVybiBOVUxMOwogICAgaWYgKCFmZ2V0cyhidWZmZXIsIChpbnQp"
    "c2l6ZSwgc3RkaW4pKSByZXR1cm4gTlVMTDsKICAgIHNpemVfdCBuID0gc3RybGVuKGJ1ZmZlcik7CiAgICB3aGls"
    "ZSAobiAmJiAoYnVmZmVyW24gLSAxXSA9PSAnXG4nIHx8IGJ1ZmZlcltuIC0gMV0gPT0gJ1xyJykpIGJ1ZmZlclst"
    "LW5dID0gJ1wwJzsKICAgIHJldHVybiBidWZmZXI7Cn0KCnN0YXRpYyBpbmxpbmUgaW50IHNhd2l0X3NhZGFwX2th"
    "cmFrdGVyKHZvaWQpIHsKICAgIGlmIChzYXdpdF9zdGRpbl9idW5na2FtKSByZXR1cm4gLTE7CiAgICByZXR1cm4g"
    "Z2V0Y2hhcigpOwp9CgovKiBCdWFuZyBzaXNhIGJhcmlzIHNldGVsYWggc2FkYXAoKSBzdXBheWEgbmV3bGluZSB0"
    "aWRhayBtZW5nZ2FuZ2d1IGlucHV0IGJlcmlrdXQuICovCnN0YXRpYyBpbmxpbmUgdm9pZCBzYXdpdF9zYWRhcF9z"
    "aXNhKHZvaWQpIHsKICAgIGludCBjOwogICAgd2hpbGUgKChjID0gZ2V0Y2hhcigpKSAhPSAnXG4nICYmIGMgIT0g"
    "RU9GKSB7IH0KfQoKLyogLS0tLS0tLS0tLSBTdHJpbmcgJiBrb252ZXJzaSAtLS0tLS0tLS0tICovCnN0YXRpYyBp"
    "bmxpbmUgaW50IHNhd2l0X3NhbWFfa2F0YShjb25zdCBjaGFyICphLCBjb25zdCBjaGFyICpiKSB7CiAgICByZXR1"
    "cm4gc3RyY21wKGEsIGIpID09IDA7Cn0Kc3RhdGljIGlubGluZSBzaXplX3Qgc2F3aXRfcGFuamFuZ19rYXRhKGNv"
    "bnN0IGNoYXIgKnRleHQpIHsgcmV0dXJuIHN0cmxlbih0ZXh0KTsgfQpzdGF0aWMgaW5saW5lIHZvaWQgc2F3aXRf"
    "c2FsaW5fa2F0YShjaGFyICpkc3QsIGNvbnN0IGNoYXIgKnNyYykgeyAodm9pZClzdHJjcHkoZHN0LCBzcmMpOyB9"
    "CnN0YXRpYyBpbmxpbmUgaW50IHNhd2l0X3RlcmplbWFoX2FuZ2thKGNvbnN0IGNoYXIgKnRleHQpIHsgcmV0dXJu"
    "IGF0b2kodGV4dCk7IH0KCiNkZWZpbmUgc2F3aXRfcGFqYWsodmFsdWUsIHBlcmNlbnQpIFwKICAgICgodmFsdWUp"
    "IC09ICgodmFsdWUpICogKHBlcmNlbnQpKSAvIDEwMCkKCi8qIE1CRyBjcmVkaXRzIGZ1bmRzOyBjYWlyIGRpc2J1"
    "cnNlcyBvbmx5IHdoYXQgaXMgY3VycmVudGx5IGF2YWlsYWJsZS4gKi8Kc3RhdGljIGxvbmcgbG9uZyBzYXdpdF9h"
    "bmdnYXJhbiA9IDA7CgpzdGF0aWMgaW5saW5lIGxvbmcgbG9uZyBzYXdpdF9tYmcobG9uZyBsb25nIGFtb3VudCkg"
    "ewogICAgaWYgKGFtb3VudCA+IDApIHNhd2l0X2FuZ2dhcmFuICs9IGFtb3VudDsKICAgIHJldHVybiBhbW91bnQ7"
    "Cn0Kc3RhdGljIGlubGluZSBsb25nIGxvbmcgc2F3aXRfY2Fpcihsb25nIGxvbmcgYW1vdW50KSB7CiAgICBpZiAo"
    "YW1vdW50IDw9IDAgfHwgYW1vdW50ID4gc2F3aXRfYW5nZ2FyYW4pIHJldHVybiAwOwogICAgc2F3aXRfYW5nZ2Fy"
    "YW4gLT0gYW1vdW50OwogICAgcmV0dXJuIGFtb3VudDsKfQoKLyogVHJhY2tlZCBoZWFwIGFsbG9jYXRpb25zOiBi"
    "ZWxpIGFsbG9jYXRlcywganVhbCByZWxlYXNlcyBhbmQgdW5yZWdpc3RlcnMuICovCnR5cGVkZWYgc3RydWN0IFNh"
    "d2l0QWxsb2NhdGlvbiB7CiAgICB2b2lkICpwb2ludGVyOwogICAgc2l6ZV90IGJ5dGVzOwogICAgc3RydWN0IFNh"
    "d2l0QWxsb2NhdGlvbiAqbmV4dDsKfSBTYXdpdEFsbG9jYXRpb247CgpzdGF0aWMgU2F3aXRBbGxvY2F0aW9uICpz"
    "YXdpdF9hbGxvY2F0aW9ucyA9IE5VTEw7CgpzdGF0aWMgaW5saW5lIHZvaWQgKnNhd2l0X2JlbGkoc2l6ZV90IGJ5"
    "dGVzKSB7CiAgICBpZiAoYnl0ZXMgPT0gMCkgYnl0ZXMgPSAxOwogICAgdm9pZCAqcG9pbnRlciA9IG1hbGxvYyhi"
    "eXRlcyk7CiAgICBpZiAoIXBvaW50ZXIpIHJldHVybiBOVUxMOwogICAgU2F3aXRBbGxvY2F0aW9uICpyZWNvcmQg"
    "PSAoU2F3aXRBbGxvY2F0aW9uICopbWFsbG9jKHNpemVvZihTYXdpdEFsbG9jYXRpb24pKTsKICAgIGlmICghcmVj"
    "b3JkKSB7CiAgICAgICAgZnJlZShwb2ludGVyKTsKICAgICAgICByZXR1cm4gTlVMTDsKICAgIH0KICAgIHJlY29y"
    "ZC0+cG9pbnRlciA9IHBvaW50ZXI7CiAgICByZWNvcmQtPmJ5dGVzID0gYnl0ZXM7CiAgICByZWNvcmQtPm5leHQg"
    "PSBzYXdpdF9hbGxvY2F0aW9uczsKICAgIHNhd2l0X2FsbG9jYXRpb25zID0gcmVjb3JkOwogICAgcmV0dXJuIHBv"
    "aW50ZXI7Cn0KCnN0YXRpYyBpbmxpbmUgdm9pZCBzYXdpdF9qdWFsKHZvaWQgKnBvaW50ZXIpIHsKICAgIGlmICgh"
    "cG9pbnRlcikgcmV0dXJuOwogICAgU2F3aXRBbGxvY2F0aW9uICoqY3Vyc29yID0gJnNhd2l0X2FsbG9jYXRpb25z"
    "OwogICAgd2hpbGUgKCpjdXJzb3IpIHsKICAgICAgICBpZiAoKCpjdXJzb3IpLT5wb2ludGVyID09IHBvaW50ZXIp"
    "IHsKICAgICAgICAgICAgU2F3aXRBbGxvY2F0aW9uICpyZWNvcmQgPSAqY3Vyc29yOwogICAgICAgICAgICAqY3Vy"
    "c29yID0gcmVjb3JkLT5uZXh0OwogICAgICAgICAgICBmcmVlKHJlY29yZCk7CiAgICAgICAgICAgIGZyZWUocG9p"
    "bnRlcik7CiAgICAgICAgICAgIHJldHVybjsKICAgICAgICB9CiAgICAgICAgY3Vyc29yID0gJigqY3Vyc29yKS0+"
    "bmV4dDsKICAgIH0KICAgIC8qIFBvaW50ZXIgYnVrYW4gaGFzaWwgYmVsaSgpOiBhYmFpa2FuLiBJbmkgbWVuY2Vn"
    "YWggZG91YmxlLWZyZWUKICAgICAgIChqdWFsIGR1YSBrYWxpIHBhZGEgcG9pbnRlciB5YW5nIHNhbWEpIGRhbiBt"
    "ZW5jZWdhaCBmcmVlKCkgcGFkYQogICAgICAgcG9pbnRlciB5YW5nIHRpZGFrIGRpbWlsaWtpIGFsbG9jYXRvciAo"
    "bWlzLiB2YXJpYWJlbCBzdGFjaykuICovCn0KCi8qIHJhbXBhcygpOiBwaW5kYWhrYW4ga2VwZW1pbGlrYW4gcG9p"
    "bnRlciBkYXJpIHNyYyBrZSBkc3QsIGxhbHUga29zb25na2FuIHNyYy4KICogQ2F0YXRhbiBhbG9rYXNpIHRpZGFr"
    "IHBlcmx1IGRpdWJhaCDigJQgaWEgc3VkYWggdGVyaWthdCBwYWRhIG5pbGFpIHBvaW50ZXIuCiAqIFNldGVsYWgg"
    "aW5pIGp1YWwoc3JjKSBqYWRpIG5vLW9wIGRhbiBqdWFsKGRzdCkgbWVtYmViYXNrYW4gdGVwYXQgc2VrYWxpLiAq"
    "LwpzdGF0aWMgaW5saW5lIHZvaWQgc2F3aXRfcmFtcGFzKHZvaWQgKipkc3QsIHZvaWQgKipzcmMpIHsKICAgIGlm"
    "ICghZHN0IHx8ICFzcmMgfHwgZHN0ID09IHNyYykgcmV0dXJuOwogICAgdm9pZCAqcG9pbnRlciA9ICpzcmM7CiAg"
    "ICAqZHN0ID0gcG9pbnRlcjsKICAgICpzcmMgPSBOVUxMOwp9CgojZGVmaW5lIHNhd2l0X255YXdpdChmdW5jdGlv"
    "bikgKChmdW5jdGlvbikoKSkKCiNlbmRpZiAvKiBTQVdJVF9GUkVFU1RBTkRJTkcgKi8KI2VuZGlmIC8qIFNBV0lU"
    "X1JVTlRJTUVfSCAqLwo="
),
    "sawit_kernel.h": (
    "I2lmbmRlZiBTQVdJVF9LRVJORUxfSAojZGVmaW5lIFNBV0lUX0tFUk5FTF9ICgovKiBSdW50aW1lIGZyZWVzdGFu"
    "ZGluZyBTYXdpdC9TKysg4oCUIFRBTlBBIGxpYmMuCiAqIERpYWt0aWZrYW4gb2xlaCAtLWZyZWVzdGFuZGluZy4g"
    "RGl0dWp1a2FuIHVudHVrIGtlcm5lbCB4ODYgMzItYml0CiAqIGRlbmdhbiBvdXRwdXQgVkdBIHRleHQgbW9kZSBk"
    "YW4gZHJpdmVyIGtleWJvYXJkIHBvcnQgMHg2MC4KICogSGVscGVyIGJlcnRlbWEgdGV0YXAgYmVybmFtYSBzYXdp"
    "dF8qIGFnYXIgaGFzaWwgdHJhbnNwaWxlIGlkZW50aWsuCiAqLwoKLyogVGlkYWsgbWVueWVydGFrYW4gPHN0ZGFy"
    "Zy5oPi88c3RkZGVmLmg+IHNlbmdhamE6IHBhZGEgTWluR1cga2VkdWFueWEKICogbWVuYXJpayBjb3JlY3J0Lmgg"
    "eWFuZyBtZW5kZWZpbmlzaWthbiB1aW50cHRyX3QgZGFuIGJlcnRhYnJha2FuIGRlbmdhbgogKiB0eXBlZGVmIGZy"
    "ZWVzdGFuZGluZyBkaSBiYXdhaC4gS2VybmVsIHNlamF0aSB0aWRhayBwdW55YSBsaWJjIHNhbWEgc2VrYWxpLiAq"
    "LwojaWYgZGVmaW5lZChfX0dOVUNfXykgfHwgZGVmaW5lZChfX2NsYW5nX18pCnR5cGVkZWYgX19idWlsdGluX3Zh"
    "X2xpc3Qgc2F3aXRfdmFfbGlzdDsKI2RlZmluZSBzYXdpdF92YV9zdGFydChhcCwgbGFzdCkgX19idWlsdGluX3Zh"
    "X3N0YXJ0KGFwLCBsYXN0KQojZGVmaW5lIHNhd2l0X3ZhX2FyZyhhcCwgdHlwZSkgICBfX2J1aWx0aW5fdmFfYXJn"
    "KGFwLCB0eXBlKQojZGVmaW5lIHNhd2l0X3ZhX2VuZChhcCkgICAgICAgICBfX2J1aWx0aW5fdmFfZW5kKGFwKQoj"
    "ZWxzZQp0eXBlZGVmIHZvaWQgKnNhd2l0X3ZhX2xpc3Q7CiNkZWZpbmUgc2F3aXRfdmFfc3RhcnQoYXAsIGxhc3Qp"
    "ICgodm9pZCkobGFzdCksICh2b2lkKShhcCkpCiNkZWZpbmUgc2F3aXRfdmFfYXJnKGFwLCB0eXBlKSAgICgodHlw"
    "ZSkwKQojZGVmaW5lIHNhd2l0X3ZhX2VuZChhcCkgICAgICAgICAoKHZvaWQpKGFwKSkKI2VuZGlmCgojZGVmaW5l"
    "IE5VTEwgKCh2b2lkICopMCkKCi8qIFRpcGUgbGViYXIgdGV0YXAgdGFucGEgPHN0ZGludC5oPiAoaGVhZGVyIGl0"
    "dSB0aWRhayBmcmVlc3RhbmRpbmcgZGkgc2VtdWEgdG9vbGNoYWluKS4gKi8KI2lmZGVmIF9fVUlOVDhfVFlQRV9f"
    "CnR5cGVkZWYgX19VSU5UOF9UWVBFX18gdWludDhfdDsKI2Vsc2UKdHlwZWRlZiB1bnNpZ25lZCBjaGFyIHVpbnQ4"
    "X3Q7CiNlbmRpZgojaWZkZWYgX19JTlQ4X1RZUEVfXwp0eXBlZGVmIF9fSU5UOF9UWVBFX18gaW50OF90OwojZWxz"
    "ZQp0eXBlZGVmIHNpZ25lZCBjaGFyIGludDhfdDsKI2VuZGlmCiNpZmRlZiBfX1VJTlQxNl9UWVBFX18KdHlwZWRl"
    "ZiBfX1VJTlQxNl9UWVBFX18gdWludDE2X3Q7CiNlbHNlCnR5cGVkZWYgdW5zaWduZWQgc2hvcnQgdWludDE2X3Q7"
    "CiNlbmRpZgojaWZkZWYgX19JTlQxNl9UWVBFX18KdHlwZWRlZiBfX0lOVDE2X1RZUEVfXyBpbnQxNl90OwojZWxz"
    "ZQp0eXBlZGVmIHNpZ25lZCBzaG9ydCBpbnQxNl90OwojZW5kaWYKI2lmZGVmIF9fVUlOVDMyX1RZUEVfXwp0eXBl"
    "ZGVmIF9fVUlOVDMyX1RZUEVfXyB1aW50MzJfdDsKI2Vsc2UKdHlwZWRlZiB1bnNpZ25lZCBpbnQgdWludDMyX3Q7"
    "CiNlbmRpZgojaWZkZWYgX19JTlQzMl9UWVBFX18KdHlwZWRlZiBfX0lOVDMyX1RZUEVfXyBpbnQzMl90OwojZWxz"
    "ZQp0eXBlZGVmIHNpZ25lZCBpbnQgaW50MzJfdDsKI2VuZGlmCiNpZmRlZiBfX1VJTlQ2NF9UWVBFX18KdHlwZWRl"
    "ZiBfX1VJTlQ2NF9UWVBFX18gdWludDY0X3Q7CiNlbHNlCnR5cGVkZWYgdW5zaWduZWQgbG9uZyBsb25nIHVpbnQ2"
    "NF90OwojZW5kaWYKI2lmZGVmIF9fSU5UNjRfVFlQRV9fCnR5cGVkZWYgX19JTlQ2NF9UWVBFX18gaW50NjRfdDsK"
    "I2Vsc2UKdHlwZWRlZiBzaWduZWQgbG9uZyBsb25nIGludDY0X3Q7CiNlbmRpZgojaWZkZWYgX19VSU5UUFRSX1RZ"
    "UEVfXwp0eXBlZGVmIF9fVUlOVFBUUl9UWVBFX18gdWludHB0cl90OwojZWxzZQp0eXBlZGVmIHVuc2lnbmVkIGxv"
    "bmcgdWludHB0cl90OwojZW5kaWYKCi8qIC0tLS0tLS0tLS0gVkdBIHRleHQgbW9kZSAweEI4MDAwIC0tLS0tLS0t"
    "LS0gKi8KI2RlZmluZSBTQVdJVF9WR0FfQkFTRSAweEI4MDAwVUwKI2RlZmluZSBTQVdJVF9WR0FfQ09MUyA4MAoj"
    "ZGVmaW5lIFNBV0lUX1ZHQV9ST1dTIDI1CiNkZWZpbmUgU0FXSVRfVkdBX0FUVFIgMHgwNwoKc3RhdGljIHZvbGF0"
    "aWxlIHVuc2lnbmVkIHNob3J0ICpzYXdpdF92Z2EgPQogICAgKHZvbGF0aWxlIHVuc2lnbmVkIHNob3J0ICopU0FX"
    "SVRfVkdBX0JBU0U7CnN0YXRpYyB1bnNpZ25lZCBpbnQgc2F3aXRfdmdhX3BvcyA9IDA7CgpzdGF0aWMgaW5saW5l"
    "IHZvaWQgc2F3aXRfa2VybmVsX3B1dGModW5zaWduZWQgY2hhciBjKSB7CiAgICBpZiAoYyA9PSAnXG4nKSB7CiAg"
    "ICAgICAgc2F3aXRfdmdhX3BvcyA9IChzYXdpdF92Z2FfcG9zIC8gU0FXSVRfVkdBX0NPTFMgKyAxKSAqIFNBV0lU"
    "X1ZHQV9DT0xTOwogICAgICAgIHJldHVybjsKICAgIH0KICAgIGlmIChjID09ICdccicpIHsKICAgICAgICBzYXdp"
    "dF92Z2FfcG9zIC09IHNhd2l0X3ZnYV9wb3MgJSBTQVdJVF9WR0FfQ09MUzsKICAgICAgICByZXR1cm47CiAgICB9"
    "CiAgICBpZiAoc2F3aXRfdmdhX3BvcyA+PSBTQVdJVF9WR0FfQ09MUyAqIFNBV0lUX1ZHQV9ST1dTKSB7CiAgICAg"
    "ICAgdW5zaWduZWQgaW50IGk7CiAgICAgICAgZm9yIChpID0gMDsgaSA8IChTQVdJVF9WR0FfUk9XUyAtIDEpICog"
    "U0FXSVRfVkdBX0NPTFM7ICsraSkKICAgICAgICAgICAgc2F3aXRfdmdhW2ldID0gc2F3aXRfdmdhW2kgKyBTQVdJ"
    "VF9WR0FfQ09MU107CiAgICAgICAgc2F3aXRfdmdhX3BvcyA9IChTQVdJVF9WR0FfUk9XUyAtIDEpICogU0FXSVRf"
    "VkdBX0NPTFM7CiAgICAgICAgZm9yIChpID0gMDsgaSA8IFNBV0lUX1ZHQV9DT0xTOyArK2kpCiAgICAgICAgICAg"
    "IHNhd2l0X3ZnYVtzYXdpdF92Z2FfcG9zICsgaV0gPSAoU0FXSVRfVkdBX0FUVFIgPDwgOCkgfCAnICc7CiAgICB9"
    "CiAgICBzYXdpdF92Z2Fbc2F3aXRfdmdhX3BvcysrXSA9ICh1bnNpZ25lZCBzaG9ydCkoKFNBV0lUX1ZHQV9BVFRS"
    "IDw8IDgpIHwgYyk7Cn0KCnN0YXRpYyBpbmxpbmUgdm9pZCBzYXdpdF90dWxpc19sYXlhcihjb25zdCBjaGFyICp0"
    "ZXh0KSB7CiAgICBpZiAoIXRleHQpIHJldHVybjsKICAgIHdoaWxlICgqdGV4dCkgc2F3aXRfa2VybmVsX3B1dGMo"
    "KHVuc2lnbmVkIGNoYXIpKnRleHQrKyk7Cn0KCnN0YXRpYyBpbmxpbmUgdm9pZCBzYXdpdF9iZXJzaWhrYW5fbGF5"
    "YXIodm9pZCkgewogICAgdW5zaWduZWQgaW50IGk7CiAgICBmb3IgKGkgPSAwOyBpIDwgU0FXSVRfVkdBX0NPTFMg"
    "KiBTQVdJVF9WR0FfUk9XUzsgKytpKQogICAgICAgIHNhd2l0X3ZnYVtpXSA9IChTQVdJVF9WR0FfQVRUUiA8PCA4"
    "KSB8ICcgJzsKICAgIHNhd2l0X3ZnYV9wb3MgPSAwOwp9CgovKiBQYWthaSAzMi1iaXQ6IHBlbWJhZ2lhbiA2NC1i"
    "aXQgYnV0dWggX191ZGl2ZGkzL19fdW1vZGRpMyBkYXJpIGxpYmdjYywKICogeWFuZyB0aWRhayB0ZXJzZWRpYSBw"
    "YWRhIGxpbmsgLW5vc3RkbGliLiBTZW11YSBuaWxhaSBwcmludGYga2VybmVsCiAqIChpbnQvdW5zaWduZWQvcG9p"
    "bnRlcikgbXVhdCBkaSAzMiBiaXQgcGFkYSB0YXJnZXQgaTM4Ni4gKi8Kc3RhdGljIGlubGluZSB2b2lkIHNhd2l0"
    "X2tlcm5lbF9wcmludF91aW50KHVuc2lnbmVkIGludCB2YWx1ZSwKICAgICAgICAgICAgICAgICAgICAgICAgICAg"
    "ICAgICAgICAgICAgICAgIHVuc2lnbmVkIGludCBiYXNlLCBpbnQgdXBwZXIpIHsKICAgIGNoYXIgZGlnaXRzWzI0"
    "XTsKICAgIGludCB1c2VkID0gMDsKICAgIGlmICh2YWx1ZSA9PSAwKSB7CiAgICAgICAgc2F3aXRfa2VybmVsX3B1"
    "dGMoJzAnKTsKICAgICAgICByZXR1cm47CiAgICB9CiAgICB3aGlsZSAodmFsdWUpIHsKICAgICAgICB1bnNpZ25l"
    "ZCBpbnQgZGlnaXQgPSAodW5zaWduZWQgaW50KSh2YWx1ZSAlIGJhc2UpOwogICAgICAgIGRpZ2l0c1t1c2VkKytd"
    "ID0gKGNoYXIpKGRpZ2l0IDwgMTAKICAgICAgICAgICAgPyAnMCcgKyBkaWdpdAogICAgICAgICAgICA6ICh1cHBl"
    "ciA/ICdBJyA6ICdhJykgKyBkaWdpdCAtIDEwKTsKICAgICAgICB2YWx1ZSAvPSBiYXNlOwogICAgfQogICAgd2hp"
    "bGUgKHVzZWQpIHNhd2l0X2tlcm5lbF9wdXRjKCh1bnNpZ25lZCBjaGFyKWRpZ2l0c1stLXVzZWRdKTsKfQoKc3Rh"
    "dGljIGlubGluZSB2b2lkIHNhd2l0X2tlcm5lbF9idXp6ZXJfdmEoY29uc3QgY2hhciAqZm9ybWF0LCBzYXdpdF92"
    "YV9saXN0IGFyZ3MpIHsKICAgIGNvbnN0IGNoYXIgKnAgPSBmb3JtYXQ7CiAgICBpZiAoIXApIHJldHVybjsKICAg"
    "IHdoaWxlICgqcCkgewogICAgICAgIGlmICgqcCAhPSAnJScpIHsKICAgICAgICAgICAgc2F3aXRfa2VybmVsX3B1"
    "dGMoKHVuc2lnbmVkIGNoYXIpKnArKyk7CiAgICAgICAgICAgIGNvbnRpbnVlOwogICAgICAgIH0KICAgICAgICAr"
    "K3A7CiAgICAgICAgaWYgKCEqcCkgYnJlYWs7CiAgICAgICAgc3dpdGNoICgqcCkgewogICAgICAgIGNhc2UgJ3Mn"
    "OiB7CiAgICAgICAgICAgIGNvbnN0IGNoYXIgKnRleHQgPSBzYXdpdF92YV9hcmcoYXJncywgY29uc3QgY2hhciAq"
    "KTsKICAgICAgICAgICAgc2F3aXRfdHVsaXNfbGF5YXIodGV4dCA/IHRleHQgOiAiKG51bGwpIik7CiAgICAgICAg"
    "ICAgIGJyZWFrOwogICAgICAgIH0KICAgICAgICBjYXNlICdjJzoKICAgICAgICAgICAgc2F3aXRfa2VybmVsX3B1"
    "dGMoKHVuc2lnbmVkIGNoYXIpc2F3aXRfdmFfYXJnKGFyZ3MsIGludCkpOwogICAgICAgICAgICBicmVhazsKICAg"
    "ICAgICBjYXNlICdkJzoKICAgICAgICBjYXNlICdpJzogewogICAgICAgICAgICBpbnQgdmFsdWUgPSBzYXdpdF92"
    "YV9hcmcoYXJncywgaW50KTsKICAgICAgICAgICAgaWYgKHZhbHVlIDwgMCkgewogICAgICAgICAgICAgICAgc2F3"
    "aXRfa2VybmVsX3B1dGMoJy0nKTsKICAgICAgICAgICAgICAgIC8qIEhpbmRhcmkgbmVnYXNpIElOVF9NSU4geWFu"
    "ZyBvdmVyZmxvdy4gKi8KICAgICAgICAgICAgICAgIHNhd2l0X2tlcm5lbF9wcmludF91aW50KCh1bnNpZ25lZCBp"
    "bnQpKC0odmFsdWUgKyAxKSkgKyAxdSwgMTAsIDApOwogICAgICAgICAgICB9IGVsc2UgewogICAgICAgICAgICAg"
    "ICAgc2F3aXRfa2VybmVsX3ByaW50X3VpbnQoKHVuc2lnbmVkIGludCl2YWx1ZSwgMTAsIDApOwogICAgICAgICAg"
    "ICB9CiAgICAgICAgICAgIGJyZWFrOwogICAgICAgIH0KICAgICAgICBjYXNlICd1JzoKICAgICAgICAgICAgc2F3"
    "aXRfa2VybmVsX3ByaW50X3VpbnQoc2F3aXRfdmFfYXJnKGFyZ3MsIHVuc2lnbmVkIGludCksIDEwLCAwKTsKICAg"
    "ICAgICAgICAgYnJlYWs7CiAgICAgICAgY2FzZSAneCc6CiAgICAgICAgICAgIHNhd2l0X2tlcm5lbF9wcmludF91"
    "aW50KHNhd2l0X3ZhX2FyZyhhcmdzLCB1bnNpZ25lZCBpbnQpLCAxNiwgMCk7CiAgICAgICAgICAgIGJyZWFrOwog"
    "ICAgICAgIGNhc2UgJ1gnOgogICAgICAgICAgICBzYXdpdF9rZXJuZWxfcHJpbnRfdWludChzYXdpdF92YV9hcmco"
    "YXJncywgdW5zaWduZWQgaW50KSwgMTYsIDEpOwogICAgICAgICAgICBicmVhazsKICAgICAgICBjYXNlICdwJzoK"
    "ICAgICAgICAgICAgc2F3aXRfa2VybmVsX3B1dGMoJzAnKTsKICAgICAgICAgICAgc2F3aXRfa2VybmVsX3B1dGMo"
    "J3gnKTsKICAgICAgICAgICAgc2F3aXRfa2VybmVsX3ByaW50X3VpbnQoKHVuc2lnbmVkIGludCkodWludHB0cl90"
    "KXNhd2l0X3ZhX2FyZyhhcmdzLCB2b2lkICopLCAxNiwgMCk7CiAgICAgICAgICAgIGJyZWFrOwogICAgICAgIGNh"
    "c2UgJyUnOgogICAgICAgICAgICBzYXdpdF9rZXJuZWxfcHV0YygnJScpOwogICAgICAgICAgICBicmVhazsKICAg"
    "ICAgICBkZWZhdWx0OgogICAgICAgICAgICBzYXdpdF9rZXJuZWxfcHV0YygnJScpOwogICAgICAgICAgICBzYXdp"
    "dF9rZXJuZWxfcHV0YygodW5zaWduZWQgY2hhcikqcCk7CiAgICAgICAgfQogICAgICAgICsrcDsKICAgIH0KfQoK"
    "LyogTW9kZSBzZW55YXAgbWVtYWthaSBzdHJlYW0gc2VtdTogc3Rkb3V0PTEsIHN0ZGVycj0yLiAqLwp0eXBlZGVm"
    "IGludCBTYXdpdFN0cmVhbTsKI2RlZmluZSBzdGRvdXQgMQojZGVmaW5lIHN0ZGVyciAyCnN0YXRpYyBpbnQgc2F3"
    "aXRfc3Rkb3V0X2J1bmdrYW0gPSAwOwpzdGF0aWMgaW50IHNhd2l0X3N0ZGVycl9idW5na2FtID0gMDsKc3RhdGlj"
    "IGludCBzYXdpdF9zdGRpbl9idW5na2FtID0gMDsKCnN0YXRpYyBpbmxpbmUgdm9pZCBzYXdpdF9idW5na2FtKFNh"
    "d2l0U3RyZWFtIHN0cmVhbSkgewogICAgaWYgKHN0cmVhbSA9PSBzdGRvdXQpIHNhd2l0X3N0ZG91dF9idW5na2Ft"
    "ID0gMTsKICAgIGlmIChzdHJlYW0gPT0gc3RkZXJyKSBzYXdpdF9zdGRlcnJfYnVuZ2thbSA9IDE7Cn0Kc3RhdGlj"
    "IGlubGluZSB2b2lkIHNhd2l0X2J1a2Ffc3VhcmEoU2F3aXRTdHJlYW0gc3RyZWFtKSB7CiAgICBpZiAoc3RyZWFt"
    "ID09IHN0ZG91dCkgc2F3aXRfc3Rkb3V0X2J1bmdrYW0gPSAwOwogICAgaWYgKHN0cmVhbSA9PSBzdGRlcnIpIHNh"
    "d2l0X3N0ZGVycl9idW5na2FtID0gMDsKfQpzdGF0aWMgaW5saW5lIHZvaWQgc2F3aXRfdHV0dXBfdGVsaW5nYSh2"
    "b2lkKSB7IHNhd2l0X3N0ZGluX2J1bmdrYW0gPSAxOyB9CnN0YXRpYyBpbmxpbmUgdm9pZCBzYXdpdF9idWthX3Rl"
    "bGluZ2Eodm9pZCkgIHsgc2F3aXRfc3RkaW5fYnVuZ2thbSA9IDA7IH0KCnN0YXRpYyBpbmxpbmUgdm9pZCBzYXdp"
    "dF9idXp6ZXIoY29uc3QgY2hhciAqZm9ybWF0LCAuLi4pIHsKICAgIGlmIChzYXdpdF9zdGRvdXRfYnVuZ2thbSkg"
    "cmV0dXJuOwogICAgc2F3aXRfdmFfbGlzdCBhcmdzOwogICAgc2F3aXRfdmFfc3RhcnQoYXJncywgZm9ybWF0KTsK"
    "ICAgIHNhd2l0X2tlcm5lbF9idXp6ZXJfdmEoZm9ybWF0LCBhcmdzKTsKICAgIHNhd2l0X3ZhX2VuZChhcmdzKTsK"
    "fQoKc3RhdGljIGlubGluZSB2b2lkIHNhd2l0X2RlbW9fbWFzeWFyYWthdChjb25zdCBjaGFyICpmb3JtYXQsIC4u"
    "LikgewogICAgaWYgKHNhd2l0X3N0ZGVycl9idW5na2FtKSByZXR1cm47CiAgICBzYXdpdF90dWxpc19sYXlhcigi"
    "W0RFTU8gTUFTWUFSQUtBVF0gIik7CiAgICBzYXdpdF92YV9saXN0IGFyZ3M7CiAgICBzYXdpdF92YV9zdGFydChh"
    "cmdzLCBmb3JtYXQpOwogICAgc2F3aXRfa2VybmVsX2J1enplcl92YShmb3JtYXQsIGFyZ3MpOwogICAgc2F3aXRf"
    "dmFfZW5kKGFyZ3MpOwogICAgc2F3aXRfa2VybmVsX3B1dGMoJ1xuJyk7Cn0KCi8qIC0tLS0tLS0tLS0gU3RyaW5n"
    "ICYga29udmVyc2kgKHRhbnBhIHN0cmluZy5oKSAtLS0tLS0tLS0tICovCnN0YXRpYyBpbmxpbmUgaW50IHNhd2l0"
    "X3NhbWFfa2F0YShjb25zdCBjaGFyICphLCBjb25zdCBjaGFyICpiKSB7CiAgICBpZiAoIWEgfHwgIWIpIHJldHVy"
    "biBhID09IGI7CiAgICB3aGlsZSAoKmEgJiYgKmIgJiYgKmEgPT0gKmIpIHsgKythOyArK2I7IH0KICAgIHJldHVy"
    "biAqYSA9PSAqYjsKfQpzdGF0aWMgaW5saW5lIHVuc2lnbmVkIGxvbmcgc2F3aXRfcGFuamFuZ19rYXRhKGNvbnN0"
    "IGNoYXIgKnRleHQpIHsKICAgIHVuc2lnbmVkIGxvbmcgbiA9IDA7CiAgICBpZiAoIXRleHQpIHJldHVybiAwOwog"
    "ICAgd2hpbGUgKHRleHRbbl0pICsrbjsKICAgIHJldHVybiBuOwp9CnN0YXRpYyBpbmxpbmUgdm9pZCBzYXdpdF9z"
    "YWxpbl9rYXRhKGNoYXIgKmRzdCwgY29uc3QgY2hhciAqc3JjKSB7CiAgICBpZiAoIWRzdCB8fCAhc3JjKSByZXR1"
    "cm47CiAgICB3aGlsZSAoKCpkc3QrKyA9ICpzcmMrKykpIHsgfQp9CnN0YXRpYyBpbmxpbmUgaW50IHNhd2l0X3Rl"
    "cmplbWFoX2FuZ2thKGNvbnN0IGNoYXIgKnRleHQpIHsKICAgIGludCB2YWx1ZSA9IDAsIHNpZ24gPSAxOwogICAg"
    "aWYgKCF0ZXh0KSByZXR1cm4gMDsKICAgIHdoaWxlICgqdGV4dCA9PSAnICcgfHwgKnRleHQgPT0gJ1x0JykgKyt0"
    "ZXh0OwogICAgaWYgKCp0ZXh0ID09ICctJykgeyBzaWduID0gLTE7ICsrdGV4dDsgfQogICAgZWxzZSBpZiAoKnRl"
    "eHQgPT0gJysnKSArK3RleHQ7CiAgICB3aGlsZSAoKnRleHQgPj0gJzAnICYmICp0ZXh0IDw9ICc5JykgdmFsdWUg"
    "PSB2YWx1ZSAqIDEwICsgKCp0ZXh0KysgLSAnMCcpOwogICAgcmV0dXJuIHZhbHVlICogc2lnbjsKfQoKLyogLS0t"
    "LS0tLS0tLSBJbnB1dCAtLS0tLS0tLS0tICovCi8qIEZvcm1hdHRlZCBzY2FuZiBiZWx1bSB0ZXJzZWRpYSBkaSBr"
    "ZXJuZWw6IGtlbWJhbGlrYW4gLTEgYmlsYSBkaXBhbmdnaWwuICovCnN0YXRpYyBpbmxpbmUgaW50IHNhd2l0X3Nh"
    "ZGFwKGNvbnN0IGNoYXIgKmZvcm1hdCwgLi4uKSB7CiAgICAodm9pZClmb3JtYXQ7CiAgICByZXR1cm4gc2F3aXRf"
    "c3RkaW5fYnVuZ2thbSA/IC0xIDogLTE7Cn0KLyogQmFyaXMga29zb25nIHNhbXBhaSBkcml2ZXIga2V5Ym9hcmQg"
    "YnVmZmVyLWRyaXZlbiBkaXRhbWJhaGthbi4gKi8Kc3RhdGljIGlubGluZSBjaGFyICpzYXdpdF9zYWRhcF9iYXJp"
    "cyhjaGFyICpidWZmZXIsIHVuc2lnbmVkIGxvbmcgc2l6ZSkgewogICAgaWYgKCFidWZmZXIgfHwgc2l6ZSA9PSAw"
    "KSByZXR1cm4gKGNoYXIgKikwOwogICAgYnVmZmVyWzBdID0gJ1wwJzsKICAgIHJldHVybiBidWZmZXI7Cn0Kc3Rh"
    "dGljIGlubGluZSB2b2lkIHNhd2l0X3NhZGFwX3Npc2Eodm9pZCkgeyB9CnN0YXRpYyBpbmxpbmUgaW50IHNhd2l0"
    "X3NhZGFwX2thcmFrdGVyKHZvaWQpOwoKLyogLS0tLS0tLS0tLSBQb3J0IEkvTyAmIGtvbnRyb2wgQ1BVICh4ODYs"
    "IEdDQy9DbGFuZykgLS0tLS0tLS0tLSAqLwojaWYgZGVmaW5lZChfX0dOVUNfXykgfHwgZGVmaW5lZChfX2NsYW5n"
    "X18pCnN0YXRpYyBpbmxpbmUgdWludDhfdCBzYXdpdF9pbmIodWludDE2X3QgcG9ydCkgewogICAgdWludDhfdCB2"
    "YWx1ZTsKICAgIF9fYXNtX18gdm9sYXRpbGUgKCJpbiAlJWR4LCAlJWFsIiA6ICI9YSIodmFsdWUpIDogImQiKHBv"
    "cnQpKTsKICAgIHJldHVybiB2YWx1ZTsKfQpzdGF0aWMgaW5saW5lIHZvaWQgc2F3aXRfb3V0Yih1aW50MTZfdCBw"
    "b3J0LCB1aW50OF90IHZhbHVlKSB7CiAgICBfX2FzbV9fIHZvbGF0aWxlICgib3V0ICUlYWwsICUlZHgiIDogOiAi"
    "YSIodmFsdWUpLCAiZCIocG9ydCkpOwp9CnN0YXRpYyBpbmxpbmUgdm9pZCBzYXdpdF9jbGkodm9pZCkgeyBfX2Fz"
    "bV9fIHZvbGF0aWxlICgiY2xpIik7IH0Kc3RhdGljIGlubGluZSB2b2lkIHNhd2l0X3N0aSh2b2lkKSB7IF9fYXNt"
    "X18gdm9sYXRpbGUgKCJzdGkiKTsgfQpzdGF0aWMgaW5saW5lIHZvaWQgc2F3aXRfaGx0KHZvaWQpIHsgX19hc21f"
    "XyB2b2xhdGlsZSAoImhsdCIpOyB9CiNlbHNlCnN0YXRpYyBpbmxpbmUgdWludDhfdCBzYXdpdF9pbmIodWludDE2"
    "X3QgcG9ydCkgeyAodm9pZClwb3J0OyByZXR1cm4gMDsgfQpzdGF0aWMgaW5saW5lIHZvaWQgc2F3aXRfb3V0Yih1"
    "aW50MTZfdCBwb3J0LCB1aW50OF90IHZhbHVlKSB7ICh2b2lkKXBvcnQ7ICh2b2lkKXZhbHVlOyB9CnN0YXRpYyBp"
    "bmxpbmUgdm9pZCBzYXdpdF9jbGkodm9pZCkgeyB9CnN0YXRpYyBpbmxpbmUgdm9pZCBzYXdpdF9zdGkodm9pZCkg"
    "eyB9CnN0YXRpYyBpbmxpbmUgdm9pZCBzYXdpdF9obHQodm9pZCkgeyB9CiNlbmRpZgoKLyogU2FkYXAgbWVudGFo"
    "OiBiYWNhIHBvcnQgMHg2MCwgbWVuZ2VtYmFsaWthbiBzY2FuY29kZSAoYnVrYW4gQVNDSUkpLiAqLwpzdGF0aWMg"
    "aW5saW5lIGludCBzYXdpdF9zYWRhcF9rYXJha3Rlcih2b2lkKSB7CiAgICBpZiAoc2F3aXRfc3RkaW5fYnVuZ2th"
    "bSkgcmV0dXJuIC0xOwogICAgaWYgKHNhd2l0X2luYigweDY0KSAmIDF1KSByZXR1cm4gKGludClzYXdpdF9pbmIo"
    "MHg2MCk7CiAgICByZXR1cm4gLTE7Cn0KCi8qIC0tLS0tLS0tLS0gTGVkZ2VyICYgYWxva2F0b3IgbWFuZGlyaSAt"
    "LS0tLS0tLS0tICovCiNkZWZpbmUgc2F3aXRfcGFqYWsodmFsdWUsIHBlcmNlbnQpICgodmFsdWUpIC09ICgodmFs"
    "dWUpICogKHBlcmNlbnQpKSAvIDEwMCkKc3RhdGljIGludCBzYXdpdF9hbmdnYXJhbiA9IDA7CnN0YXRpYyBpbmxp"
    "bmUgaW50IHNhd2l0X21iZyhpbnQgYW1vdW50KSB7CiAgICBpZiAoYW1vdW50ID4gMCkgc2F3aXRfYW5nZ2FyYW4g"
    "Kz0gYW1vdW50OwogICAgcmV0dXJuIGFtb3VudDsKfQpzdGF0aWMgaW5saW5lIGludCBzYXdpdF9jYWlyKGludCBh"
    "bW91bnQpIHsKICAgIGlmIChhbW91bnQgPD0gMCB8fCBhbW91bnQgPiBzYXdpdF9hbmdnYXJhbikgcmV0dXJuIDA7"
    "CiAgICBzYXdpdF9hbmdnYXJhbiAtPSBhbW91bnQ7CiAgICByZXR1cm4gYW1vdW50Owp9CgojZGVmaW5lIFNBV0lU"
    "X0tFUk5FTF9IRUFQX1NJWkUgNjU1MzYKc3RhdGljIHVuc2lnbmVkIGNoYXIgc2F3aXRfa2VybmVsX2hlYXBbU0FX"
    "SVRfS0VSTkVMX0hFQVBfU0laRV07CnN0YXRpYyB1aW50cHRyX3Qgc2F3aXRfa2VybmVsX2hlYXBfcG9zID0gMDsK"
    "CnN0YXRpYyBpbmxpbmUgdm9pZCAqc2F3aXRfYmVsaSh1bnNpZ25lZCBsb25nIGJ5dGVzKSB7CiAgICB1aW50cHRy"
    "X3QgYWxpZ25lZDsKICAgIGlmIChieXRlcyA9PSAwKSBieXRlcyA9IDE7CiAgICBhbGlnbmVkID0gKHNhd2l0X2tl"
    "cm5lbF9oZWFwX3BvcyArIDd1KSAmIH4oKHVpbnRwdHJfdCk3dSk7CiAgICBpZiAoYWxpZ25lZCArICh1aW50cHRy"
    "X3QpYnl0ZXMgPiBTQVdJVF9LRVJORUxfSEVBUF9TSVpFKSByZXR1cm4gKHZvaWQgKikwOwogICAgc2F3aXRfa2Vy"
    "bmVsX2hlYXBfcG9zID0gYWxpZ25lZCArICh1aW50cHRyX3QpYnl0ZXM7CiAgICByZXR1cm4gKHZvaWQgKikoc2F3"
    "aXRfa2VybmVsX2hlYXAgKyBhbGlnbmVkKTsKfQovKiBCdW1wIGFsbG9jYXRvciBiZWx1bSBiaXNhIG1lbmdlbWJh"
    "bGlrYW4gYmxvazsganVhbCBzZW5nYWphIG5vLW9wLiAqLwpzdGF0aWMgaW5saW5lIHZvaWQgc2F3aXRfanVhbCh2"
    "b2lkICpwb2ludGVyKSB7ICh2b2lkKXBvaW50ZXI7IH0KCi8qIHJhbXBhcygpOiB0cmFuc2ZlciBwb2ludGVyIGFu"
    "dGFyIHZhcmlhYmVsLCBzdW1iZXIgZGlrb3NvbmdrYW4uICovCnN0YXRpYyBpbmxpbmUgdm9pZCBzYXdpdF9yYW1w"
    "YXModm9pZCAqKmRzdCwgdm9pZCAqKnNyYykgewogICAgaWYgKCFkc3QgfHwgIXNyYyB8fCBkc3QgPT0gc3JjKSBy"
    "ZXR1cm47CiAgICB2b2lkICpwb2ludGVyID0gKnNyYzsKICAgICpkc3QgPSBwb2ludGVyOwogICAgKnNyYyA9IE5V"
    "TEw7Cn0KCiNkZWZpbmUgc2F3aXRfbnlhd2l0KGZ1bmN0aW9uKSAoKGZ1bmN0aW9uKSgpKQoKLyogZysrIG1lbnlp"
    "c2lwa2FuIHBhbmdnaWxhbiBfX21haW4oKSB1bnR1ayBjb25zdHJ1Y3RvciBnbG9iYWwuIFRhbnBhIENSVCwKICog"
    "c2ltYm9sIGl0dSBoYXJ1cyBkaXNlZGlha2FuIHNlbmRpcmkgc3VwYXlhIHRhdXRhbiAtbm9zdGRsaWIgYmVyaGFz"
    "aWwuICovCiNpZiBkZWZpbmVkKF9fY3BsdXNwbHVzKQpleHRlcm4gIkMiIHZvaWQgX19tYWluKHZvaWQpIHsgfQoj"
    "ZW5kaWYKCiNlbmRpZiAvKiBTQVdJVF9LRVJORUxfSCAqLwo="
),
    "start.S": (
    "LyogTXVsdGlib290IDIgaGVhZGVyICsgZW50cnkgcG9pbnQgdW50dWsga2VybmVsIFNhd2l0L1MrKy4KICogRGly"
    "YWtpdCBvbGVoIGdjYyAtbTMyIChHQVMpLCBidWthbiBOQVNNLiBOQVNNIGRpcGFrYWkgbmFudGkgdW50dWsgYm9v"
    "dCBzZWN0b3IuICovCi5leHRlcm4gbWFpbgoKLnNlY3Rpb24gLm11bHRpYm9vdCwgImEiCi5hbGlnbiA0Ci8qIE11"
    "bHRpYm9vdCAxOiBkaWtlbmFsaSBRRU1VIC1rZXJuZWwgZGFuIEdSVUIgIm11bHRpYm9vdCIuCiAgIGZsYWdzPTAg"
    "4oaSIHRpZGFrIGJ1dHVoIGZpZWxkIHRhbWJhaGFuOyBlbnRyeSBkaWFtYmlsIGRhcmkgRUxGLiAqLwoubG9uZyAw"
    "eDFCQURCMDAyCi5sb25nIDAKLmxvbmcgLSgweDFCQURCMDAyKQoKLyogTXVsdGlib290IDI6IGRpcGFrYWkgR1JV"
    "QiAibXVsdGlib290MiIgYmlsYSBkaWluZ2lua2FuLiAqLwouYWxpZ24gNAptdWx0aWJvb3RfaGVhZGVyX3N0YXJ0"
    "OgogICAgLmxvbmcgMHhFODUyNTBENiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAvKiBtYWdpYyBN"
    "dWx0aWJvb3QgMiAqLwogICAgLmxvbmcgMCAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAg"
    "ICAvKiBhcnNpdGVrdHVyIGkzODYgKi8KICAgIC5sb25nIG11bHRpYm9vdF9oZWFkZXJfZW5kIC0gbXVsdGlib290"
    "X2hlYWRlcl9zdGFydCAvKiBwYW5qYW5nIGhlYWRlciAqLwogICAgLmxvbmcgLSgweEU4NTI1MEQ2ICsgKG11bHRp"
    "Ym9vdF9oZWFkZXJfZW5kIC0gbXVsdGlib290X2hlYWRlcl9zdGFydCkpIC8qIGNoZWNrc3VtICovCiAgICAvKiB0"
    "YWcgYWtoaXIgKi8KICAgIC5zaG9ydCAwCiAgICAuc2hvcnQgMAogICAgLmxvbmcgOAptdWx0aWJvb3RfaGVhZGVy"
    "X2VuZDoKCi5zZWN0aW9uIC50ZXh0Ci5nbG9iYWwgX3N0YXJ0Ci50eXBlIF9zdGFydCwgQGZ1bmN0aW9uCl9zdGFy"
    "dDoKICAgIG1vdiAkc3RhY2tfdG9wLCAlZXNwCiAgICBjYWxsIG1haW4KMToKICAgIGNsaQogICAgaGx0CiAgICBq"
    "bXAgMWIKLnNpemUgX3N0YXJ0LCAuIC0gX3N0YXJ0Cgouc2VjdGlvbiAuYnNzCi5hbGlnbiAxNgpzdGFja19ib3R0"
    "b206CiAgICAuc2tpcCAxNjM4NApzdGFja190b3A6Cg=="
),
    "linker.ld": (
    "LyogTGlua2VyIHNjcmlwdCB1bnR1ayBrZXJuZWwgU2F3aXQvUysrIChNdWx0aWJvb3QgMiwgaTM4NikuICovCkVO"
    "VFJZKF9zdGFydCkKClNFQ1RJT05TCnsKICAgIC8qIEltYWdlIGRpbXVhdCBkaSAxTS4gSGVhZGVyIE11bHRpYm9v"
    "dCBIQVJVUyBtZW5qYWRpIHNlY3Rpb24gcGVydGFtYQogICAgICAgYWdhciBtZW5kYXJhdCBkaSBvZmZzZXQgZmls"
    "ZSAweDEwMDAgKFFFTVUgaGFueWEgbWVtaW5kYWkgOEtCIGF3YWwpLgogICAgICAgX3N0YXJ0IG1lbmdpa3V0aSBo"
    "ZWFkZXIgKH4yNCBieXRlKSwgamFkaSBlbnRyeSBzZWRpa2l0IGRpIGF0YXMgMHgxMDAwMDAuICovCiAgICAuID0g"
    "MU07CgogICAgLm11bHRpYm9vdCA6IEFMSUdOKDQpCiAgICB7CiAgICAgICAgKigubXVsdGlib290KQogICAgfQoK"
    "ICAgIC50ZXh0IDoKICAgIHsKICAgICAgICAqKC50ZXh0KQogICAgICAgICooLnRleHQuKikKICAgIH0KCiAgICAu"
    "cm9kYXRhIDogeyAqKC5yb2RhdGEqKSB9CiAgICAuZGF0YSAgIDogeyAqKC5kYXRhKikgfQoKICAgIC5ic3MgOgog"
    "ICAgewogICAgICAgICooLmJzcyopCiAgICAgICAgKihDT01NT04pCiAgICB9CgogICAgL0RJU0NBUkQvIDoKICAg"
    "IHsKICAgICAgICAqKC5laF9mcmFtZSkKICAgICAgICAqKC5ub3RlLkdOVS1zdGFjaykKICAgICAgICAqKC5jb21t"
    "ZW50KQogICAgfQp9Cg=="
),
}


def _asset_bytes(name: str) -> bytes:
    """Ambil aset runtime/boot dari dalam skrip (tanpa file eksternal)."""
    return _base64.b64decode("".join(_EMBEDDED_ASSETS[name]))

KEYWORDS = {
    "tandan": "int",
    "pertalite": "float",
    "pertamax": "double",
    "buah": "char",
    "pelepah": "std::string",
    "panen": "bool",
    "rodok": "bool",
    "hutan": "void",
    # Tipe lebar tetap & qualifier (dipakai driver/MMIO).
    "keping8": "uint8_t",
    "keping16": "uint16_t",
    "keping32": "uint32_t",
    "keping64": "uint64_t",
    "alamat": "uintptr_t",
    "takTanda": "unsigned",
    "menguap": "volatile",
    "tetap": "const",
    "statis": "static",
    "luar": "extern",
    "segera": "inline",
    "kosong": "NULL",
    "seberapa": "sizeof",
    "rumpun": "enum",
    "serikat": "union",
    "sebutan": "typedef",
    "kalauPanen": "if",
    "kalauTidak": "else",
    "rapat": "switch",
    "kasus": "case",
    "muter": "for",
    "selama": "while",
    "pidato": "do",
    "janji": "while",
    "pulang": "break",
    "lanjutPidato": "continue",
    "lanjutOmonOmon": "continue",
    "setor": "return",
    "perkebunan": "class",
    "kopdes": "struct",
    "jalanProduksi": "public",
    "lahanPribadi": "private",
    "lahanPejabat": "protected",
    "wilayahKebun": "namespace",
    "imporBibit": "#include",
    "RUU": "#define",
    "PerPres": "#define",
    "bakarHutan": "new",
    "gusur": "delete",
    "sesuai": "true",
    "hoaks": "false",
    "relokasi": "goto",
    "panenUtama": "main",
}

FUNCTIONS = {
    "buzzer": "sawit_buzzer",
    "bungkam": "sawit_bungkam",
    "bukaSuara": "sawit_buka_suara",
    "demoMasyarakat": "sawit_demo_masyarakat",
    "pajak": "sawit_pajak",
    "MBG": "sawit_mbg",
    "nyawit": "sawit_nyawit",
    "cair": "sawit_cair",
    "beli": "sawit_beli",
    "jual": "sawit_jual",
    "rampas": "sawit_rampas",
    "sita": "sawit_rampas",
    "bersihkanLayar": "sawit_bersihkan_layar",
    # Input — "sadap" menyadap stdin / port keyboard.
    "sadap": "sawit_sadap",
    "sadapBaris": "sawit_sadap_baris",
    "sadapKarakter": "sawit_sadap_karakter",
    "sadapSisa": "sawit_sadap_sisa",
    "tutupTelinga": "sawit_tutup_telinga",
    "bukaTelinga": "sawit_buka_telinga",
    # String & konversi.
    "samaKata": "sawit_sama_kata",
    "panjangKata": "sawit_panjang_kata",
    "terjemahAngka": "sawit_terjemah_angka",
    "salinKata": "sawit_salin_kata",
    # Primitif kernel (x86, GCC/Clang).
    "semprot": "sawit_outb",
    "hisap": "sawit_inb",
    "kunciInterupsi": "sawit_cli",
    "bukaInterupsi": "sawit_sti",
    "hentiMesin": "sawit_hlt",
    "tulisLayar": "sawit_tulis_layar",
}

BUILD_FILE_ALIASES = {
    "AMDAL.txt": "CMakeLists.txt",
    "SuratIzinTanam": "Makefile",
    "ProposalKebun": "meson.build",
    "ProyekStrategisNasional": "BUILD.bazel",
    "IzinTambang.txt": "conanfile.txt",
    "antekModal.json": "vcpkg.json",
}

TOKEN = re.compile(
    r'(?P<comment>//[^\n]*|/\*.*?\*/)|'
    r'(?P<string>"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\')|'
    r'(?P<identifier>[A-Za-z_]\w*)|(?P<other>.)',
    re.DOTALL,
)

AMDAL_TEMPLATE = """cmake_minimum_required(VERSION 3.20)
project(SawitProject C)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(SAWIT_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/main.sawit)
set(SAWIT_GENERATED ${CMAKE_CURRENT_BINARY_DIR}/main.c)

add_custom_command(
    OUTPUT ${SAWIT_GENERATED}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/sawit_transpiler.py transpile ${SAWIT_SOURCE} -o ${SAWIT_GENERATED}
    DEPENDS ${SAWIT_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/sawit_transpiler.py ${CMAKE_CURRENT_SOURCE_DIR}/sawit_runtime.h
    VERBATIM
)
add_executable(panen ${SAWIT_GENERATED})
target_include_directories(panen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
"""

REQUIRED_AMDAL = (
    "cmake_minimum_required",
    "project(",
    "add_custom_command",
    "add_executable",
    "target_include_directories",
)


def _read_source(path: Path) -> str:
    """Read source verbatim (no newline translation) to match the Go version."""
    with open(path, "r", encoding="utf-8", newline="") as handle:
        return handle.read()


def _write_stdout(text: str) -> None:
    sys.stdout.buffer.write(text.encode("utf-8"))
    sys.stdout.buffer.flush()


def transpile(source: str, freestanding: bool = False) -> str:
    """Translate identifiers while preserving comments and literals."""
    chunks: list[str] = []
    if freestanding:
        chunks.append("#define SAWIT_FREESTANDING\n")
    chunks.append('#include "sawit_runtime.h"\n\n')
    for match in TOKEN.finditer(source):
        kind = match.lastgroup
        value = match.group(0)
        if kind == "identifier":
            value = KEYWORDS.get(value, FUNCTIONS.get(value, value))
        chunks.append(value)
    return "".join(chunks)


def _compiler_for(source: Path, requested: str | None) -> str:
    if requested:
        return requested
    candidates = ("g++", "clang++", "gcc", "clang") if source.suffix == ".sawitpp" else ("gcc", "clang", "g++", "clang++")
    for candidate in candidates:
        if shutil.which(candidate):
            return candidate
    raise SystemExit("Compiler tidak ditemukan. Pasang GCC/Clang atau gunakan --compiler.")


def build(
    source: Path,
    output: Path | None,
    compiler: str | None,
    keep_generated: bool,
    freestanding: bool = False,
    multiboot: bool = False,
) -> int:
    if multiboot and os.name == "nt" and compiler is None:
        print(
            "sawit: mode --multiboot menghasilkan ELF32; gunakan WSL/Linux "
            "atau berikan --compiler cross-toolchain (contoh: i686-elf-gcc).",
            file=sys.stderr,
        )
        return 1
    compiler_path = _compiler_for(source, compiler)
    generated_suffix = ".cpp" if source.suffix == ".sawitpp" else ".c"
    tmp_dir = Path(tempfile.mkdtemp(prefix=".sawit-build-", dir=source.parent))
    try:
        generated_path = tmp_dir / ("program" + generated_suffix)
        generated_path.write_bytes(
            transpile(_read_source(source), freestanding or multiboot).encode("utf-8")
        )
        for name in ("sawit_runtime.h", "sawit_kernel.h"):
            (tmp_dir / name).write_bytes(_asset_bytes(name))
        includes = ["-I", str(tmp_dir)]
        start_path: Path | None = None
        linker_path: Path | None = None
        if multiboot:
            for name in ("start.S", "linker.ld"):
                (tmp_dir / name).write_bytes(_asset_bytes(name))
            start_path = tmp_dir / "start.S"
            linker_path = tmp_dir / "linker.ld"
        suffix = ".elf" if multiboot else (".exe" if os.name == "nt" else "")
        target = output or source.with_suffix(suffix)
        if multiboot:
            command = [
                compiler_path, "-m32", "-ffreestanding", "-fno-builtin",
                "-fno-stack-protector", "-fno-pie", "-no-pie", "-fno-rtti",
                "-fno-exceptions", "-nostdlib",
                *includes, "-T", str(linker_path),
                str(start_path), str(generated_path), "-lgcc", "-o", str(target),
            ]
        else:
            command = [compiler_path, *includes, str(generated_path)]
            if freestanding:
                command += [
                    "-ffreestanding", "-fno-builtin", "-fno-stack-protector",
                    "-fno-pie", "-no-pie", "-nostdlib", "-Wl,-e,main",
                ]
            command += ["-o", str(target)]
        completed = subprocess.run(command, text=True, capture_output=True)
        if completed.stdout:
            print(completed.stdout, end="")
        if completed.stderr:
            print(completed.stderr, end="", file=sys.stderr)
        return completed.returncode
    finally:
        if not keep_generated:
            shutil.rmtree(tmp_dir, ignore_errors=True)

def run_program(source: Path, compiler: str | None, program_args: list[str]) -> int:
    """Compile to a temporary executable, run it, then remove temporary files."""
    suffix = ".exe" if os.name == "nt" else ""
    # Keep the temporary executable beside the source: some Windows toolchains
    # cannot link into the system temp directory under restricted permissions.
    descriptor, executable_name = tempfile.mkstemp(
        prefix=".sawit-run-", suffix=suffix, dir=source.parent
    )
    os.close(descriptor)
    executable = Path(executable_name)
    executable.unlink()
    try:
        result = build(source, executable, compiler, keep_generated=False)
        if result != 0:
            return result
        completed = subprocess.run([str(executable), *program_args])
        return completed.returncode
    finally:
        if executable.exists():
            executable.unlink()


def init_project(directory: Path, force: bool) -> int:
    """Create a self-contained starter Sawit project."""
    directory = directory.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    marker = directory / ".sawit-project"
    if marker.exists() and not force:
        print(f"Proyek Sawit sudah ada: {directory}")
        print("Gunakan --force jika ingin menimpa file starter.")
        return 1

    (directory / "src").mkdir(exist_ok=True)
    (directory / "build").mkdir(exist_ok=True)
    with open(directory / "src" / "main.sawit", "w", encoding="utf-8", newline="") as handle:
        handle.write(
            'imporBibit <stdio.h>\n\n'
            'tandan panenUtama() {\n'
            '    buzzer("Halo dari proyek Sawit!\\n");\n'
            '    setor 0;\n'
            '}\n'
        )
    (directory / "sawit_runtime.h").write_bytes(_asset_bytes("sawit_runtime.h"))
    (directory / "sawit_kernel.h").write_bytes(_asset_bytes("sawit_kernel.h"))
    (directory / "boot").mkdir(exist_ok=True)
    for asset in ("start.S", "linker.ld"):
        (directory / "boot" / asset).write_bytes(_asset_bytes(asset))
    (directory / "sawit_transpiler.py").write_text(
        Path(__file__).resolve().read_text(encoding="utf-8"), encoding="utf-8"
    )
    (directory / "AMDAL.txt").write_text(AMDAL_TEMPLATE, encoding="utf-8")
    # CMake itself only discovers CMakeLists.txt. This tiny compatibility
    # shim keeps AMDAL.txt as the real project definition.
    (directory / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.20)\n"
        "project(SawitProject C)\n"
        "# Compatibility shim; edit AMDAL.txt instead.\n"
        "include(${CMAKE_CURRENT_LIST_DIR}/AMDAL.txt)\n",
        encoding="utf-8",
    )
    marker.write_text("Sawit project v0.1\n", encoding="utf-8")
    print(f"Proyek Sawit dibuat di {directory}")
    print("Entry point: src/main.sawit")
    print("Build system utama: AMDAL.txt")
    print("Alias build system:")
    for sawit_name, upstream_name in BUILD_FILE_ALIASES.items():
        print(f"  {sawit_name} -> {upstream_name}")
    return 0


def audit(path: Path | None, fix: bool) -> int:
    """Check (and optionally rewrite) an AMDAL.txt build definition."""
    target = path or Path("AMDAL.txt")
    data = target.read_text(encoding="utf-8")
    missing = [item for item in REQUIRED_AMDAL if item not in data]
    if not missing:
        print("AMDAL.txt: struktur valid.")
    else:
        print(f"AMDAL.txt: bagian kurang: {', '.join(missing)}")
    if fix:
        backup = Path(str(target) + ".bak")
        backup.write_bytes(data.encode("utf-8"))
        target.write_text(AMDAL_TEMPLATE, encoding="utf-8")
        print(f"AMDAL.txt dirapikan; cadangan: {backup}")
    if missing and not fix:
        print("sawit: audit menemukan struktur AMDAL yang belum lengkap", file=sys.stderr)
        return 1
    return 0


def main() -> int:
    # Backward-compatible shorthand: sawit_transpiler.py file.sawit
    known_commands = {"transpile", "build", "run", "init", "audit", "-h", "--help"}
    if len(sys.argv) == 2 and sys.argv[1] not in known_commands:
        candidate = Path(sys.argv[1])
        if candidate.is_file():
            _write_stdout(transpile(_read_source(candidate)))
            return 0

    parser = argparse.ArgumentParser(description="Transpile dan compile Sawit/S++.")
    subparsers = parser.add_subparsers(dest="command")

    transpile_parser = subparsers.add_parser("transpile", help="hasilkan C/C++")
    transpile_parser.add_argument("source", type=Path)
    transpile_parser.add_argument("-o", "--output", type=Path)
    transpile_parser.add_argument("--freestanding", action="store_true")
    transpile_parser.add_argument("--multiboot", action="store_true")

    build_parser = subparsers.add_parser("build", help="transpile lalu compile")
    build_parser.add_argument("source", type=Path)
    build_parser.add_argument("-o", "--output", type=Path)
    build_parser.add_argument("--compiler")
    build_parser.add_argument("--keep-generated", action="store_true")
    build_parser.add_argument("--freestanding", action="store_true")
    build_parser.add_argument("--multiboot", action="store_true")

    run_parser = subparsers.add_parser("run", help="compile dan jalankan tanpa menyimpan executable")
    run_parser.add_argument("source", type=Path)
    run_parser.add_argument("--compiler")
    run_parser.add_argument("--freestanding", action="store_true")
    run_parser.add_argument("--multiboot", action="store_true")
    run_parser.add_argument("program_args", nargs=argparse.REMAINDER)

    init_parser = subparsers.add_parser("init", help="buat proyek Sawit baru")
    init_parser.add_argument("directory", nargs="?", type=Path, default=Path.cwd())
    init_parser.add_argument("--force", action="store_true")

    audit_parser = subparsers.add_parser("audit", help="periksa struktur AMDAL.txt")
    audit_parser.add_argument("path", nargs="?", type=Path)
    audit_parser.add_argument("--fix", action="store_true")

    args = parser.parse_args()

    if args.command == "build":
        return build(args.source, args.output, args.compiler, args.keep_generated,
                     args.freestanding, args.multiboot)
    if args.command == "run":
        if args.freestanding or args.multiboot:
            print("sawit: mode kernel tidak bisa di-run; gunakan build", file=sys.stderr)
            return 1
        return run_program(args.source, args.compiler, args.program_args)
    if args.command == "init":
        return init_project(args.directory, args.force)
    if args.command == "audit":
        return audit(args.path, args.fix)
    if args.command == "transpile":
        result = transpile(_read_source(args.source), args.freestanding or args.multiboot)
        if args.output:
            args.output.write_bytes(result.encode("utf-8"))
        else:
            _write_stdout(result)
        return 0
    parser.print_help()
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
