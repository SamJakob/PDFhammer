// Copyright 2020 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FPDFSDK_FORMFILLER_CFFL_PRIVATEDATA_H_
#define FPDFSDK_FORMFILLER_CFFL_PRIVATEDATA_H_

#include <memory>

#include "core/fxcrt/observed_ptr.h"
#include "core/fxcrt/unowned_ptr.h"
#include "fpdfsdk/pwl/ipwl_systemhandler.h"

class CFFL_FormField;
class CPDFSDK_PageView;
class CPDFSDK_Widget;

class CFFL_PrivateData final : public IPWL_SystemHandler::PerWindowData {
 public:
  CFFL_PrivateData(CPDFSDK_Widget* pWidget,
                   const CPDFSDK_PageView* pPageView,
                   uint32_t nAppearanceAge,
                   uint32_t nValueAge);
  CFFL_PrivateData& operator=(const CFFL_PrivateData& that) = delete;
  ~CFFL_PrivateData() override;

  // IPWL_SystemHandler::PerWindowData:
  std::unique_ptr<IPWL_SystemHandler::PerWindowData> Clone() const override;

  CPDFSDK_Widget* GetWidget() const { return m_pWidget.Get(); }
  const CPDFSDK_PageView* GetPageView() const { return m_pPageView; }
  bool AppearanceAgeEquals(uint32_t age) const {
    return age == m_nAppearanceAge;
  }
  uint32_t GetValueAge() const { return m_nValueAge; }

  void SetFormField(CFFL_FormField* pFormField) { m_pFormField = pFormField; }
  CFFL_FormField* GetFormField() { return m_pFormField.Get(); }

 private:
  CFFL_PrivateData(const CFFL_PrivateData& that);

  ObservedPtr<CPDFSDK_Widget> m_pWidget;
  UnownedPtr<const CPDFSDK_PageView> const m_pPageView;
  UnownedPtr<CFFL_FormField> m_pFormField;
  const uint32_t m_nAppearanceAge;
  const uint32_t m_nValueAge;
};

#endif  // FPDFSDK_FORMFILLER_CFFL_PRIVATEDATA_H_
