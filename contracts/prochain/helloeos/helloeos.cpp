//
// Created by 郑喆君 on 2018/6/19.
//

#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>

using namespace eosio;

class helloeos : public eosio::contract {

public:
    using contract::contract;

    void hello(account_name user) {
        print("Welcome to EOS! ", name{user});
    }

};

EOSIO_ABI(helloeos, (hello));