#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>


struct trans_request {
    account_name from;
    account_name to;
    eosio::asset quantity;
};