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

#include "module/irohad/ametsuchi/ametsuchi_mocks.hpp"

#include <model/queries/responses/account_assets_response.hpp>
#include "model/queries/responses/account_response.hpp"
#include "model/queries/responses/asset_response.hpp"
#include "model/queries/responses/error_response.hpp"
#include "model/queries/responses/roles_response.hpp"
#include "model/query_execution.hpp"
#include "model/permissions.hpp"

using ::testing::Return;
using ::testing::AtLeast;
using ::testing::_;
using ::testing::AllOf;

using namespace iroha::ametsuchi;
using namespace iroha::model;

// TODO 26/09/17 grimadas: refactor (check CommandValidateExecuteTest test) IR-513

/**
 * Variables for testing
 */
auto ACCOUNT_ID = "test@test";
auto ADMIN_ID = "admin@test";
auto DOMAIN_NAME = "test";
auto ADVERSARY_ID = "adversary@test";
auto ASSET_ID = "coin";
auto ADMIN_ROLE = "admin";

/**
 * Default accounts for testing
 */

iroha::model::Account get_default_creator() {
  iroha::model::Account creator = iroha::model::Account();
  creator.account_id = ADMIN_ID;
  creator.domain_id = DOMAIN_NAME;
  creator.quorum = 1;
  // TODO: add role based permission
  return creator;
}

iroha::model::Account get_default_account() {
  auto dummy = iroha::model::Account();
  dummy.account_id = ACCOUNT_ID;
  dummy.domain_id = DOMAIN_NAME;
  dummy.quorum = 1;
  return dummy;
}

iroha::model::Account get_default_adversary() {
  auto dummy = iroha::model::Account();
  dummy.account_id = ADVERSARY_ID;
  dummy.domain_id = DOMAIN_NAME;
  dummy.quorum = 1;
  return dummy;
}

/**
 * Set default behaviour for Ametsuchi mock classes
 * @param test_wsv
 * @param test_blocks
 */
void set_default_ametsuchi(MockWsvQuery &test_wsv,
                           MockBlockQuery &test_blocks) {
  // If No account exist - return nullopt
  EXPECT_CALL(test_wsv, getAccount(_)).WillRepeatedly(Return(nonstd::nullopt));

  // Admin's account exist in the database
  auto admin = get_default_creator();
  EXPECT_CALL(test_wsv, getAccount(ADMIN_ID)).WillRepeatedly(Return(admin));
  // Test account exist in the database
  auto dummy = get_default_account();
  EXPECT_CALL(test_wsv, getAccount(ACCOUNT_ID)).WillRepeatedly(Return(dummy));
  // Adversary database exist in the database
  auto adversary = get_default_adversary();
  EXPECT_CALL(test_wsv, getAccount(ADVERSARY_ID))
      .WillRepeatedly(Return(adversary));
  // If no account_asset exist - return nullopt
  EXPECT_CALL(test_wsv, getAccountAsset(_, _))
      .WillRepeatedly(Return(nonstd::nullopt));

  std::vector<std::string> roles = {ADMIN_ROLE};
  EXPECT_CALL(test_wsv, getRoles()).WillRepeatedly(Return(roles));

  EXPECT_CALL(test_wsv, getAccountRoles(_))
      .WillRepeatedly(Return(nonstd::nullopt));
  EXPECT_CALL(test_wsv, getAccountRoles(ADMIN_ID))
      .WillRepeatedly(Return(roles));
  EXPECT_CALL(test_wsv, getAccountRoles(ACCOUNT_ID))
      .WillRepeatedly(Return(roles));

  std::vector<std::string> perms = {can_get_all_acc_ast_txs,
                                    can_get_all_acc_ast,
                                    can_get_all_acc_txs,
                                    can_get_all_accounts,
                                    can_get_all_signatories,
                                    can_read_assets,
                                    can_get_roles};
  EXPECT_CALL(test_wsv, getRolePermissions(_))
      .WillRepeatedly(Return(nonstd::nullopt));
  EXPECT_CALL(test_wsv, getRolePermissions(ADMIN_ROLE))
      .WillRepeatedly(Return(perms));
  auto def_asset = Asset(ASSET_ID, DOMAIN_NAME, 2);
  EXPECT_CALL(test_wsv, getAsset(_)).WillRepeatedly(Return(nonstd::nullopt));
  EXPECT_CALL(test_wsv, getAsset(ASSET_ID)).WillRepeatedly(Return(def_asset));
  // Test account has some amount of test assets
  auto acct_asset = iroha::model::AccountAsset();
  acct_asset.asset_id = ASSET_ID;
  acct_asset.account_id = ACCOUNT_ID;
  iroha::Amount balance(150);
  acct_asset.balance = balance;
  EXPECT_CALL(test_wsv, getAccountAsset(ACCOUNT_ID, ASSET_ID))
      .WillRepeatedly(Return(acct_asset));
  acct_asset.account_id = ADMIN_ID;
  EXPECT_CALL(test_wsv, getAccountAsset(ADMIN_ID, ASSET_ID))
      .WillRepeatedly(Return(acct_asset));
  EXPECT_CALL(test_wsv, hasAccountGrantablePermission(_, _, _))
      .WillRepeatedly(Return(false));
}

