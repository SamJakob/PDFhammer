// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CFXJSE_RESOLVEPROCESSOR_H_
#define FXJS_XFA_CFXJSE_RESOLVEPROCESSOR_H_

#include <memory>

#include "core/fxcrt/unowned_ptr.h"
#include "core/fxcrt/widestring.h"
#include "fxjs/xfa/cfxjse_engine.h"
#include "v8/include/cppgc/macros.h"
#include "xfa/fxfa/fxfa_basic.h"
#include "xfa/fxfa/parser/xfa_basic_data.h"

class CFXJSE_NodeHelper;

class CFXJSE_ResolveProcessor {
 public:
  class NodeData {
    CPPGC_STACK_ALLOCATED();  // Allows Raw/Unowned pointers.

   public:
    explicit NodeData(CFXJSE_Engine* pSC);
    ~NodeData();

    UnownedPtr<CFXJSE_Engine> const m_pSC;
    UnownedPtr<CXFA_Object> m_CurObject;  // Ok, stack-only.
    WideString m_wsName;
    WideString m_wsCondition;
    XFA_HashCode m_uHashName = XFA_HASHCODE_None;
    int32_t m_nLevel = 0;
    Mask<XFA_ResolveFlag> m_dwStyles = XFA_ResolveFlag::kChildren;
    CFXJSE_Engine::ResolveResult m_Result;
  };

  CFXJSE_ResolveProcessor();
  ~CFXJSE_ResolveProcessor();

  bool Resolve(v8::Isolate* pIsolate, NodeData& rnd);
  int32_t GetFilter(WideStringView wsExpression, int32_t nStart, NodeData& rnd);
  int32_t IndexForDataBind(const WideString& wsNextCondition, int32_t iCount);
  void SetCurStart(int32_t start) { m_iCurStart = start; }

  CFXJSE_NodeHelper* GetNodeHelper() { return m_pNodeHelper.get(); }

 private:
  bool ResolveForAttributeRs(CXFA_Object* curNode,
                             CFXJSE_Engine::ResolveResult* rnd,
                             WideStringView strAttr);
  bool ResolveAnyChild(v8::Isolate* pIsolate, NodeData& rnd);
  bool ResolveDollar(v8::Isolate* pIsolate, NodeData& rnd);
  bool ResolveExcalmatory(v8::Isolate* pIsolate, NodeData& rnd);
  bool ResolveNumberSign(v8::Isolate* pIsolate, NodeData& rnd);
  bool ResolveAsterisk(NodeData& rnd);
  bool ResolveNormal(v8::Isolate* pIsolate, NodeData& rnd);
  void SetStylesForChild(Mask<XFA_ResolveFlag> dwParentStyles, NodeData& rnd);

  void ConditionArray(size_t iCurIndex,
                      WideString wsCondition,
                      size_t iFoundCount,
                      NodeData* pRnd);
  void FilterCondition(v8::Isolate* pIsolate,
                       WideString wsCondition,
                       NodeData* pRnd);

  int32_t m_iCurStart = 0;
  std::unique_ptr<CFXJSE_NodeHelper> const m_pNodeHelper;
};

#endif  // FXJS_XFA_CFXJSE_RESOLVEPROCESSOR_H_
