#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <string>

struct BidRequest {
    account_name account;
    uint8_t adId;
    eosio::asset quantity;
    std::string imgurl = "";
    std::string landurl = "";
};

struct DepositRequest {
    account_name account;
    eosio::asset quantity;
};

struct WithdrawRequest {
    account_name account;
    eosio::asset quantity;
};

struct PublishRequest {
    uint32_t starttime;
    uint32_t endtime;
    uint32_t bidstarttime;
    uint32_t bidendtime;
};
