//
// Created by 郑喆君 on 2018/6/21.
//

#include <eosiolib/eosio.hpp>
#include <string>
#include <eosiolib/print.hpp>
#include <eosiolib/contract.hpp>
#include <string>
#include "adbid.hpp"

using eosio::asset;
using eosio::permission_level;
using eosio::action;

class Adbid : public eosio::contract {
public:
    explicit Adbid(action_name self) : contract(self) {};

    // 竞价
    //@abi action
    void bid(const BidRequest& bidRequest) {
        require_auth(bidRequest.account);
        eosio_assert(bidRequest.quantity.is_valid(), "invalid quantity");
        eosio_assert(bidRequest.quantity.amount > 0, "amout must > 0");
        eosio_assert(bidRequest.quantity.symbol == S(4,EPRA), "asset only to epra");
        eosio_assert(bidRequest.imgurl.size() > 0 && bidRequest.imgurl.size() < 300, "imgurl is empty or too long");
        eosio_assert(bidRequest.landurl.size() > 0 && bidRequest.landurl.size() < 300, "landurl is empty or too long");

        adpos_index adposes(_self, _self);
        account_index accounts(_self, _self);
        auto adpos_itr = adposes.find(bidRequest.adId);
        eosio_assert(adpos_itr != adposes.end(), "adpos is not exsit");
        eosio_assert(adpos_itr->bidstarttime.utc_seconds <= now() && adpos_itr->bidendtime.utc_seconds >= now(), "bid time error");
        auto account_itr = accounts.find(bidRequest.account);
        eosio_assert(account_itr != accounts.end(), "account is not exsit");
        eosio_assert(account_itr->balance >= bidRequest.quantity, "balance is not enough");
        eosio_assert(bidRequest.quantity > adpos_itr->bidasset, "bid is less than current");

        // 退款给上一个竞拍者
        if(adpos_itr->has_bid) {
            auto preaccount = adpos_itr->account;
            auto preaccount_itr = accounts.find(preaccount);
            eosio_assert(preaccount_itr != accounts.end(), "preaccount is not exsit");
            accounts.modify(preaccount_itr, 0, [&](auto& acnt) {
                acnt.balance += adpos_itr->bidasset;
                acnt.bidding_count--;
            });

        }

        // 当前竞拍者汇款给系统
        accounts.modify(account_itr, 0, [&](auto& acnt){
            acnt.balance -= bidRequest.quantity;
            acnt.bidding_count++;
        });
        adposes.modify(adpos_itr, 0, [&](auto& adpos) {
            adpos.account = bidRequest.account;
            adpos.bidasset = bidRequest.quantity;
            adpos.imgurl = bidRequest.imgurl;
            adpos.landurl = bidRequest.landurl;
        });


    }

    // 清理过期广告位
    //@abi action
    void clearexpire() {
        require_auth(_self);
        adpos_index adposes(_self, _self);
        std::vector<uint64_t> ids;
        for(auto itr = adposes.begin(); itr != adposes.end(); itr++) {
            if(itr->endtime.utc_seconds < now()) {
                ids.push_back(itr->id);
            }
        }
        for(int i = 0; i < ids.size(); i++) {
            auto itr = adposes.find(ids[i]);
            if(itr != adposes.end()){
                adposes.erase(itr);
            }
        }
    }

    // 发布广告位
    //@abi action
    void publishadpos(const PublishRequest& publishRequest) {
        require_auth(_self);
        eosio_assert(publishRequest.starttime <= publishRequest.endtime && publishRequest.bidstarttime <= publishRequest.bidendtime
                    && publishRequest.bidendtime <= publishRequest.starttime && publishRequest.bidstarttime > now(), "time error");
        adpos_index adposes(_self, _self);

        for(auto itr = adposes.begin(); itr != adposes.end(); itr++) {
            eosio_assert(publishRequest.is_collide(itr->starttime, itr->endtime), "time collide");
        }

        adposes.emplace(_self, [&](auto& adpos) {
            adpos.id = adposes.available_primary_key();
            adpos.starttime = eosio::time_point_sec(publishRequest.starttime);
            adpos.endtime = eosio::time_point_sec(publishRequest.endtime);
            adpos.bidstarttime = eosio::time_point_sec(publishRequest.bidstarttime);
            adpos.bidendtime = eosio::time_point_sec(publishRequest.bidendtime);
        });

    }

    // 竞价者抵押epra
    //@abi action
    void deposit(const DepositRequest& depositRequest) {
        eosio_assert(depositRequest.quantity.is_valid(), "invalid quantity");
        eosio_assert(depositRequest.quantity.amount > 0, "amout must > 0");
        eosio_assert(depositRequest.quantity.symbol == S(4,EPRA), "asset only to epra");
        require_auth(depositRequest.account);
        account_index accounts(_self, _self);
        account_name account = depositRequest.account;
        auto itr = accounts.find(account);
        if(itr == accounts.end()) {
            itr = accounts.emplace(_self, [&](auto& acnt){
                acnt.account = account;
            });
        }

        action(permission_level{account, N(active)}, N(eosio.token), N(transfer), std::make_tuple(account, _self, depositRequest.quantity, std::string(""))
        ).send();

        accounts.modify(itr, 0, [&](auto& acnt){
            acnt.balance += depositRequest.quantity;
        });

    }

    // 竞价者提取epra
    //@abi action
    void withdraw(const WithdrawRequest& withdrawRequest) {
        eosio_assert(withdrawRequest.quantity.is_valid(), "invalid quantity");
        eosio_assert(withdrawRequest.quantity.amount > 0, "amout must > 0");
        eosio_assert(withdrawRequest.quantity.symbol == S(4,EPRA), "asset only to epra");
        require_auth(withdrawRequest.account);
        account_index accounts(_self, _self);
        auto itr = accounts.find(withdrawRequest.account);
        eosio_assert(itr != accounts.end(), "unknown account");

        accounts.modify(itr, 0, [&](auto& acnt) {
            eosio_assert(acnt.balance >= withdrawRequest.quantity, "insufficient balance");
            acnt.balance -= withdrawRequest.quantity;
        });

        action(permission_level{ _self, N(active) }, N(eosio.token), N(transfer),
                std::make_tuple(_self, withdrawRequest.account, withdrawRequest.quantity, std::string(""))
        ).send();
        if(itr->balance.amount == 0 && itr->bidding_count == 0) {
            accounts.erase(itr);
        }
    }

private:
    //@abi table adpos i64
    struct AdPos {
        uint64_t id;
        eosio::time_point_sec starttime;
        eosio::time_point_sec endtime;
        eosio::time_point_sec bidstarttime;
        eosio::time_point_sec bidendtime;
        std::string imgurl;
        std::string landurl;
        asset bidasset;
        account_name account;
        bool has_bid = false;

        uint64_t primary_key() const { return id; }

        EOSLIB_SERIALIZE(AdPos, (id)(starttime)(endtime)(bidstarttime)(bidendtime)(imgurl)(landurl)(bidasset)(account));
    };
    typedef eosio::multi_index<N(adpos), AdPos> adpos_index;

    //@abi table account i64
    struct Account {
        account_name account;
        asset balance;
        uint32_t bid_success_count = 0;
        uint32_t bidding_count = 0;

        uint64_t primary_key() const { return account; }
        EOSLIB_SERIALIZE(Account, (account)(balance)(bid_success_count)(bidding_count));
    };
    typedef eosio::multi_index<N(account), Account> account_index;

};
EOSIO_ABI(Adbid, (bid)(clearexpire)(publishadpos)(deposit)(withdraw))
