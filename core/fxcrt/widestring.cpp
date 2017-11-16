// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcrt/widestring.h"

#include <stddef.h>

#include <algorithm>
#include <cctype>
#include <cwctype>

#include "core/fxcrt/cfx_utf8decoder.h"
#include "core/fxcrt/fx_codepage.h"
#include "core/fxcrt/fx_extension.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/string_pool_template.h"
#include "third_party/base/numerics/safe_math.h"
#include "third_party/base/stl_util.h"

template class fxcrt::StringDataTemplate<wchar_t>;
template class fxcrt::StringViewTemplate<wchar_t>;
template class fxcrt::StringPoolTemplate<WideString>;
template struct std::hash<WideString>;

#define FORCE_ANSI 0x10000
#define FORCE_UNICODE 0x20000
#define FORCE_INT64 0x40000

namespace {

const wchar_t* FX_wcsstr(const wchar_t* haystack,
                         int haystack_len,
                         const wchar_t* needle,
                         int needle_len) {
  if (needle_len > haystack_len || needle_len == 0) {
    return nullptr;
  }
  const wchar_t* end_ptr = haystack + haystack_len - needle_len;
  while (haystack <= end_ptr) {
    int i = 0;
    while (1) {
      if (haystack[i] != needle[i]) {
        break;
      }
      i++;
      if (i == needle_len) {
        return haystack;
      }
    }
    haystack++;
  }
  return nullptr;
}

pdfium::Optional<size_t> GuessSizeForVSWPrintf(const wchar_t* pFormat,
                                               va_list argList) {
  size_t nMaxLen = 0;
  for (const wchar_t* pStr = pFormat; *pStr != 0; pStr++) {
    if (*pStr != '%' || *(pStr = pStr + 1) == '%') {
      ++nMaxLen;
      continue;
    }
    int nItemLen = 0;
    int nWidth = 0;
    for (; *pStr != 0; pStr++) {
      if (*pStr == '#') {
        nMaxLen += 2;
      } else if (*pStr == '*') {
        nWidth = va_arg(argList, int);
      } else if (*pStr != '-' && *pStr != '+' && *pStr != '0' && *pStr != ' ') {
        break;
      }
    }
    if (nWidth == 0) {
      nWidth = FXSYS_wtoi(pStr);
      while (std::iswdigit(*pStr))
        ++pStr;
    }
    if (nWidth < 0 || nWidth > 128 * 1024)
      return pdfium::Optional<size_t>();
    int nPrecision = 0;
    if (*pStr == '.') {
      pStr++;
      if (*pStr == '*') {
        nPrecision = va_arg(argList, int);
        pStr++;
      } else {
        nPrecision = FXSYS_wtoi(pStr);
        while (std::iswdigit(*pStr))
          ++pStr;
      }
    }
    if (nPrecision < 0 || nPrecision > 128 * 1024)
      return pdfium::Optional<size_t>();
    int nModifier = 0;
    if (*pStr == L'I' && *(pStr + 1) == L'6' && *(pStr + 2) == L'4') {
      pStr += 3;
      nModifier = FORCE_INT64;
    } else {
      switch (*pStr) {
        case 'h':
          nModifier = FORCE_ANSI;
          pStr++;
          break;
        case 'l':
          nModifier = FORCE_UNICODE;
          pStr++;
          break;
        case 'F':
        case 'N':
        case 'L':
          pStr++;
          break;
      }
    }
    switch (*pStr | nModifier) {
      case 'c':
      case 'C':
        nItemLen = 2;
        va_arg(argList, int);
        break;
      case 'c' | FORCE_ANSI:
      case 'C' | FORCE_ANSI:
        nItemLen = 2;
        va_arg(argList, int);
        break;
      case 'c' | FORCE_UNICODE:
      case 'C' | FORCE_UNICODE:
        nItemLen = 2;
        va_arg(argList, int);
        break;
      case 's': {
        const wchar_t* pstrNextArg = va_arg(argList, const wchar_t*);
        if (pstrNextArg) {
          nItemLen = wcslen(pstrNextArg);
          if (nItemLen < 1) {
            nItemLen = 1;
          }
        } else {
          nItemLen = 6;
        }
      } break;
      case 'S': {
        const char* pstrNextArg = va_arg(argList, const char*);
        if (pstrNextArg) {
          nItemLen = strlen(pstrNextArg);
          if (nItemLen < 1) {
            nItemLen = 1;
          }
        } else {
          nItemLen = 6;
        }
      } break;
      case 's' | FORCE_ANSI:
      case 'S' | FORCE_ANSI: {
        const char* pstrNextArg = va_arg(argList, const char*);
        if (pstrNextArg) {
          nItemLen = strlen(pstrNextArg);
          if (nItemLen < 1) {
            nItemLen = 1;
          }
        } else {
          nItemLen = 6;
        }
      } break;
      case 's' | FORCE_UNICODE:
      case 'S' | FORCE_UNICODE: {
        const wchar_t* pstrNextArg = va_arg(argList, wchar_t*);
        if (pstrNextArg) {
          nItemLen = wcslen(pstrNextArg);
          if (nItemLen < 1) {
            nItemLen = 1;
          }
        } else {
          nItemLen = 6;
        }
      } break;
    }
    if (nItemLen != 0) {
      if (nPrecision != 0 && nItemLen > nPrecision) {
        nItemLen = nPrecision;
      }
      if (nItemLen < nWidth) {
        nItemLen = nWidth;
      }
    } else {
      switch (*pStr) {
        case 'd':
        case 'i':
        case 'u':
        case 'x':
        case 'X':
        case 'o':
          if (nModifier & FORCE_INT64) {
            va_arg(argList, int64_t);
          } else {
            va_arg(argList, int);
          }
          nItemLen = 32;
          if (nItemLen < nWidth + nPrecision) {
            nItemLen = nWidth + nPrecision;
          }
          break;
        case 'a':
        case 'A':
        case 'e':
        case 'E':
        case 'g':
        case 'G':
          va_arg(argList, double);
          nItemLen = 128;
          if (nItemLen < nWidth + nPrecision) {
            nItemLen = nWidth + nPrecision;
          }
          break;
        case 'f':
          if (nWidth + nPrecision > 100) {
            nItemLen = nPrecision + nWidth + 128;
          } else {
            double f;
            char pszTemp[256];
            f = va_arg(argList, double);
            FXSYS_snprintf(pszTemp, sizeof(pszTemp), "%*.*f", nWidth,
                           nPrecision + 6, f);
            nItemLen = strlen(pszTemp);
          }
          break;
        case 'p':
          va_arg(argList, void*);
          nItemLen = 32;
          if (nItemLen < nWidth + nPrecision) {
            nItemLen = nWidth + nPrecision;
          }
          break;
        case 'n':
          va_arg(argList, int*);
          break;
      }
    }
    nMaxLen += nItemLen;
  }
  nMaxLen += 32;  // Fudge factor.
  return pdfium::Optional<size_t>(nMaxLen);
}

// Returns string unless we ran out of space.
pdfium::Optional<WideString> TryVSWPrintf(size_t size,
                                          const wchar_t* pFormat,
                                          va_list argList) {
  WideString str;
  wchar_t* buffer = str.GetBuffer(size);

  // In the following two calls, there's always space in the buffer for
  // a terminating NUL that's not included in nMaxLen.
  // For vswprintf(), MSAN won't untaint the buffer on a truncated write's
  // -1 return code even though the buffer is written. Probably just as well
  // not to trust the vendor's implementation to write anything anyways.
  // See https://crbug.com/705912.
  memset(buffer, 0, (size + 1) * sizeof(wchar_t));
  int ret = vswprintf(buffer, size + 1, pFormat, argList);

  bool bSufficientBuffer = ret >= 0 || buffer[size - 1] == 0;
  if (!bSufficientBuffer)
    return {};

  str.ReleaseBuffer(str.GetStringLength());
  return {str};
}

#ifndef NDEBUG
bool IsValidCodePage(uint16_t codepage) {
  switch (codepage) {
    case FX_CODEPAGE_DefANSI:
    case FX_CODEPAGE_ShiftJIS:
    case FX_CODEPAGE_ChineseSimplified:
    case FX_CODEPAGE_Hangul:
    case FX_CODEPAGE_ChineseTraditional:
      return true;
    default:
      return false;
  }
}
#endif

WideString GetWideString(uint16_t codepage, const ByteStringView& bstr) {
  ASSERT(IsValidCodePage(codepage));

  int src_len = bstr.GetLength();
  int dest_len = FXSYS_MultiByteToWideChar(
      codepage, 0, bstr.unterminated_c_str(), src_len, nullptr, 0);
  if (!dest_len)
    return WideString();

  WideString wstr;
  wchar_t* dest_buf = wstr.GetBuffer(dest_len);
  FXSYS_MultiByteToWideChar(codepage, 0, bstr.unterminated_c_str(), src_len,
                            dest_buf, dest_len);
  wstr.ReleaseBuffer(dest_len);
  return wstr;
}

}  // namespace

