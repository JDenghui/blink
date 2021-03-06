// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PAYMENTS_WEB_PAYMENT_CURRENCY_AMOUNT_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PAYMENTS_WEB_PAYMENT_CURRENCY_AMOUNT_H_

#include "third_party/blink/public/platform/web_string.h"

namespace blink {

// https://w3c.github.io/browser-payment-api/#paymentcurrencyamount-dictionary
struct WebPaymentCurrencyAmount {
  WebString currency;
  WebString value;
  WebString currency_system;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PAYMENTS_WEB_PAYMENT_CURRENCY_AMOUNT_H_
