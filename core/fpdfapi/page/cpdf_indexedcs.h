// Copyright 2022 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FPDFAPI_PAGE_CPDF_INDEXEDCS_H_
#define CORE_FPDFAPI_PAGE_CPDF_INDEXEDCS_H_

#include <set>

#include "core/fpdfapi/page/cpdf_basedcs.h"
#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/fx_memory_wrappers.h"
#include "core/fxcrt/retain_ptr.h"

class CPDF_Document;

struct IndexedColorMinMax {
  float min;
  float max;
};
FX_DATA_PARTITION_EXCEPTION(IndexedColorMinMax);

class CPDF_IndexedCS final : public CPDF_BasedCS {
 public:
  CONSTRUCT_VIA_MAKE_RETAIN;
  ~CPDF_IndexedCS() override;

  // CPDF_ColorSpace:
  bool GetRGB(pdfium::span<const float> pBuf,
              float* R,
              float* G,
              float* B) const override;
  const CPDF_IndexedCS* AsIndexedCS() const override;
  uint32_t v_Load(CPDF_Document* pDoc,
                  const CPDF_Array* pArray,
                  std::set<const CPDF_Object*>* pVisited) override;

  int GetMaxIndex() const { return max_index_; }

 private:
  CPDF_IndexedCS();

  uint32_t base_component_count_ = 0;
  int max_index_ = 0;
  ByteString lookup_table_;
  DataVector<IndexedColorMinMax> component_min_max_;
};

#endif  // CORE_FPDFAPI_PAGE_CPDF_INDEXEDCS_H_
