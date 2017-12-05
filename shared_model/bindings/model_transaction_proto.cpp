/**
 * Copyright Soramitsu Co., Ltd. 2017 All Rights Reserved.
 * http://soramitsu.co.jp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "bindings/model_transaction_proto.hpp"
#include "interfaces/polymorphic_wrapper.hpp"
#include "primitive.pb.h"

namespace shared_model {
  namespace bindings {
    crypto::Blob SimpleTransactionProto::signAndAddSignature(
        proto::UnsignedWrapper<proto::Transaction> &tx,
        const crypto::Keypair &keypair) {
      return tx.signAndAddSignature(keypair).blob();
    }
  }  // namespace bindings
}  // namespace shared_model