TEST(QueryExecutor, get_account) {
  auto wsv_queries = std::make_shared<MockWsvQuery>();
  auto block_queries = std::make_shared<MockBlockQuery>();

  auto query_proccesor =
      iroha::model::QueryProcessingFactory(wsv_queries, block_queries);

  set_default_ametsuchi(*wsv_queries, *block_queries);

  // Valid cases:
  // 1. Admin asks about test account
  auto query = std::make_shared<iroha::model::GetAccount>();
  query->account_id = ACCOUNT_ID;
  query->creator_account_id = ADMIN_ID;

  auto response = query_proccesor.execute(query);
  auto cast_resp =
      std::static_pointer_cast<iroha::model::AccountResponse>(response);
  ASSERT_EQ(cast_resp->account.account_id, ACCOUNT_ID);

  // 2. Account creator asks about his account
  query->account_id = ADMIN_ID;
  query->creator_account_id = ADMIN_ID;
  response = query_proccesor.execute(query);
  cast_resp = std::static_pointer_cast<iroha::model::AccountResponse>(response);
  ASSERT_EQ(cast_resp->account.account_id, ADMIN_ID);

  // --------- Non valid cases: -------

  // 1. Asking non-existing account

  query->account_id = "nonacct";
  query->creator_account_id = ADMIN_ID;
  response = query_proccesor.execute(query);
  auto cast_resp_2 =
      std::dynamic_pointer_cast<iroha::model::AccountResponse>(response);
  ASSERT_EQ(cast_resp_2, nullptr);
  auto err_resp =
      std::dynamic_pointer_cast<iroha::model::ErrorResponse>(response);
  ASSERT_EQ(err_resp->reason, iroha::model::ErrorResponse::NO_ACCOUNT);

  // 2. No rights to ask account
  query->account_id = ACCOUNT_ID;
  query->creator_account_id = ADVERSARY_ID;
  response = query_proccesor.execute(query);
  cast_resp =
      std::dynamic_pointer_cast<iroha::model::AccountResponse>(response);
  ASSERT_EQ(cast_resp, nullptr);

  err_resp = std::dynamic_pointer_cast<iroha::model::ErrorResponse>(response);
  ASSERT_EQ(err_resp->reason, iroha::model::ErrorResponse::STATEFUL_INVALID);

  // 3. No creator
  query->account_id = ACCOUNT_ID;
  query->creator_account_id = "noacc";
  response = query_proccesor.execute(query);
  cast_resp =
      std::dynamic_pointer_cast<iroha::model::AccountResponse>(response);
  ASSERT_EQ(cast_resp, nullptr);

  err_resp = std::dynamic_pointer_cast<iroha::model::ErrorResponse>(response);
  ASSERT_EQ(err_resp->reason, iroha::model::ErrorResponse::STATEFUL_INVALID);

  // TODO: tests for signatures
}

