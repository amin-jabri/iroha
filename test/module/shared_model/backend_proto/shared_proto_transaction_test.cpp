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

#include "backend/protobuf/transaction.hpp"
#include "builders/protobuf/transaction.hpp"

#include <gtest/gtest.h>

/**
 * @given protobuf transaction with transaction counter set
 * @when converted to shared model
 * @then shared model is created correctly
 */
TEST(ProtoTransaction, Create) {
  iroha::protocol::Transaction transaction;
  transaction.mutable_payload()->set_tx_counter(1);
  shared_model::proto::Transaction proto(transaction);
  ASSERT_EQ(proto.transactionCounter(), transaction.payload().tx_counter());
}

/**
 * generates sample transaction without commands
 * @return generated transaction
 */
iroha::protocol::Transaction generateEmptyTransaction() {
  uint64_t created_time = 10000000000ull;
  shared_model::interface::Transaction::TxCounterType tx_counter = 1;
  std::string account_id = "admin@test", asset_id = "coin#test",
              amount = "10.00";

  iroha::protocol::Transaction proto_tx;
  auto &payload = *proto_tx.mutable_payload();
  payload.set_tx_counter(tx_counter);
  payload.set_creator_account_id(account_id);
  payload.set_created_time(created_time);

  return proto_tx;
}

/**
 * Helper function to generate AddAssetQuantityCommand
 * @param account_id
 * @param asset_id
 * @return AddAssetQuantity protocol command
 */
iroha::protocol::AddAssetQuantity generateAddAssetQuantity(
    std::string account_id, std::string asset_id) {
  iroha::protocol::AddAssetQuantity command;

  command.set_account_id(account_id);
  command.set_asset_id(asset_id);
  command.mutable_amount()->mutable_value()->set_fourth(1000);
  command.mutable_amount()->set_precision(2);

  return command;
}

/**
 * @given transaction field values and sample command values, reference tx
 * @when create transaction with sample command using transaction builder
 * @then transaction is built correctly
 */
TEST(ProtoTransaction, Builder) {
  uint64_t created_time = 10000000000ull;
  shared_model::interface::Transaction::TxCounterType tx_counter = 1;
  std::string account_id = "admin@test", asset_id = "coin#test",
              amount = "10.00";

  iroha::protocol::Transaction proto_tx = generateEmptyTransaction();
  auto command =
      proto_tx.mutable_payload()->add_commands()->mutable_add_asset_quantity();

  command->CopyFrom(generateAddAssetQuantity(account_id, asset_id));

  auto tx = shared_model::proto::TransactionBuilder()
                .txCounter(tx_counter)
                .creatorAccountId(account_id)
                .assetQuantity(account_id, asset_id, amount)
                .createdTime(created_time)
                .build();
  auto &proto = tx.getTransport();

  ASSERT_EQ(proto_tx.SerializeAsString(), proto.SerializeAsString());
}

/**
 * @given transaction field values and sample command values with wrongly set
 * values
 * @when create transaction with sample command using transaction builder
 * @then transaction throws exception due to badly formed fields in commands
 */
TEST(ProtoTransaction, BuilderWithInvalidTx) {
  uint64_t created_time = 10000000000ull;
  shared_model::interface::Transaction::TxCounterType tx_counter = 1;
  std::string account_id = "admintest"; // account_id without @
  std::string asset_id = "coin#test", // asset_id without #
      amount = "10.00";

  iroha::protocol::Transaction proto_tx = generateEmptyTransaction();
  auto command =
      proto_tx.mutable_payload()->add_commands()->mutable_add_asset_quantity();

  command->CopyFrom(generateAddAssetQuantity(account_id, asset_id));

  ASSERT_THROW(shared_model::proto::TransactionBuilder()
                   .txCounter(tx_counter)
                   .creatorAccountId(account_id)
                   .assetQuantity(account_id, asset_id, amount)
                   .createdTime(created_time)
                   .build(),
               std::invalid_argument);
}
