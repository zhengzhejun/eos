#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <string>
#include <eosiolib/time.hpp>

struct BidRequest {
    account_name account;
    uint64_t adId;
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

    bool is_collide(eosio::time_point_sec stime, eosio::time_point_sec etime) const {
        return !((starttime >= stime.utc_seconds && starttime < etime.utc_seconds) | (endtime > stime.utc_seconds && endtime <= etime.utc_seconds)
                 | (stime.utc_seconds >= starttime && stime.utc_seconds < endtime) | (etime.utc_seconds > starttime && etime.utc_seconds <= endtime));
    }
};
