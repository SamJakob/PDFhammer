// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXGE_ANDROID_CFPF_SKIAFONT_H_
#define CORE_FXGE_ANDROID_CFPF_SKIAFONT_H_

#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxge/fx_font.h"

class CFPF_SkiaFontMgr;
class CFPF_SkiaPathFont;

class CFPF_SkiaFont {
 public:
  CFPF_SkiaFont();
  ~CFPF_SkiaFont();

  void Release();
  CFPF_SkiaFont* Retain();

  ByteString GetFamilyName();
  ByteString GetPsName();
  uint32_t GetFontStyle() const { return m_dwStyle; }
  uint8_t GetCharset() const { return m_uCharset; }
  int32_t GetGlyphIndex(wchar_t wUnicode);
  int32_t GetGlyphWidth(int32_t iGlyphIndex);
  int32_t GetAscent() const;
  int32_t GetDescent() const;
  bool GetGlyphBBox(int32_t iGlyphIndex, FX_RECT& rtBBox);
  bool GetBBox(FX_RECT& rtBBox);
  int32_t GetHeight() const;
  int32_t GetItalicAngle() const;
  uint32_t GetFontData(uint32_t dwTable, uint8_t* pBuffer, uint32_t dwSize);

  bool InitFont(CFPF_SkiaFontMgr* pFontMgr,
                CFPF_SkiaPathFont* pFont,
                const ByteStringView& bsFamily,
                uint32_t dwStyle,
                uint8_t uCharset);

 private:
  UnownedPtr<CFPF_SkiaFontMgr> m_pFontMgr;
  UnownedPtr<CFPF_SkiaPathFont> m_pFont;
  FXFT_Face m_Face = nullptr;
  uint32_t m_dwStyle = 0;
  uint8_t m_uCharset = 0;
  uint32_t m_dwRefCount = 0;
};

#endif  // CORE_FXGE_ANDROID_CFPF_SKIAFONT_H_