namespace fxcrt {

static_assert(sizeof(WideString) <= sizeof(wchar_t*),
              "Strings must not require more space than pointers");

// static
WideString WideString::FormatV(const wchar_t* format, va_list argList) {
  va_list argListCopy;
  va_copy(argListCopy, argList);
  int maxLen = vswprintf(nullptr, 0, format, argListCopy);
  va_end(argListCopy);

  if (maxLen <= 0) {
    va_copy(argListCopy, argList);
    auto guess = GuessSizeForVSWPrintf(format, argListCopy);
    va_end(argListCopy);

    if (!guess.has_value())
      return L"";
    maxLen = pdfium::base::checked_cast<int>(guess.value());
  }

  while (maxLen < 32 * 1024) {
    va_copy(argListCopy, argList);
    pdfium::Optional<WideString> ret =
        TryVSWPrintf(static_cast<size_t>(maxLen), format, argListCopy);
    va_end(argListCopy);

    if (ret)
      return *ret;
    maxLen *= 2;
  }
  return L"";
}

// static
WideString WideString::Format(const wchar_t* pFormat, ...) {
  va_list argList;
  va_start(argList, pFormat);
  WideString ret = FormatV(pFormat, argList);
  va_end(argList);
  return ret;
}

WideString::WideString() {}

WideString::WideString(const WideString& other) : m_pData(other.m_pData) {}

WideString::WideString(WideString&& other) noexcept {
  m_pData.Swap(other.m_pData);
}

WideString::WideString(const wchar_t* pStr, size_t nLen) {
  if (nLen)
    m_pData.Reset(StringData::Create(pStr, nLen));
}

WideString::WideString(wchar_t ch) {
  m_pData.Reset(StringData::Create(1));
  m_pData->m_String[0] = ch;
}

WideString::WideString(const wchar_t* ptr)
    : WideString(ptr, ptr ? wcslen(ptr) : 0) {}

WideString::WideString(const WideStringView& stringSrc) {
  if (!stringSrc.IsEmpty()) {
    m_pData.Reset(StringData::Create(stringSrc.unterminated_c_str(),
                                     stringSrc.GetLength()));
  }
}

WideString::WideString(const WideStringView& str1, const WideStringView& str2) {
  FX_SAFE_SIZE_T nSafeLen = str1.GetLength();
  nSafeLen += str2.GetLength();

  size_t nNewLen = nSafeLen.ValueOrDie();
  if (nNewLen == 0)
    return;

  m_pData.Reset(StringData::Create(nNewLen));
  m_pData->CopyContents(str1.unterminated_c_str(), str1.GetLength());
  m_pData->CopyContentsAt(str1.GetLength(), str2.unterminated_c_str(),
                          str2.GetLength());
}

WideString::WideString(const std::initializer_list<WideStringView>& list) {
  FX_SAFE_SIZE_T nSafeLen = 0;
  for (const auto& item : list)
    nSafeLen += item.GetLength();

  size_t nNewLen = nSafeLen.ValueOrDie();
  if (nNewLen == 0)
    return;

  m_pData.Reset(StringData::Create(nNewLen));

  size_t nOffset = 0;
  for (const auto& item : list) {
    m_pData->CopyContentsAt(nOffset, item.unterminated_c_str(),
                            item.GetLength());
    nOffset += item.GetLength();
  }
}

WideString::~WideString() {}

const WideString& WideString::operator=(const wchar_t* pStr) {
  if (!pStr || !pStr[0])
    clear();
  else
    AssignCopy(pStr, wcslen(pStr));

  return *this;
}

const WideString& WideString::operator=(const WideStringView& stringSrc) {
  if (stringSrc.IsEmpty())
    clear();
  else
    AssignCopy(stringSrc.unterminated_c_str(), stringSrc.GetLength());

  return *this;
}

const WideString& WideString::operator=(const WideString& stringSrc) {
  if (m_pData != stringSrc.m_pData)
    m_pData = stringSrc.m_pData;

  return *this;
}

const WideString& WideString::operator+=(const wchar_t* pStr) {
  if (pStr)
    Concat(pStr, wcslen(pStr));

  return *this;
}

const WideString& WideString::operator+=(wchar_t ch) {
  Concat(&ch, 1);
  return *this;
}

const WideString& WideString::operator+=(const WideString& str) {
  if (str.m_pData)
    Concat(str.m_pData->m_String, str.m_pData->m_nDataLength);

  return *this;
}

const WideString& WideString::operator+=(const WideStringView& str) {
  if (!str.IsEmpty())
    Concat(str.unterminated_c_str(), str.GetLength());

  return *this;
}

bool WideString::operator==(const wchar_t* ptr) const {
  if (!m_pData)
    return !ptr || !ptr[0];

  if (!ptr)
    return m_pData->m_nDataLength == 0;

  return wcslen(ptr) == m_pData->m_nDataLength &&
         wmemcmp(ptr, m_pData->m_String, m_pData->m_nDataLength) == 0;
}

bool WideString::operator==(const WideStringView& str) const {
  if (!m_pData)
    return str.IsEmpty();

  return m_pData->m_nDataLength == str.GetLength() &&
         wmemcmp(m_pData->m_String, str.unterminated_c_str(),
                 str.GetLength()) == 0;
}

bool WideString::operator==(const WideString& other) const {
  if (m_pData == other.m_pData)
    return true;

  if (IsEmpty())
    return other.IsEmpty();

  if (other.IsEmpty())
    return false;

  return other.m_pData->m_nDataLength == m_pData->m_nDataLength &&
         wmemcmp(other.m_pData->m_String, m_pData->m_String,
                 m_pData->m_nDataLength) == 0;
}

bool WideString::operator<(const wchar_t* ptr) const {
  if (!m_pData && !ptr)
    return false;
  if (c_str() == ptr)
    return false;

  size_t len = GetLength();
  size_t other_len = ptr ? wcslen(ptr) : 0;
  int result = wmemcmp(c_str(), ptr, std::min(len, other_len));
  return result < 0 || (result == 0 && len < other_len);
}

bool WideString::operator<(const WideStringView& str) const {
  if (!m_pData && !str.unterminated_c_str())
    return false;
  if (c_str() == str.unterminated_c_str())
    return false;

  size_t len = GetLength();
  size_t other_len = str.GetLength();
  int result =
      wmemcmp(c_str(), str.unterminated_c_str(), std::min(len, other_len));
  return result < 0 || (result == 0 && len < other_len);
}

bool WideString::operator<(const WideString& other) const {
  if (m_pData == other.m_pData)
    return false;

  size_t len = GetLength();
  size_t other_len = other.GetLength();
  int result = wmemcmp(c_str(), other.c_str(), std::min(len, other_len));
  return result < 0 || (result == 0 && len < other_len);
}

void WideString::AssignCopy(const wchar_t* pSrcData, size_t nSrcLen) {
  AllocBeforeWrite(nSrcLen);
  m_pData->CopyContents(pSrcData, nSrcLen);
  m_pData->m_nDataLength = nSrcLen;
}

void WideString::ReallocBeforeWrite(size_t nNewLength) {
  if (m_pData && m_pData->CanOperateInPlace(nNewLength))
    return;

  if (nNewLength == 0) {
    clear();
    return;
  }

  RetainPtr<StringData> pNewData(StringData::Create(nNewLength));
  if (m_pData) {
    size_t nCopyLength = std::min(m_pData->m_nDataLength, nNewLength);
    pNewData->CopyContents(m_pData->m_String, nCopyLength);
    pNewData->m_nDataLength = nCopyLength;
  } else {
    pNewData->m_nDataLength = 0;
  }
  pNewData->m_String[pNewData->m_nDataLength] = 0;
  m_pData.Swap(pNewData);
}

void WideString::AllocBeforeWrite(size_t nNewLength) {
  if (m_pData && m_pData->CanOperateInPlace(nNewLength))
    return;

  if (nNewLength == 0) {
    clear();
    return;
  }

  m_pData.Reset(StringData::Create(nNewLength));
}

void WideString::ReleaseBuffer(size_t nNewLength) {
  if (!m_pData)
    return;

  nNewLength = std::min(nNewLength, m_pData->m_nAllocLength);
  if (nNewLength == 0) {
    clear();
    return;
  }

  ASSERT(m_pData->m_nRefs == 1);
  m_pData->m_nDataLength = nNewLength;
  m_pData->m_String[nNewLength] = 0;
  if (m_pData->m_nAllocLength - nNewLength >= 32) {
    // Over arbitrary threshold, so pay the price to relocate.  Force copy to
    // always occur by holding a second reference to the string.
    WideString preserve(*this);
    ReallocBeforeWrite(nNewLength);
  }
}

void WideString::Reserve(size_t len) {
  GetBuffer(len);
}

wchar_t* WideString::GetBuffer(size_t nMinBufLength) {
  if (!m_pData) {
    if (nMinBufLength == 0)
      return nullptr;

    m_pData.Reset(StringData::Create(nMinBufLength));
    m_pData->m_nDataLength = 0;
    m_pData->m_String[0] = 0;
    return m_pData->m_String;
  }

  if (m_pData->CanOperateInPlace(nMinBufLength))
    return m_pData->m_String;

  nMinBufLength = std::max(nMinBufLength, m_pData->m_nDataLength);
  if (nMinBufLength == 0)
    return nullptr;

  RetainPtr<StringData> pNewData(StringData::Create(nMinBufLength));
  pNewData->CopyContents(*m_pData);
  pNewData->m_nDataLength = m_pData->m_nDataLength;
  m_pData.Swap(pNewData);
  return m_pData->m_String;
}

size_t WideString::Delete(size_t index, size_t count) {
  if (!m_pData)
    return 0;

  size_t old_length = m_pData->m_nDataLength;
  if (count == 0 ||
      index != pdfium::clamp(index, static_cast<size_t>(0), old_length))
    return old_length;

  size_t removal_length = index + count;
  if (removal_length > old_length)
    return old_length;

  ReallocBeforeWrite(old_length);
  size_t chars_to_copy = old_length - removal_length + 1;
  wmemmove(m_pData->m_String + index, m_pData->m_String + removal_length,
           chars_to_copy);
  m_pData->m_nDataLength = old_length - count;
  return m_pData->m_nDataLength;
}

void WideString::Concat(const wchar_t* pSrcData, size_t nSrcLen) {
  if (!pSrcData || nSrcLen == 0)
    return;

  if (!m_pData) {
    m_pData.Reset(StringData::Create(pSrcData, nSrcLen));
    return;
  }

  if (m_pData->CanOperateInPlace(m_pData->m_nDataLength + nSrcLen)) {
    m_pData->CopyContentsAt(m_pData->m_nDataLength, pSrcData, nSrcLen);
    m_pData->m_nDataLength += nSrcLen;
    return;
  }

  RetainPtr<StringData> pNewData(
      StringData::Create(m_pData->m_nDataLength + nSrcLen));
  pNewData->CopyContents(*m_pData);
  pNewData->CopyContentsAt(m_pData->m_nDataLength, pSrcData, nSrcLen);
  m_pData.Swap(pNewData);
}

ByteString WideString::UTF8Encode() const {
  return FX_UTF8Encode(AsStringView());
}

ByteString WideString::UTF16LE_Encode() const {
  if (!m_pData) {
    return ByteString("\0\0", 2);
  }
  int len = m_pData->m_nDataLength;
  ByteString result;
  char* buffer = result.GetBuffer(len * 2 + 2);
  for (int i = 0; i < len; i++) {
    buffer[i * 2] = m_pData->m_String[i] & 0xff;
    buffer[i * 2 + 1] = m_pData->m_String[i] >> 8;
  }
  buffer[len * 2] = 0;
  buffer[len * 2 + 1] = 0;
  result.ReleaseBuffer(len * 2 + 2);
  return result;
}

WideString WideString::Mid(size_t first, size_t count) const {
  if (!m_pData)
    return WideString();

  if (!IsValidIndex(first))
    return WideString();

  if (count == 0 || !IsValidLength(count))
    return WideString();

  if (!IsValidIndex(first + count - 1))
    return WideString();

  if (first == 0 && count == GetLength())
    return *this;

  WideString dest;
  AllocCopy(dest, count, first);
  return dest;
}

WideString WideString::Left(size_t count) const {
  if (count == 0 || !IsValidLength(count))
    return WideString();
  return Mid(0, count);
}

WideString WideString::Right(size_t count) const {
  if (count == 0 || !IsValidLength(count))
    return WideString();
  return Mid(GetLength() - count, count);
}

void WideString::AllocCopy(WideString& dest,
                           size_t nCopyLen,
                           size_t nCopyIndex) const {
  if (nCopyLen == 0)
    return;

  RetainPtr<StringData> pNewData(
      StringData::Create(m_pData->m_String + nCopyIndex, nCopyLen));
  dest.m_pData.Swap(pNewData);
}

size_t WideString::Insert(size_t location, wchar_t ch) {
  const size_t cur_length = m_pData ? m_pData->m_nDataLength : 0;
  if (!IsValidLength(location))
    return cur_length;

  const size_t new_length = cur_length + 1;
  ReallocBeforeWrite(new_length);
  wmemmove(m_pData->m_String + location + 1, m_pData->m_String + location,
           new_length - location);
  m_pData->m_String[location] = ch;
  m_pData->m_nDataLength = new_length;
  return new_length;
}

pdfium::Optional<size_t> WideString::Find(wchar_t ch, size_t start) const {
  if (!m_pData)
    return pdfium::Optional<size_t>();

  if (!IsValidIndex(start))
    return pdfium::Optional<size_t>();

  const wchar_t* pStr =
      wmemchr(m_pData->m_String + start, ch, m_pData->m_nDataLength - start);
  return pStr ? pdfium::Optional<size_t>(
                    static_cast<size_t>(pStr - m_pData->m_String))
              : pdfium::Optional<size_t>();
}

pdfium::Optional<size_t> WideString::Find(const WideStringView& subStr,
                                          size_t start) const {
  if (!m_pData)
    return pdfium::Optional<size_t>();

  if (!IsValidIndex(start))
    return pdfium::Optional<size_t>();

  const wchar_t* pStr =
      FX_wcsstr(m_pData->m_String + start, m_pData->m_nDataLength - start,
                subStr.unterminated_c_str(), subStr.GetLength());
  return pStr ? pdfium::Optional<size_t>(
                    static_cast<size_t>(pStr - m_pData->m_String))
              : pdfium::Optional<size_t>();
}

void WideString::MakeLower() {
  if (!m_pData)
    return;

  ReallocBeforeWrite(m_pData->m_nDataLength);
  FXSYS_wcslwr(m_pData->m_String);
}

void WideString::MakeUpper() {
  if (!m_pData)
    return;

  ReallocBeforeWrite(m_pData->m_nDataLength);
  FXSYS_wcsupr(m_pData->m_String);
}

size_t WideString::Remove(wchar_t chRemove) {
  if (!m_pData || m_pData->m_nDataLength < 1)
    return 0;

  wchar_t* pstrSource = m_pData->m_String;
  wchar_t* pstrEnd = m_pData->m_String + m_pData->m_nDataLength;
  while (pstrSource < pstrEnd) {
    if (*pstrSource == chRemove)
      break;
    pstrSource++;
  }
  if (pstrSource == pstrEnd)
    return 0;

  ptrdiff_t copied = pstrSource - m_pData->m_String;
  ReallocBeforeWrite(m_pData->m_nDataLength);
  pstrSource = m_pData->m_String + copied;
  pstrEnd = m_pData->m_String + m_pData->m_nDataLength;

  wchar_t* pstrDest = pstrSource;
  while (pstrSource < pstrEnd) {
    if (*pstrSource != chRemove) {
      *pstrDest = *pstrSource;
      pstrDest++;
    }
    pstrSource++;
  }

  *pstrDest = 0;
  size_t count = static_cast<size_t>(pstrSource - pstrDest);
  m_pData->m_nDataLength -= count;
  return count;
}

size_t WideString::Replace(const WideStringView& pOld,
                           const WideStringView& pNew) {
  if (!m_pData || pOld.IsEmpty())
    return 0;

  size_t nSourceLen = pOld.GetLength();
  size_t nReplacementLen = pNew.GetLength();
  size_t count = 0;
  const wchar_t* pStart = m_pData->m_String;
  wchar_t* pEnd = m_pData->m_String + m_pData->m_nDataLength;
  while (1) {
    const wchar_t* pTarget =
        FX_wcsstr(pStart, static_cast<size_t>(pEnd - pStart),
                  pOld.unterminated_c_str(), nSourceLen);
    if (!pTarget)
      break;

    count++;
    pStart = pTarget + nSourceLen;
  }
  if (count == 0)
    return 0;

  size_t nNewLength =
      m_pData->m_nDataLength + (nReplacementLen - nSourceLen) * count;

  if (nNewLength == 0) {
    clear();
    return count;
  }

  RetainPtr<StringData> pNewData(StringData::Create(nNewLength));
  pStart = m_pData->m_String;
  wchar_t* pDest = pNewData->m_String;
  for (size_t i = 0; i < count; i++) {
    const wchar_t* pTarget =
        FX_wcsstr(pStart, static_cast<size_t>(pEnd - pStart),
                  pOld.unterminated_c_str(), nSourceLen);
    wmemcpy(pDest, pStart, pTarget - pStart);
    pDest += pTarget - pStart;
    wmemcpy(pDest, pNew.unterminated_c_str(), pNew.GetLength());
    pDest += pNew.GetLength();
    pStart = pTarget + nSourceLen;
  }
  wmemcpy(pDest, pStart, pEnd - pStart);
  m_pData.Swap(pNewData);
  return count;
}

// static
WideString WideString::FromLocal(const ByteStringView& str) {
  return FromCodePage(str, 0);
}

// static
WideString WideString::FromCodePage(const ByteStringView& str,
                                    uint16_t codepage) {
  return GetWideString(codepage, str);
}

// static
WideString WideString::FromUTF8(const ByteStringView& str) {
  if (str.IsEmpty())
    return WideString();

  CFX_UTF8Decoder decoder;
  for (size_t i = 0; i < str.GetLength(); i++)
    decoder.Input(str[i]);

  return WideString(decoder.GetResult());
}

// static
WideString WideString::FromUTF16LE(const unsigned short* wstr, size_t wlen) {
  if (!wstr || wlen == 0) {
    return WideString();
  }

  WideString result;
  wchar_t* buf = result.GetBuffer(wlen);
  for (size_t i = 0; i < wlen; i++) {
    buf[i] = wstr[i];
  }
  result.ReleaseBuffer(wlen);
  return result;
}

void WideString::SetAt(size_t index, wchar_t c) {
  ASSERT(IsValidIndex(index));
  ReallocBeforeWrite(m_pData->m_nDataLength);
  m_pData->m_String[index] = c;
}

int WideString::Compare(const wchar_t* lpsz) const {
  if (m_pData)
    return lpsz ? wcscmp(m_pData->m_String, lpsz) : 1;
  return (!lpsz || lpsz[0] == 0) ? 0 : -1;
}

int WideString::Compare(const WideString& str) const {
  if (!m_pData)
    return str.m_pData ? -1 : 0;
  if (!str.m_pData)
    return 1;

  size_t this_len = m_pData->m_nDataLength;
  size_t that_len = str.m_pData->m_nDataLength;
  size_t min_len = std::min(this_len, that_len);
  for (size_t i = 0; i < min_len; i++) {
    if (m_pData->m_String[i] < str.m_pData->m_String[i])
      return -1;
    if (m_pData->m_String[i] > str.m_pData->m_String[i])
      return 1;
  }
  if (this_len < that_len)
    return -1;
  if (this_len > that_len)
    return 1;
  return 0;
}

int WideString::CompareNoCase(const wchar_t* lpsz) const {
  if (m_pData)
    return lpsz ? FXSYS_wcsicmp(m_pData->m_String, lpsz) : 1;
  return (!lpsz || lpsz[0] == 0) ? 0 : -1;
}

size_t WideString::WStringLength(const unsigned short* str) {
  size_t len = 0;
  if (str)
    while (str[len])
      len++;
  return len;
}

void WideString::TrimRight(const WideStringView& pTargets) {
  if (IsEmpty() || pTargets.IsEmpty())
    return;

  size_t pos = GetLength();
  while (pos && pTargets.Contains(m_pData->m_String[pos - 1]))
    pos--;

  if (pos < m_pData->m_nDataLength) {
    ReallocBeforeWrite(m_pData->m_nDataLength);
    m_pData->m_String[pos] = 0;
    m_pData->m_nDataLength = pos;
  }
}

void WideString::TrimRight(wchar_t chTarget) {
  wchar_t str[2] = {chTarget, 0};
  TrimRight(str);
}

void WideString::TrimRight() {
  TrimRight(L"\x09\x0a\x0b\x0c\x0d\x20");
}

void WideString::TrimLeft(const WideStringView& pTargets) {
  if (!m_pData || pTargets.IsEmpty())
    return;

  size_t len = GetLength();
  if (len == 0)
    return;

  size_t pos = 0;
  while (pos < len) {
    size_t i = 0;
    while (i < pTargets.GetLength() &&
           pTargets.CharAt(i) != m_pData->m_String[pos]) {
      i++;
    }
    if (i == pTargets.GetLength()) {
      break;
    }
    pos++;
  }
  if (!pos)
    return;

  ReallocBeforeWrite(len);
  size_t nDataLength = len - pos;
  memmove(m_pData->m_String, m_pData->m_String + pos,
          (nDataLength + 1) * sizeof(wchar_t));
  m_pData->m_nDataLength = nDataLength;
}

void WideString::TrimLeft(wchar_t chTarget) {
  wchar_t str[2] = {chTarget, 0};
  TrimLeft(str);
}

void WideString::TrimLeft() {
  TrimLeft(L"\x09\x0a\x0b\x0c\x0d\x20");
}
float FX_wtof(const wchar_t* str, int len) {
  if (len == 0) {
    return 0.0;
  }
  int cc = 0;
  bool bNegative = false;
  if (str[0] == '+') {
    cc++;
  } else if (str[0] == '-') {
    bNegative = true;
    cc++;
  }
  int integer = 0;
  while (cc < len) {
    if (str[cc] == '.') {
      break;
    }
    integer = integer * 10 + FXSYS_DecimalCharToInt(str[cc]);
    cc++;
  }
  float fraction = 0;
  if (str[cc] == '.') {
    cc++;
    float scale = 0.1f;
    while (cc < len) {
      fraction += scale * FXSYS_DecimalCharToInt(str[cc]);
      scale *= 0.1f;
      cc++;
    }
  }
  fraction += static_cast<float>(integer);
  return bNegative ? -fraction : fraction;
}

int WideString::GetInteger() const {
  return m_pData ? FXSYS_wtoi(m_pData->m_String) : 0;
}

float WideString::GetFloat() const {
  return m_pData ? FX_wtof(m_pData->m_String, m_pData->m_nDataLength) : 0.0f;
}

std::wostream& operator<<(std::wostream& os, const WideString& str) {
  return os.write(str.c_str(), str.GetLength());
}

std::ostream& operator<<(std::ostream& os, const WideString& str) {
  os << str.UTF8Encode();
  return os;
}

std::wostream& operator<<(std::wostream& os, const WideStringView& str) {
  return os.write(str.unterminated_c_str(), str.GetLength());
}

std::ostream& operator<<(std::ostream& os, const WideStringView& str) {
  os << FX_UTF8Encode(str);
  return os;
}

}  // namespace fxcrt
