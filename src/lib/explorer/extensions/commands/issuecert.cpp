/**
 * Copyright (c) 2016-2018 mvs developers
 *
 * This file is part of metaverse-explorer.
 *
 * metaverse-explorer is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <metaverse/explorer/json_helper.hpp>
#include <metaverse/explorer/extensions/commands/issuecert.hpp>
#include <metaverse/explorer/extensions/command_extension_func.hpp>
#include <metaverse/explorer/extensions/command_assistant.hpp>
#include <metaverse/explorer/extensions/exception.hpp>
#include <metaverse/explorer/extensions/base_helper.hpp>

namespace libbitcoin {
namespace explorer {
namespace commands {

console_result issuecert::invoke (Json::Value& jv_output,
                                  libbitcoin::server::server_node& node)
{
    auto& blockchain = node.chain_impl();
    blockchain.is_account_passwd_valid(auth_.name, auth_.auth);

    blockchain.uppercase_symbol(argument_.symbol);
    boost::to_lower(argument_.cert);

    // check asset symbol
    check_asset_symbol(argument_.symbol);

    auto to_did = argument_.to;
    auto to_address = get_address_from_did(to_did, blockchain);
    if (!blockchain.is_valid_address(to_address)) {
        throw address_invalid_exception{"invalid did parameter! " + to_did};
    }

    if (!blockchain.get_account_address(auth_.name, to_address)) {
        throw address_dismatch_account_exception{"target did does not match account. " + to_did};
    }

    // check asset cert types
    auto certs_create = check_issue_cert(blockchain, auth_.name, argument_.symbol, argument_.cert);

    // receiver
    std::vector<receiver_record> receiver{
        {
            to_address, argument_.symbol, 0, 0,
            certs_create, utxo_attach_type::asset_cert_issue,
            attachment("", to_did)
        }
    };

    if (!option_.memo.empty()) {
        check_message(option_.memo);

        receiver.push_back({
            to_address, "", 0, 0, utxo_attach_type::message,
            attachment(0, 0, blockchain_message(option_.memo))
        });
    }

    if (certs_create == asset_cert_ns::naming) {
        auto&& domain = asset_cert::get_domain(argument_.symbol);
        receiver.push_back({
            to_address, domain, 0, 0,
            asset_cert_ns::domain, utxo_attach_type::asset_cert,
            attachment("", to_did)
        });
    }

    auto helper = issuing_asset_cert(
                      *this, blockchain,
                      std::move(auth_.name), std::move(auth_.auth),
                      std::move(to_address), std::move(argument_.symbol),
                      std::move(receiver), argument_.fee);

    helper.exec();

    // json output
    auto tx = helper.get_transaction();
    jv_output = config::json_helper(get_api_version()).prop_tree(tx, true);

    return console_result::okay;
}


} // namespace commands
} // namespace explorer
} // namespace libbitcoin