TEST(QueryExecutor, get_account_assets) {
  auto wsv_queries = std::make_shared<MockWsvQuery>();
  auto block_queries = std::make_shared<MockBlockQuery>();

  auto query_proccesor =
      iroha::model::QueryProcessingFactory(wsv_queries, block_queries);

  set_default_ametsuchi(*wsv_queries, *block_queries);

  // Valid cases:
  // 1. Admin asks account_id
  auto query = std::make_shared<iroha::model::GetAccountAssets>();
  query->account_id = ACCOUNT_ID;
  query->asset_id = ASSET_ID;
  query->creator_account_id = ADMIN_ID;
  auto response = query_proccesor.execute(query);
  auto cast_resp =
      std::static_pointer_cast<iroha::model::AccountAssetResponse>(response);
  ASSERT_EQ(cast_resp->acct_asset.account_id, ACCOUNT_ID);
  ASSERT_EQ(cast_resp->acct_asset.asset_id, ASSET_ID);

  // 2. Account creator asks about his account
  query->account_id = ADMIN_ID;
  query->creator_account_id = ADMIN_ID;
  response = query_proccesor.execute(query);
  cast_resp =
      std::static_pointer_cast<iroha::model::AccountAssetResponse>(response);
  ASSERT_EQ(cast_resp->acct_asset.account_id, ADMIN_ID);
  ASSERT_EQ(cast_resp->acct_asset.asset_id, ASSET_ID);

  // --------- Non valid cases: -------

  // 1. Asking non-existed account asset

  query->account_id = "nonacct";
  query->creator_account_id = ADMIN_ID;
  response = query_proccesor.execute(query);
  auto cast_resp_2 =
      std::dynamic_pointer_cast<iroha::model::AccountAssetResponse>(response);
  ASSERT_EQ(cast_resp_2, nullptr);
  auto err_resp =
      std::dynamic_pointer_cast<iroha::model::ErrorResponse>(response);
  ASSERT_EQ(err_resp->reason, iroha::model::ErrorResponse::NO_ACCOUNT_ASSETS);

  // Asking non-existed account asset

  query->account_id = ACCOUNT_ID;
  query->asset_id = "nonasset";
  query->creator_account_id = ADMIN_ID;
  response = query_proccesor.execute(query);
  cast_resp_2 =
      std::dynamic_pointer_cast<iroha::model::AccountAssetResponse>(response);
  ASSERT_EQ(cast_resp_2, nullptr);
  err_resp = std::dynamic_pointer_cast<iroha::model::ErrorResponse>(response);
  ASSERT_EQ(err_resp->reason, iroha::model::ErrorResponse::NO_ACCOUNT_ASSETS);

  // 2. No rights to ask
  query->account_id = ACCOUNT_ID;
  query->asset_id = ASSET_ID;
  query->creator_account_id = ADVERSARY_ID;
  response = query_proccesor.execute(query);
  cast_resp =
      std::dynamic_pointer_cast<iroha::model::AccountAssetResponse>(response);
  ASSERT_EQ(cast_resp, nullptr);

  err_resp = std::dynamic_pointer_cast<iroha::model::ErrorResponse>(response);
  ASSERT_EQ(err_resp->reason, iroha::model::ErrorResponse::STATEFUL_INVALID);

  // 3. No creator
  query->account_id = ACCOUNT_ID;
  query->creator_account_id = "noacct";
  response = query_proccesor.execute(query);
  cast_resp =
      std::dynamic_pointer_cast<iroha::model::AccountAssetResponse>(response);
  ASSERT_EQ(cast_resp, nullptr);

  err_resp = std::dynamic_pointer_cast<iroha::model::ErrorResponse>(response);
  ASSERT_EQ(err_resp->reason, iroha::model::ErrorResponse::STATEFUL_INVALID);

  // TODO: tests for signatures
}

TEST(QueryExecutor, get_asset_info) {
  auto wsv_queries = std::make_shared<MockWsvQuery>();
  auto block_queries = std::make_shared<MockBlockQuery>();

  auto query_proccesor =
      iroha::model::QueryProcessingFactory(wsv_queries, block_queries);

  set_default_ametsuchi(*wsv_queries, *block_queries);

  auto query = std::make_shared<GetAssetInfo>(ASSET_ID);
  query->creator_account_id = ADMIN_ID;
  auto response = query_proccesor.execute(query);
  auto cast_resp = std::static_pointer_cast<AssetResponse>(response);
  ASSERT_EQ(ASSET_ID, cast_resp->asset.asset_id);
  // TODO: add more bad test cases
}

TEST(QueryExecutor, get_roles) {
  auto wsv_queries = std::make_shared<MockWsvQuery>();
  auto block_queries = std::make_shared<MockBlockQuery>();

  auto query_proccesor =
      iroha::model::QueryProcessingFactory(wsv_queries, block_queries);

  set_default_ametsuchi(*wsv_queries, *block_queries);

  auto query = std::make_shared<GetRoles>();
  query->creator_account_id = ADMIN_ID;
  auto response = query_proccesor.execute(query);
  auto cast_resp = std::static_pointer_cast<RolesResponse>(response);
  ASSERT_EQ(1, cast_resp->roles.size());
  ASSERT_EQ(ADMIN_ROLE, cast_resp->roles.at(0));
  // TODO: add more test cases
}

TEST(QueryExecutor, get_role_permissions) {
  auto wsv_queries = std::make_shared<MockWsvQuery>();
  auto block_queries = std::make_shared<MockBlockQuery>();

  auto query_proccesor =
      iroha::model::QueryProcessingFactory(wsv_queries, block_queries);

  set_default_ametsuchi(*wsv_queries, *block_queries);

  auto query = std::make_shared<GetRolePermissions>(ADMIN_ROLE);
  query->creator_account_id = ADMIN_ID;
  auto response = query_proccesor.execute(query);
  auto cast_resp = std::static_pointer_cast<RolePermissionsResponse>(response);
  ASSERT_GT(cast_resp->role_permissions.size(), 0);
  // TODO: add more test cases
}

