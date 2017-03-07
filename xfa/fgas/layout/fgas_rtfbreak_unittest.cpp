// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fgas/layout/fgas_rtfbreak.h"

#include <memory>

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/test_support.h"
#include "third_party/base/ptr_util.h"
#include "xfa/fgas/font/cfgas_fontmgr.h"
#include "xfa/fgas/font/cfgas_gefont.h"

class RTFBreakTest : public testing::Test {
 public:
  void SetUp() override {
    CFX_GEModule::Get()->GetFontMgr()->SetSystemFontInfo(
        IFX_SystemFontInfo::CreateDefault(nullptr));

#if _FXM_PLATFORM_ == _FXM_PLATFORM_WINDOWS_
    font_mgr_ = CFGAS_FontMgr::Create(FX_GetDefFontEnumerator());
#else
    font_source_ = pdfium::MakeUnique<CFX_FontSourceEnum_File>();
    font_mgr_ = CFGAS_FontMgr::Create(font_source_.get());
#endif

    font_ = CFGAS_GEFont::LoadFont(L"Arial Black", 0, 0, font_mgr_.get());
    ASSERT(font_.Get() != nullptr);
  }

  std::unique_ptr<CFX_RTFBreak> CreateBreak(int32_t args) {
    auto b = pdfium::MakeUnique<CFX_RTFBreak>(args);
    b->SetFont(font_);
    return b;
  }

 private:
  std::unique_ptr<CFGAS_FontMgr> font_mgr_;
  CFX_RetainPtr<CFGAS_GEFont> font_;

#if _FXM_PLATFORM_ != _FXM_PLATFORM_WINDOWS_
  std::unique_ptr<CFX_FontSourceEnum_File> font_source_;
#endif
};

// As soon as you get one of the control characters the break is complete
// and must be consumed before you get any more characters ....

TEST_F(RTFBreakTest, AddChars) {
  auto b = CreateBreak(FX_RTFLAYOUTSTYLE_ExpandTab);

  CFX_WideString str(L"Input String.");
  for (int32_t i = 0; i < str.GetLength(); i++)
    EXPECT_EQ(CFX_RTFBreakType::None, b->AppendChar(str.GetAt(i)));

  EXPECT_EQ(CFX_RTFBreakType::Paragraph, b->AppendChar(L'\n'));
  ASSERT_EQ(1, b->CountBreakPieces());
  EXPECT_EQ(str + L"\n", b->GetBreakPieceUnstable(0)->GetString());

  b->ClearBreakPieces();
  b->Reset();
  EXPECT_EQ(0, b->GetCurrentLineForTesting()->GetLineEnd());

  str = L"Second str.";
  for (int32_t i = 0; i < str.GetLength(); i++)
    EXPECT_EQ(CFX_RTFBreakType::None, b->AppendChar(str.GetAt(i)));

  // Force the end of the break at the end of the string.
  b->EndBreak(CFX_RTFBreakType::Paragraph);
  ASSERT_EQ(1, b->CountBreakPieces());
  EXPECT_EQ(str, b->GetBreakPieceUnstable(0)->GetString());
}

TEST_F(RTFBreakTest, ControlCharacters) {
  auto b = CreateBreak(FX_RTFLAYOUTSTYLE_ExpandTab);
  EXPECT_EQ(CFX_RTFBreakType::Line, b->AppendChar(L'\v'));
  EXPECT_EQ(CFX_RTFBreakType::Page, b->AppendChar(L'\f'));
  // 0x2029 is the Paragraph Separator unicode character.
  EXPECT_EQ(CFX_RTFBreakType::Paragraph, b->AppendChar(0x2029));
  EXPECT_EQ(CFX_RTFBreakType::Paragraph, b->AppendChar(L'\n'));

  ASSERT_EQ(1, b->CountBreakPieces());
  EXPECT_EQ(L"\v", b->GetBreakPieceUnstable(0)->GetString());
}
