//
// Created by 郑喆君 on 2018/6/21.
//

#include <eosiolib/eosio.hpp>
#include <string>
#include <eosiolib/print.hpp>
#include "inlinetransfer.hpp"
#include <eosiolib/contract.hpp>

using eosio::asset;
using eosio::permission_level;
using eosio::action;

class inlinetransfer : public eosio::contract {

public:
    explicit inlinetransfer(action_name self) : contract(self) {};

    //@abi action
    void transfer(const trans_request& request) {
        require_auth(request.from);

        eosio_assert(request.quantity.is_valid(), "invalid quantity");
        eosio_assert(request.quantity.amount > 0, "must bigger than 0");

        eosio::print("enter transfer, ", eosio::name{request.from}, "\t", eosio::name{request.to}, "\t", request.quantity);

        action(permission_level{request.from, N(active)}, N(eosio.token), N(transfer), std::make_tuple(request.from, request.to, request.quantity, std::string(""))
        ).send();

    }
};
EOSIO_ABI(inlinetransfer, (transfer))