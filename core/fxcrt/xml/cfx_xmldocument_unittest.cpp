// Copyright 2018 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/xml/cfx_xmldocument.h"
#include "core/fxcrt/xml/cfx_xmlelement.h"
#include "core/fxcrt/xml/cfx_xmlinstruction.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(CFXXMLDocumentTest, Root) {
  CFX_XMLDocument doc;
  EXPECT_TRUE(doc.GetRoot() != nullptr);
}

TEST(CFXXMLDocumentTest, CreateNode) {
  CFX_XMLDocument doc;
  auto* node = doc.CreateNode<CFX_XMLElement>(L"elem");

  ASSERT_EQ(CFX_XMLNode::Type::kElement, node->GetType());
  EXPECT_EQ(L"elem", node->GetName());
}
